from __future__ import division

import warnings
from random import Random
import os
import colorsys
import numpy as np

from PIL import Image
from PIL import ImageColor
from PIL import ImageDraw
from PIL import ImageFilter
from PIL import ImageFont

from .query_integral_image import query_integral_image


FILE = os.path.dirname(__file__)
FONT_PATH = os.environ.get('FONT_PATH', os.path.join(FILE, 'DroidSansMono.ttf'))
with open(os.path.join(FILE, 'stopwords')) as f:
    STOPWORDS = set(map(str.strip, f.readlines()))


class IntegralOccupancyMap(object):
    def __init__(self, height, width, mask):
        self.height = height
        self.width = width
        if mask is not None:
            self.integral = np.cumsum(np.cumsum(255 * mask, axis=1), axis=0).astype(np.uint32)
        else:
            self.integral = np.zeros((height, width), dtype=np.uint32)

    def sample_position(self, size_x, size_y, random_state):
        return query_integral_image(self.integral, size_x, size_y, random_state)

    def update(self, img_array, pos_x, pos_y):
        partial_integral = np.cumsum(np.cumsum(img_array[pos_x:, pos_y:], axis=1), axis=0)
        # 将重新计算的部分粘贴回原积分图像
        if pos_x > 0:
            if pos_y > 0:
                partial_integral += (self.integral[pos_x - 1, pos_y:] - self.integral[pos_x - 1, pos_y - 1])
            else:
                partial_integral += self.integral[pos_x - 1, pos_y:]

        if pos_y > 0:
            partial_integral += self.integral[pos_x:, pos_y - 1][:, np.newaxis]

        self.integral[pos_x:, pos_y:] = partial_integral


def random_color_func(word=None, font_size=None, position=None,
                      orientation=None, font_path=None, random_state=None):
    """随机色调颜色生成函数"""

    # 随机选择一个色调，饱和度固定为80%，亮度固定为50%
    if random_state is None:
        random_state = Random()
    return "hsl(%d, 80%%, 50%%)" % random_state.randint(0, 255)


class colormap_color_func(object):
    """从matplotlib颜色映射创建的颜色函数类"""
    def __init__(self, colormap):
        import matplotlib.pyplot as plt
        self.colormap = plt.get_cmap(colormap)

    def __call__(self, word, font_size, position, orientation,
                 random_state=None, **kwargs):
        if random_state is None:
            random_state = Random()
        r, g, b, _ = np.maximum(0, 255 * np.array(self.colormap(
            random_state.uniform(0, 1))))
        return "rgb({:.0f}, {:.0f}, {:.0f})".format(r, g, b)


def get_single_color_func(color):
    """创建一个返回一个单一的色调和饱和度的颜色函数"""
    old_r, old_g, old_b = ImageColor.getrgb(color)
    rgb_max = 255.
    h, s, v = colorsys.rgb_to_hsv(old_r / rgb_max, old_g / rgb_max, old_b / rgb_max)

    def single_color_func(word=None, font_size=None, position=None,
                          orientation=None, font_path=None, random_state=None):
        """生成随机颜色"""
        if random_state is None:
            random_state = Random()
        r, g, b = colorsys.hsv_to_rgb(h, s, random_state.uniform(0.2, 1))
        return 'rgb({:.0f}, {:.0f}, {:.0f})'.format(r * rgb_max, g * rgb_max, b * rgb_max)
    return single_color_func


