import numpy as np
from PIL import ImageFont

class ImageColorGenerator(object):
    def __init__(self, image, default_color=None):
        if image.ndim != 3 or image.shape[2] not in [3, 4]:
            raise ValueError("需要RGB或RGBA图片")
        self.image = image
        self.default_color = default_color

    def __call__(self, word, font_size, font_path, position, orientation, **kwargs):
        font = ImageFont.truetype(font_path, font_size)
        transposed_font = ImageFont.TransposedFont(font, orientation=orientation)
        box_size = transposed_font.getbbox(word)

        x, y = position

        # 截取图片区域
        patch = self.image[x:x + box_size[2], y:y + box_size[3]]

        if patch.ndim == 3:
            patch = patch[:, :, :3]

        reshape = patch.reshape(-1, 3)
        if not np.all(reshape.shape):
            if self.default_color is None:
                raise ValueError('图像颜色生成器比画布小')
            return "rgb(%d, %d, %d)" % tuple(self.default_color)
        color = np.mean(reshape, axis=0)
        return "rgb(%d, %d, %d)" % tuple(color)
