| Supported Targets | ESP32-P4 |
| ----------------- | -------- |

# MP4 / AVI Player

[中文版本](./README_ZH.md)

This example plays MP4 or AVI media files from a MicroSD card on the onboard
MIPI-DSI LCD using the ESP32-P4 video decoder and the board BSP.

## How to Use the Example

1. Copy a compatible media file to a FAT32-formatted MicroSD card and insert
   the card into the board.
2. Configure the media file name:

   ```
   idf.py menuconfig
   ```

   Open `MP4 Player Configuration > Video File Configuration` and set
   `Video File Name` (default `test_video.mp4`). The name must match exactly,
   including the `.mp4` or `.avi` extension.

3. Select the LCD panel that matches your board under
   `Board Support Package (ESP32-P4) > Display > Select LCD type`.
4. Build, flash, and monitor:

   ```
   idf.py -p PORT build flash monitor
   ```

## Video Format Requirements

- MP4 files must use MJPEG video encoding. Other codecs such as H.264 or
  H.265 are not supported.
- Keep video dimensions aligned to the codec requirements:
  - YUV420: width and height divisible by 16
  - YUV422: width divisible by 16, height divisible by 8
  - YUV444: width and height divisible by 8
- When using the LCD internal buffer mode, match the video resolution to the
  selected panel resolution for best results.

## Video Conversion with FFmpeg

```bash
# Convert any video to MJPEG MP4
ffmpeg -i input.mp4 -c:v mjpeg -c:a aac output.mp4
```

Recommended settings for the onboard panels:

```bash
# RGB565, 720x1280 (7-inch)
ffmpeg -i input.mp4 -c:v mjpeg -q:v 6 -vf scale=720:1280 -r 20 -c:a aac output.mp4
# RGB565, 800x1280 (8 / 10.1-inch)
ffmpeg -i input.mp4 -c:v mjpeg -q:v 6 -vf scale=800:1280 -r 20 -c:a aac output.mp4
```

## Troubleshooting

Blue-screen flickering during playback is usually caused by insufficient
PSRAM bandwidth. Try RGB565 output, a lower resolution, a lower frame rate,
or higher JPEG compression. AVI files with audio are not recommended;
prefer MP4 with AAC audio instead.
