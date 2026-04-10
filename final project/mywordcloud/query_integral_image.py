def query_integral_image(integral_image, size_x, size_y, random_state):
    x = integral_image.shape[0]
    y = integral_image.shape[1]
    hits = 0

    # 遍历图像中所有可能放置单词的位置
    for i in range(x - size_x):
        for j in range(y - size_y):
            # 计算当前位置的积分面积
            area = (integral_image[i, j] + integral_image[i + size_x, j + size_y] -
                    integral_image[i + size_x, j] - integral_image[i, j + size_y])
            # 如果面积为0，表示该区域没有像素
            if not area:
                hits += 1

    if not hits:
        return None

    # 随机选择一个位置
    goal = random_state.randint(0, hits)
    hits = 0
    for i in range(x - size_x):
        for j in range(y - size_y):
            area = (integral_image[i, j] + integral_image[i + size_x, j + size_y] -
                    integral_image[i + size_x, j] - integral_image[i, j + size_y])
            if not area:
                hits += 1
                if hits == goal:
                    return i, j