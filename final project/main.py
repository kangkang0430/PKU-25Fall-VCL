import tkinter as tk
from tkinter import ttk, filedialog, messagebox, scrolledtext
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from mywordcloud import WordCloud, STOPWORDS
from collections import Counter
import numpy as np
from PIL import Image
import re
import os


class WordCloudGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Word Cloud Generator")
        self.root.geometry("1200x800")

        # 初始化变量
        self.wordcloud = None
        self.word_freq = {}
        self.current_mask = None
        self.mask_images = {}

        # 创建界面
        self.create_widgets()

        # 加载预定义蒙版
        self.load_predefined_masks()

        # 加载可用字体
        self.load_available_fonts()

    def load_predefined_masks(self):
        """加载预定义的蒙版图片"""
        masks_dir = "masks"
        if not os.path.exists(masks_dir):
            os.makedirs(masks_dir)

        # 预定义蒙版名称和生成函数
        predefined_masks = {
            "circle": self.create_circle_mask,
            "square": self.create_square_mask,
            "heart": self.create_heart_mask,
            "star": self.create_star_mask,
            "default": None
        }

        # 生成或加载蒙版图片
        for mask_name, generator in predefined_masks.items():
            mask_path = os.path.join(masks_dir, f"{mask_name}.png")

            if mask_name == "default":
                self.mask_images[mask_name] = None

            elif generator:
                # 生成新的蒙版
                img = generator()
                img.save(mask_path)
                self.mask_images[mask_name] = np.array(img)

        # 更新蒙版选择下拉框
        self.mask_combo['values'] = list(self.mask_images.keys())
        self.mask_combo.current(0)

    def create_circle_mask(self, size=400):
        """创建圆形蒙版"""
        img = Image.new('L', (size, size), 255)
        draw = ImageDraw.Draw(img)
        draw.ellipse((10, 10, size - 10, size - 10), fill=0)
        return img

    def create_square_mask(self, size=400):
        """创建方形蒙版"""
        img = Image.new('L', (size, size), 255)
        draw = ImageDraw.Draw(img)
        draw.rectangle((10, 10, size - 10, size - 10), fill=0)
        return img

    def create_heart_mask(self, size=400):
        """创建简单爱心形状"""
        img = Image.new('L', (size, size), 255)
        draw = ImageDraw.Draw(img)

        cx, cy = size // 2, size // 2
        r = size // 3

        # 定义爱心形状的关键点
        points = [
            # 顶部点
            (cx, cy - r),

            # 左上半圆
            (cx - r * 0.9, cy - r * 0.6),
            (cx - r * 1.3, cy - r * 0.2),
            (cx - r * 1.3, cy),

            # 左下角
            (cx - r * 1.1, cy + r * 0.4),
            (cx - r * 0.6, cy + r * 0.9),
            (cx, cy + r * 0.5),

            # 右下角
            (cx + r * 0.6, cy + r * 0.9),
            (cx + r * 1.1, cy + r * 0.4),

            # 右上半圆
            (cx + r * 1.3, cy),
            (cx + r * 1.3, cy - r * 0.2),
            (cx + r * 0.9, cy - r * 0.6),
        ]

        draw.polygon(points, fill=0)

        img = img.rotate(180)

        return img


    def create_star_mask(self, size=400):
        """创建星形蒙版"""
        img = Image.new('L', (size, size), 255)
        draw = ImageDraw.Draw(img)

        # 星形坐标
        points = []
        for i in range(10):
            angle = i * 36
            if i % 2 == 0:
                radius = size // 2 - 20
            else:
                radius = size // 4
            x = size // 2 + radius * np.cos(np.radians(angle))
            y = size // 2 + radius * np.sin(np.radians(angle))
            points.append((x, y))

        draw.polygon(points, fill=0)
        return img


    def create_widgets(self):
        """创建GUI界面组件"""
        # 创建主框架
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.grid(row=0, column=0, sticky="nsew")

        # 左侧控制面板
        control_frame = ttk.LabelFrame(main_frame, text="控制面板", padding="10")
        control_frame.grid(row=0, column=0, sticky="nsw", padx=(0, 10))

        # 右侧显示面板
        display_frame = ttk.LabelFrame(main_frame, text="词云显示", padding="10")
        display_frame.grid(row=0, column=1, sticky="nsew")

        # 配置网格权重
        main_frame.columnconfigure(1, weight=1)
        main_frame.rowconfigure(0, weight=1)
        display_frame.columnconfigure(0, weight=1)
        display_frame.rowconfigure(0, weight=1)


        # ========== 控制面板内容 ==========

        # 1. 文本输入区域
        ttk.Label(control_frame, text="输入文本:").grid(row=0, column=0, sticky=tk.W, pady=(0, 5))

        self.text_input = scrolledtext.ScrolledText(control_frame, width=40, height=10)
        self.text_input.grid(row=1, column=0, columnspan=2, pady=(0, 10))

        # 添加示例文本按钮
        ttk.Button(control_frame, text="加载示例文本",
                   command=self.load_sample_text).grid(row=2, column=0, pady=(0, 10))

        # 2. 文件加载区域
        ttk.Button(control_frame, text="从文件加载",
                   command=self.load_from_file).grid(row=3, column=0, pady=(0, 10))

        # 3. 蒙版选择
        ttk.Label(control_frame, text="选择蒙版:").grid(row=4, column=0, sticky=tk.W, pady=(0, 5))

        self.mask_combo = ttk.Combobox(control_frame, state="readonly", width=20)
        self.mask_combo.grid(row=5, column=0, pady=(0, 10))

        # 自定义蒙版按钮
        ttk.Button(control_frame, text="自定义蒙版...",
                   command=self.load_custom_mask).grid(row=6, column=0, pady=(0, 10))

        # 4. 字体选择
        ttk.Label(control_frame, text="选择字体:").grid(row=7, column=0, sticky=tk.W, pady=(0, 5))

        self.font_combo = ttk.Combobox(control_frame, state="readonly", width=20)
        self.font_combo.grid(row=8, column=0, pady=(0, 10))


        # 5. 颜色方案选择
        ttk.Label(control_frame, text="颜色方案:").grid(row=9, column=0, sticky=tk.W, pady=(0, 5))

        self.color_combo = ttk.Combobox(control_frame, state="readonly", width=20)
        self.color_combo['values'] = ['viridis', 'plasma', 'rainbow', 'cool', 'hot',
                                      'spring', 'summer', 'autumn', 'winter', 'tab20c']
        self.color_combo.current(0)
        self.color_combo.grid(row=10, column=0, pady=(0, 10))

        # 6. 背景颜色选择
        ttk.Label(control_frame, text="背景颜色:").grid(row=11, column=0, sticky=tk.W, pady=(0, 5))

        self.bg_combo = ttk.Combobox(control_frame, state="readonly", width=20)
        self.bg_combo['values'] = ['white', 'black', 'lightblue', 'lightgray',
                                   'darkblue', 'darkgray', '#f0f0f0']
        self.bg_combo.current(0)
        self.bg_combo.grid(row=12, column=0, pady=(0, 10))

        # 7. 其他参数
        params_frame = ttk.LabelFrame(control_frame, text="参数设置", padding="5")
        params_frame.grid(row=13, column=0, columnspan=2, pady=(10, 10), sticky="ew")

        # 最大单词数
        ttk.Label(params_frame, text="最大单词数:").grid(row=0, column=0, sticky=tk.W)
        self.max_words_var = tk.IntVar(value=200)
        ttk.Spinbox(params_frame, from_=10, to=500, textvariable=self.max_words_var,
                    width=10).grid(row=0, column=1, padx=(5, 0))

        # 最小字体大小
        ttk.Label(params_frame, text="最小字体:").grid(row=1, column=0, sticky=tk.W, pady=(5, 0))
        self.min_font_var = tk.IntVar(value=10)
        ttk.Spinbox(params_frame, from_=4, to=50, textvariable=self.min_font_var,
                    width=10).grid(row=1, column=1, padx=(5, 0), pady=(5, 0))

        # 最大字体大小
        ttk.Label(params_frame, text="最大字体:").grid(row=2, column=0, sticky=tk.W, pady=(5, 0))
        self.max_font_var = tk.IntVar(value=100)
        ttk.Spinbox(params_frame, from_=20, to=200, textvariable=self.max_font_var,
                    width=10).grid(row=2, column=1, padx=(5, 0), pady=(5, 0))

        # 8. 操作按钮
        button_frame = ttk.Frame(control_frame)
        button_frame.grid(row=14, column=0, columnspan=2, pady=(10, 0))

        ttk.Button(button_frame, text="生成词云",
                   command=self.generate_wordcloud, style="Accent.TButton").pack(side=tk.LEFT, padx=(0, 5))

        ttk.Button(button_frame, text="保存图片",
                   command=self.save_image).pack(side=tk.LEFT)

        ttk.Button(button_frame, text="显示统计",
                   command=self.show_statistics).pack(side=tk.LEFT, padx=(5, 0))

        # 9. 状态栏
        self.status_label = ttk.Label(control_frame, text="就绪")
        self.status_label.grid(row=15, column=0, columnspan=2, pady=(10, 0))



        # ========== 显示面板内容 ==========

        # 创建Matplotlib图形
        self.fig, self.ax = plt.subplots(figsize=(8, 6))
        self.canvas = FigureCanvasTkAgg(self.fig, master=display_frame)
        self.canvas.get_tk_widget().grid(row=0, column=0, sticky=(tk.N, tk.S, tk.E, tk.W))

        # 初始显示占位文本
        self.ax.text(0.5, 0.5, "Waiting for the word cloud to be generated...",
                     ha='center', va='center', fontsize=16, color='gray')
        self.ax.axis('off')
        self.canvas.draw()

        # 设置样式
        self.setup_styles()

    def setup_styles(self):
        """设置界面样式"""
        style = ttk.Style()
        style.configure("Accent.TButton", font=('Arial', 12, 'bold'))

    def load_sample_text(self):
        """加载示例文本"""
        sample_text = """Python is a powerful programming language that is widely used in data science,
        machine learning, web development, and automation. Word clouds are fun and
        informative visualizations that can help understand text data quickly.
        Data science involves statistics, programming, and domain knowledge to extract
        insights from data. Machine learning algorithms can learn patterns from data
        and make predictions. Python libraries like NumPy, Pandas, and Scikit-learn
        are essential tools for data analysis and machine learning projects."""

        self.text_input.delete(1.0, tk.END)
        self.text_input.insert(1.0, sample_text)
        self.status_label.config(text="示例文本已加载")

    def load_from_file(self):
        """从文件加载文本"""
        filepath = filedialog.askopenfilename(
            title="选择文本文件",
            filetypes=[("Text files", "*.txt"), ("All files", "*.*")]
        )

        if filepath:
            try:
                with open(filepath, 'r', encoding='utf-8') as f:
                    content = f.read()

                self.text_input.delete(1.0, tk.END)
                self.text_input.insert(1.0, content)
                self.status_label.config(text=f"已加载文件: {os.path.basename(filepath)}")
            except Exception as e:
                messagebox.showerror("错误", f"无法读取文件: {e}")

    def load_custom_mask(self):
        """加载自定义蒙版图片"""
        filepath = filedialog.askopenfilename(
            title="选择蒙版图片",
            filetypes=[("Image files", "*.png *.jpg *.jpeg *.bmp"), ("All files", "*.*")]
        )

        if filepath:
            try:
                img = Image.open(filepath)
                mask_array = np.array(img)

                # 添加到蒙版列表
                mask_name = os.path.basename(filepath)
                self.mask_images[mask_name] = mask_array

                # 更新下拉框
                current_values = list(self.mask_combo['values'])
                current_values.append(mask_name)
                self.mask_combo['values'] = current_values
                self.mask_combo.set(mask_name)

                self.status_label.config(text=f"自定义蒙版已加载: {mask_name}")
            except Exception as e:
                messagebox.showerror("错误", f"无法加载图片: {e}")

    def load_available_fonts(self):
        """加载可用字体列表"""
        import matplotlib.font_manager as fm
        import os

        fonts = ["默认字体 (DroidSansMono)"]

        # 查找系统字体
        system_fonts = fm.findSystemFonts()

        # 常见字体列表（按优先级）
        common_fonts = [
            "arial", "times", "helvetica", "verdana",
            "courier", "comic", "impact", "tahoma",
            "trebuchet", "palatino", "garamond"
        ]

        # 添加常见字体
        for font_name in common_fonts:
            for font_path in system_fonts:
                font_file = os.path.basename(font_path).lower()
                if font_name in font_file and font_path.endswith(('.ttf', '.otf')):
                    display_name = f"{font_name.title()}"
                    if display_name not in fonts:
                        fonts.append(display_name)
                    break

        # 限制字体数量，避免列表太长
        fonts = fonts[:15]  # 最多显示15种字体

        # 更新下拉框
        self.font_combo['values'] = fonts
        self.font_combo.current(0)

        # 保存字体路径映射
        self.font_paths = {}
        for font_display in fonts[1:]:  # 跳过第一个（默认字体）
            font_name = font_display.split(" (")[0].lower()
            for font_path in system_fonts:
                if font_name in os.path.basename(font_path).lower():
                    self.font_paths[font_display] = font_path
                    break

    def get_selected_font_path(self):
        """获取选中的字体路径"""
        selection = self.font_combo.get()

        if not selection or "默认字体" in selection:
            # 使用默认字体
            default_font = os.path.join(
                os.path.dirname(__file__),
                "mywordcloud",
                "DroidSansMono.ttf"
            )
            if os.path.exists(default_font):
                return default_font
            return None  # 让WordCloud使用自己的默认

        # 从映射中获取字体路径
        if selection in self.font_paths:
            return self.font_paths[selection]

        return None

    def process_text(self, text):
        """处理文本，统计词频"""
        # 获取停用词
        all_stopwords = set(STOPWORDS)

        # 转换为小写并分割单词
        words = text.lower().split()

        # 清理单词：移除标点符号
        cleaned_words = []
        for word in words:
            clean_word = re.sub(r'^[^a-zA-Z]+|[^a-zA-Z]+$', '', word)
            if clean_word:
                cleaned_words.append(clean_word)

        # 过滤停用词和短词
        filtered_words = [
            word for word in cleaned_words
            if word not in all_stopwords and len(word) > 2
        ]

        # 统计词频
        word_counts = Counter(filtered_words)
        return dict(word_counts)

    def generate_wordcloud(self):
        """生成词云"""
        # 获取输入文本
        text = self.text_input.get(1.0, tk.END).strip()

        if not text:
            messagebox.showwarning("警告", "请输入文本！")
            return

        try:
            self.status_label.config(text="正在处理文本...")
            self.root.update()

            # 处理文本
            self.word_freq = self.process_text(text)

            if not self.word_freq:
                messagebox.showwarning("警告", "没有有效的单词可以生成词云！")
                return

            # 获取选择的蒙版
            selected_mask = self.mask_combo.get()
            if selected_mask in self.mask_images:
                mask = self.mask_images[selected_mask]

            else:
                mask = None

            # 获取其他参数
            max_words = self.max_words_var.get()
            colormap = self.color_combo.get()
            background_color = self.bg_combo.get()

            self.status_label.config(text="正在生成词云...")
            self.root.update()

            font_path = self.get_selected_font_path()

            # 生成词云
            wordcloud = WordCloud(
                font_path = font_path,
                width=800,
                height=600,
                background_color=background_color,
                max_words=max_words,
                mask=mask,
                colormap=colormap,
                min_font_size=self.min_font_var.get(),
                max_font_size=self.max_font_var.get()
            )

            wordcloud.generate_from_frequencies(self.word_freq)
            self.wordcloud = wordcloud

            # 更新显示
            self.update_display(wordcloud)

            self.status_label.config(text="词云生成成功！")

        except Exception as e:
            messagebox.showerror("错误", f"生成词云时出错: {e}")
            self.status_label.config(text="生成失败")

    def update_display(self, wordcloud):
        """更新显示区域的词云图像"""
        self.ax.clear()
        self.ax.imshow(wordcloud, interpolation='bilinear')
        self.ax.axis('off')

        # 获取当前蒙版名称
        mask_name = self.mask_combo.get()
        if mask_name and mask_name != "无蒙版":
            self.ax.set_title(f"wordcloud - {mask_name}shape", fontsize=14, pad=20)
        else:
            self.ax.set_title("wordcloud", fontsize=14, pad=20)

        self.canvas.draw()

    def save_image(self):
        """保存词云图片"""
        if self.wordcloud is None:
            messagebox.showwarning("警告", "请先生成词云！")
            return

        filepath = filedialog.asksaveasfilename(
            title="保存词云图片",
            defaultextension=".png",
            filetypes=[("PNG files", "*.png"), ("JPEG files", "*.jpg"), ("All files", "*.*")]
        )

        if filepath:
            try:
                self.wordcloud.to_file(filepath)
                messagebox.showinfo("成功", f"词云已保存到:\n{filepath}")
                self.status_label.config(text=f"已保存: {os.path.basename(filepath)}")
            except Exception as e:
                messagebox.showerror("错误", f"保存失败: {e}")

    def show_statistics(self):
        """显示词频统计"""
        if not self.word_freq:
            messagebox.showwarning("警告", "请先生成词云！")
            return

        # 创建统计窗口
        stats_window = tk.Toplevel(self.root)
        stats_window.title("词频统计")
        stats_window.geometry("400x500")

        # 添加文本框显示统计
        text_frame = ttk.Frame(stats_window, padding="10")
        text_frame.pack(fill=tk.BOTH, expand=True)

        text_widget = scrolledtext.ScrolledText(text_frame, width=50, height=30)
        text_widget.pack(fill=tk.BOTH, expand=True)

        # 排序词频
        sorted_words = sorted(self.word_freq.items(), key=lambda x: x[1], reverse=True)

        # 添加统计信息
        text_widget.insert(tk.END, "词频统计\n")
        text_widget.insert(tk.END, "=" * 30 + "\n\n")
        text_widget.insert(tk.END, f"总单词数: {len(self.word_freq)}\n\n")

        text_widget.insert(tk.END, "前50个高频词:\n")
        text_widget.insert(tk.END, "-" * 30 + "\n")

        for i, (word, freq) in enumerate(sorted_words[:50], 1):
            text_widget.insert(tk.END, f"{i:2d}. {word:20s} : {freq}\n")

        text_widget.config(state=tk.DISABLED)  # 设置为只读


def main():
    try:
        root = tk.Tk()
        app = WordCloudGUI(root)

        def on_closing():
            """处理窗口关闭事件"""

            try:
                # 1. 关闭所有matplotlib图形
                plt.close('all')

                # 2. 清理可能的资源
                if hasattr(app, 'wordcloud'):
                    del app.wordcloud
                if hasattr(app, 'mask_images'):
                    app.mask_images.clear()

                # 3. 销毁窗口
                root.quit()  # 停止mainloop
                root.destroy()  # 销毁窗口

            except Exception as e:
                print(f"Error during closing: {e}")
            finally:
                # 4. 确保程序终止
                import sys
                sys.exit(0)

        # 绑定关闭事件
        root.protocol("WM_DELETE_WINDOW", on_closing)

        # 启动主循环
        root.mainloop()

    except Exception as e:
        print(f"Application error: {e}")
        import sys
        sys.exit(1)


if __name__ == "__main__":
    # 确保导入必要的库
    try:
        from PIL import ImageDraw
        import matplotlib

        matplotlib.use('TkAgg')  # 设置matplotlib后端
        main()
    except ImportError as e:
        print(f"缺少必要的库: {e}")
        print("请安装: pip install pillow matplotlib wordcloud numpy")