| Supported Targets | ESP32-P4 |
| ----------------- | -------- |

# MP4 / AVI 播放器示例

[English](./README.md)

本示例使用 ESP32-P4 视频解码器和板级 BSP，将 MicroSD 卡中的 MP4 或 AVI
媒体文件播放到板载 MIPI-DSI 屏幕。

## 使用方法

1. 将兼容的媒体文件复制到 FAT32 格式的 MicroSD 卡中，并插入开发板。
2. 配置媒体文件名：

   ```
   idf.py menuconfig
   ```

   打开 `MP4 Player Configuration > Video File Configuration`，设置
   `Video File Name`（默认 `test_video.mp4`）。文件名必须与卡中文件完全一致，
   包括 `.mp4` 或 `.avi` 扩展名。

3. 在 `Board Support Package (ESP32-P4) > Display > Select LCD type` 中选择
   与实际开发板匹配的屏幕型号。
4. 构建、烧录并监视：

   ```
   idf.py -p PORT build flash monitor
   ```

## 视频格式要求

- MP4 文件必须使用 MJPEG 视频编码，暂不支持 H.264 或 H.265 等其他编码。
- 视频尺寸需要满足编码器对齐要求：
  - YUV420：宽和高都能被 16 整除
  - YUV422：宽能被 16 整除，高能被 8 整除
  - YUV444：宽和高都能被 8 整除
- 使用 LCD 内置缓冲模式时，建议将视频分辨率设置为与所选屏幕分辨率一致，
  以获得最佳效果。

## 使用 FFmpeg 转换视频

```bash
# 将任意视频转换为 MJPEG MP4
ffmpeg -i input.mp4 -c:v mjpeg -c:a aac output.mp4
```

针对板载屏幕的推荐参数：

```bash
# RGB565，720x1280（7 英寸）
ffmpeg -i input.mp4 -c:v mjpeg -q:v 6 -vf scale=720:1280 -r 20 -c:a aac output.mp4
# RGB565，800x1280（8 / 10.1 英寸）
ffmpeg -i input.mp4 -c:v mjpeg -q:v 6 -vf scale=800:1280 -r 20 -c:a aac output.mp4
```

## 常见问题

播放时出现蓝屏闪烁通常是 PSRAM 带宽不足所致。可以尝试使用 RGB565 输出、
降低分辨率或帧率、提高 JPEG 压缩率。不建议播放带音轨的 AVI 文件，
推荐改用 AAC 音轨的 MP4。
