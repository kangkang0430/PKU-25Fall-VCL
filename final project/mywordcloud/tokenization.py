from __future__ import division
from itertools import tee
from operator import itemgetter
from collections import defaultdict
from math import log


def l(k, n, x):
    # 计算Dunning似然比的对数项
    return log(max(x, 1e-10)) * k + log(max(1 - x, 1e-10)) * (n - k)


def score(count_bigram, count1, count2, n_words):
    if n_words <= count1 or n_words <= count2:
        return 0
    n = n_words
    c12 = count_bigram
    c1 = count1
    c2 = count2
    p = c2 / n
    p1 = c12 / c1
    p2 = (c2 - c12) / (n - c1)
    score = (l(c12, c1, p) + l(c2 - c12, n - c1, p)
             - l(c12, c1, p1) - l(c2 - c12, n - c1, p2))
    return -2 * score


def pairwise(iterable):
    # 将单词列表转换为相邻单词对
    # is -> (s0,s1), (s1,s2), (s2, s3), ...
    a, b = tee(iterable)
    next(b, None)
    return zip(a, b)


def unigrams_and_bigrams(words, stopwords, normalize_plurals=True, collocation_threshold=30):
    # 在从单词中移除停用词之前创建二元词组
    # 不允许二元词组中存在停用词
    bigrams = list(p for p in pairwise(words) if not any(w.lower() in stopwords for w in p))
    unigrams = list(w for w in words if w.lower() not in stopwords)
    n_words = len(unigrams)

    # 处理单字词：归一化大小写和复数形式
    counts_unigrams, standard_form = process_tokens(unigrams, normalize_plurals=normalize_plurals)

    # 处理二元词组：将元组转换为字符串，然后归一化
    counts_bigrams, standard_form_bigrams = process_tokens(
        [" ".join(bigram) for bigram in bigrams],
        normalize_plurals=normalize_plurals)

    # 创建counts_unigrams的副本，以便分数计算不会改变原始数据
    orig_counts = counts_unigrams.copy()

    for bigram_string, count in counts_bigrams.items():
        bigram = tuple(bigram_string.split(" "))
        word1 = standard_form[bigram[0].lower()]
        word2 = standard_form[bigram[1].lower()]

        collocation_score = score(count, orig_counts[word1], orig_counts[word2], n_words)
        if collocation_score > collocation_threshold:
            counts_unigrams[word1] -= counts_bigrams[bigram_string]
            counts_unigrams[word2] -= counts_bigrams[bigram_string]

            # 将二元词组作为一个整体添加到计数中
            counts_unigrams[bigram_string] = counts_bigrams[bigram_string]

    # 删除计数为0或负数的单词
    for word, count in list(counts_unigrams.items()):
        if count <= 0:
            del counts_unigrams[word]
    return counts_unigrams


def process_tokens(words, normalize_plurals=True):
    """标准化大小写并移除复数形式"""
    d = defaultdict(dict)
    for word in words:
        word_lower = word.lower()
        case_dict = d[word_lower]
        case_dict[word] = case_dict.get(word, 0) + 1

    # 合并复数形式
    if normalize_plurals:
        merged_plurals = {}
        for key in list(d.keys()):
            if key.endswith('s') and not key.endswith("ss"):
                key_singular = key[:-1]

                if key_singular in d:
                    dict_plural = d[key]
                    dict_singular = d[key_singular]

                    for word, count in dict_plural.items():
                        singular = word[:-1]
                        dict_singular[singular] = (
                            dict_singular.get(singular, 0) + count)

                    merged_plurals[key] = key_singular
                    del d[key]

    fused_cases = {}
    standard_cases = {}
    item1 = itemgetter(1)

    for word_lower, case_dict in d.items():
        # 获取最常见的大小写形式
        first = max(case_dict.items(), key=item1)[0]
        fused_cases[first] = sum(case_dict.values())
        standard_cases[word_lower] = first

    if normalize_plurals:
        for plural, singular in merged_plurals.items():
            standard_cases[plural] = standard_cases[singular.lower()]

    return fused_cases, standard_cases
