| Supported Targets | ESP32-P4 |
| ----------------- | -------- |

# I2S Codec Example

[中文版本](./README_ZH.md)

This example initializes the onboard ES8311 codec and the ESP32-P4 I2S
peripheral. The default `music` mode repeatedly plays the embedded
`canon.pcm` sample. The optional `echo` mode reads codec input samples and
writes them back to the output.

## Board Connections

| Function | GPIO |
| --- | ---: |
| I2C SCL / SDA | 8 / 7 |
| I2S MCLK / BCLK / LRCK | 13 / 12 / 10 |
| I2S data out / data in | 9 / 11 |
| Power-amplifier enable | 53 |

These assignments match the board schematic. The default direct-hardware path
should be used for this ESP32-P4 product; the optional generic BSP setting is
disabled unless a compatible BSP dependency is selected deliberately.

## Configuration and Use

`Example Configuration` selects `music` or `echo` mode, microphone gain, and
output volume. Start with a moderate volume and keep the speaker and
microphone path clear of acoustic feedback.

```text
idf.py set-target esp32p4
idf.py menuconfig
idf.py build
idf.py -p PORT flash monitor
```
