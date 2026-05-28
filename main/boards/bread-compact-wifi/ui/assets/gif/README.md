# 启动页 GIF 背景

| 文件 | 说明 |
|------|------|
| `../mp4/splash_bg.mp4` | 原始视频（不编入固件） |
| `splash_bg.gif` | 压缩后的 GIF（约 130KB） |
| `gif_to_c.py` | 生成 `../images/splash_bg_gif.c` |

## 压缩参数（当前）

- 480×320 屏：GIF 为 **160×107**，运行时约 3× 放大
- **6 fps**，取前 **3 秒** 循环
- **24 色调色板**（约 34KB 固件占用）

## 重新生成 GIF

```bash
ffmpeg -y -t 3 -i ../mp4/splash_bg.mp4 -vf "fps=6,scale=160:107:flags=lanczos,palettegen=max_colors=24:stats_mode=diff" palette.png
ffmpeg -y -t 3 -i ../mp4/splash_bg.mp4 -i palette.png -lavfi "fps=6,scale=160:107:flags=lanczos[x];[x][1:v]paletteuse=dither=bayer:bayer_scale=5" -loop 0 splash_bg.gif
python gif_to_c.py
```

改完 GIF 后务必运行 `gif_to_c.py` 再编译。