class WordCloud(object):
    """这是一个用于生成词云的类，可以把文本变成漂亮的图片

    常用参数说明：
    1. 字体设置：
        font_path：字体文件路径，不设置就用默认字体

    2. 图片大小：
        width：图片宽度（默认400）
        height：图片高度（默认200）

    3. 蒙版功能：
        mask：可以做成特定形状的词云，比如心形、圆形等

    4. 文字设置：
        max_words：最多显示多少个词（默认200）
        min_font_size：最小字体大小
        max_font_size：最大字体大小

    5. 颜色设置：
        background_color：背景颜色
        colormap：词的颜色方案

    6. 其他实用功能：
        stopwords：停用词，这些词不会显示
        repeat：如果词太少，是否重复显示
        prefer_horizontal：让词尽量横着显示的概率（0-1之间）
    """

    def __init__(self, font_path=None, width=800, height=600, mask=None,
                 max_words=200, background_color='white', colormap='viridis',
                 min_font_size=10, max_font_size=100):
        """
        词云生成器初始化
        """
        from random import Random

        # 字体
        if font_path is None:
            font_path = FONT_PATH
        self.font_path = font_path

        # 尺寸
        if mask is not None:
            self.width = mask.shape[1]
            self.height = mask.shape[0]
        else:
            self.width = width
            self.height = height
        self.mask = mask

        # 主要参数
        self.max_words = max_words
        self.background_color = background_color
        self.colormap = colormap
        self.min_font_size = min_font_size
        self.max_font_size = max_font_size

        # 固定参数（包括轮廓相关）
        self.repeat = False
        self.stopwords = set()
        self.random_state = Random(42)
        self.prefer_horizontal = 0.9
        self.relative_scaling = 0.5

        # 轮廓设置
        self.contour_width = 0  # 设为0表示不画轮廓
        self.contour_color = 'black'

        # 技术参数
        self.margin = 2
        self.scale = 1
        self.font_step = 1
        self.mode = "RGB"
        self.color_func = colormap_color_func(colormap)

    def fit_words(self, frequencies):
        """
        根据单词和频率生成词云
        """
        return self.generate_from_frequencies(frequencies)

    def generate_from_frequencies(self, frequencies, max_font_size=None):  # noqa: C901
        """
        根据单词和频率生成词云
        """

        # 1. 检查输入
        if not frequencies:
            raise ValueError("至少需要一个词来生成词云")

        # 2. 处理词频数据
        # 排序并限制数量
        frequencies = sorted(frequencies.items(), key=lambda x: x[1], reverse=True)
        frequencies = frequencies[:self.max_words]

        # 归一化
        max_frequency = float(frequencies[0][1])
        frequencies = [(word, freq / max_frequency) for word, freq in frequencies]

        # 3. 初始化
        random_state = self.random_state

        if self.mask is not None:
            boolean_mask = self._get_boolean_mask(self.mask)
            width = self.mask.shape[1]
            height = self.mask.shape[0]
        else:
            boolean_mask = None
            height, width = self.height, self.width

        # 创建碰撞检测图
        occupancy = IntegralOccupancyMap(height, width, boolean_mask)

        # 创建临时图像用于检测
        img_grey = Image.new("L", (width, height))
        draw = ImageDraw.Draw(img_grey)

        # 4. 确定初始字体大小
        if max_font_size is None:
            max_font_size = self.max_font_size

        if max_font_size is None:
            # 简单估算
            font_size = min(height, width) // 3
            if len(frequencies) > 10:
                font_size = font_size // 2
        else:
            font_size = max_font_size


        # 5. 逐个放置单词
        font_sizes, positions, orientations, colors = [], [], [], []
        last_freq = 1.0  # 上一个词的频率

        for word, freq in frequencies:
            if freq == 0:
                continue

            # 设置字体大小
            rs = self.relative_scaling
            if rs != 0:
                font_size = int(round((rs * (freq / float(last_freq)) + (1 - rs)) * font_size))
            if random_state.random() < self.prefer_horizontal:
                orientation = None  # 水平
            else:
                orientation = Image.ROTATE_90  # 垂直旋转

            tried_other_orientation = False

            while True:
                if font_size < self.min_font_size:
                    break

                # 尝试找到一个位置放置文字
                font = ImageFont.truetype(self.font_path, font_size)

                # 如果需要，旋转字体
                transposed_font = ImageFont.TransposedFont(font, orientation=orientation)

                # 计算文字在图片中的大小和位置
                box_size = draw.textbbox((0, 0), word, font=transposed_font, anchor="lt")

                # 寻找空闲位置
                result = occupancy.sample_position(box_size[3] + self.margin,
                                                   box_size[2] + self.margin,
                                                   random_state)
                if result is not None:
                    # 找到位置，退出while循环
                    break

                # 没找到位置，先旋转再减小字体
                if not tried_other_orientation and self.prefer_horizontal < 1:
                    orientation = (Image.ROTATE_90 if orientation is None else Image.ROTATE_90)
                    tried_other_orientation = True
                else:
                    font_size -= self.font_step
                    orientation = None

            if font_size < self.min_font_size:
                # 字体太小就不画
                break

            x, y = np.array(result) + self.margin // 2
            draw.text((y, x), word, fill="white", font=transposed_font)

            # 记录信息
            positions.append((x, y))
            orientations.append(orientation)
            font_sizes.append(font_size)
            colors.append(self.color_func(word, font_size=font_size,
                                          position=(x, y),
                                          orientation=orientation,
                                          random_state=random_state,
                                          font_path=self.font_path))

            # 更新占用图
            if self.mask is None:
                img_array = np.asarray(img_grey)
            else:
                img_array = np.asarray(img_grey) + boolean_mask

            occupancy.update(img_array, x, y)
            last_freq = freq  # 用于下一个词的字体大小计算

        self.layout_ = list(zip(frequencies, font_sizes, positions, orientations, colors))
        return self


    def to_image(self):
        if self.mask is not None:
            width = self.mask.shape[1]
            height = self.mask.shape[0]
        else:
            height = self.height
            width = self.width

        img = Image.new(self.mode, (width, height), self.background_color)
        draw = ImageDraw.Draw(img)

        for (word, count), font_size, position, orientation, color in self.layout_:
            # 创建字体
            font = ImageFont.truetype(self.font_path, font_size)

            # 处理旋转
            transposed_font = ImageFont.TransposedFont(font, orientation=orientation)

            x, y = position
            pos = (y, x)  # PIL坐标：（列，行）

            draw.text(pos, word, fill=color, font=transposed_font)

        return self._draw_contour(img=img)


    def to_file(self, filename):
        """导出为图像文件"""

        img = self.to_image()
        img.save(filename, optimize=True)
        return self


    def to_array(self, copy=None):
        """转化为numpy数组"""
        return np.asarray(self.to_image(), copy=copy)


    def __array__(self, copy=None):
        """转化为numpy数组"""
        return self.to_array(copy=copy)


    def _get_boolean_mask(self, mask):
        """把蒙版图片转换成统一的二值布尔掩码"""
        # 检查数据类型
        if mask.dtype.kind == 'f':
            warnings.warn("蒙版图像应该使用0-255的无符号字节整数，但检测到浮点数数组")

        # 灰度图
        if mask.ndim == 2:
            boolean_mask = mask == 255  # 白色区域不能写字

        # 彩色图
        elif mask.ndim == 3:
            # 检查前三个通道（RGB）是否都是255（纯白色）
            # 如果是纯白色，该位置不能绘制文字
            boolean_mask = np.all(mask[:, :, :3] == 255, axis=-1)

        # 处理无效形状
        else:
            mask_shape = str(mask.shape)
            raise ValueError(f"接收到无效的蒙版形状: {mask_shape}")

        return boolean_mask


    def _draw_contour(self, img):
        """在Pillow图像上绘制蒙版轮廓"""
        if self.mask is None or self.contour_width == 0:
            return img

        # 1. 获取蒙版的布尔掩码并转换为0-255图像
        mask = self._get_boolean_mask(self.mask) * 255
        contour = Image.fromarray(mask.astype(np.uint8))

        # 2. 调整蒙版大小与词云图片一致
        contour = contour.resize(img.size)

        # 3. 找出蒙版边界
        contour = contour.filter(ImageFilter.FIND_EDGES)

        # 4. 转换回numpy数组进行处理
        contour = np.array(contour)

        # 5. 清除图片边缘的轮廓线
        contour[[0, -1], :] = 0
        contour[:, [0, -1]] = 0

        # 6. 使用高斯模糊控制轮廓线宽度
        radius = self.contour_width / 10
        contour = Image.fromarray(contour)
        contour = contour.filter(ImageFilter.GaussianBlur(radius=radius))
        contour = np.array(contour) > 0

        # 7. 扩展通道轮廓
        contour = np.dstack((contour, contour, contour))

        # 8. 给轮廓上色
        ret = np.array(img) * np.invert(contour)
        if self.contour_color != 'black':
            color = Image.new(img.mode, img.size, self.contour_color)
            ret += np.array(color) * contour

        return Image.fromarray(ret)
