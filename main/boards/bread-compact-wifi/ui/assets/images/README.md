# LVGL 图片资源（已转换）

由 `scripts/Image_Converter/LVGLImage.py` 从 `../png/` 生成，格式 **RGB565A8**，保持原始分辨率。

| 源 PNG | 尺寸 | 输出 .c | 用途 |
|--------|------|---------|------|
| `btn_pad_top.png` | 148×144（转换时缩放） | `btn_pad_top.c` | 圆形点动底盘 |
| `arrow_up.png` | 32×32 | `arrow_up.c` | Y+（上） |
| `arrow_down.png` | 32×32 | `arrow_down.c` | Y-（下） |
| `arrow_left.png` | 32×32 | `arrow_left.c` | X-（左） |
| `arrow_right.png` | 32×32 | `arrow_right.c` | X+（右） |

声明见 `../laser_ui_images.h`。重新转换示例：

```bash
cd scripts/Image_Converter
python LVGLImage.py --ofmt C --cf RGB565A8 -o ../../main/boards/esp32s3-msp3525-lcd-3.5/ui/assets/images ../../main/boards/esp32s3-msp3525-lcd-3.5/ui/assets/png
```

`main/CMakeLists.txt` 在 `CONFIG_MSP3525_LASER_UI` 下会自动编入本目录 `*.c`。
