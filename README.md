# GC9A01 ESP-IDF Driver

Minimal, self-contained ESP-IDF driver for the GC9A01 240x240 round SPI LCD
(the display featured in the DroneBot Workshop GC9A01 tutorial).

## Wiring (default GPIOs — edit in main/gc9a01.h if different)

| Display Pin | ESP32 GPIO |
|---|---|
| VCC   | 3.3V |
| GND   | GND  |
| SCL/SCK | 18 |
| SDA/MOSI | 23 |
| RES   | 4    |
| DC    | 2    |
| CS    | 5    |
| BLK   | 15 (or tie to 3.3V) |

## Build & Flash

```bash
. $IDF_PATH/export.sh
idf.py set-target esp32
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

## Files

- `main/gc9a01.h` / `main/gc9a01.c` — the display driver (init, fill_screen,
  draw_pixel, set_window, push_colors)
- `main/main.c` — example app: cycles through solid colors, then draws a
  diagonal line
- `main/CMakeLists.txt`, `CMakeLists.txt` — standard ESP-IDF project build files

## Notes

- SPI clock defaults to 40 MHz. Drop to 20-26 MHz if you see glitches on
  long wiring.
- For fast full-frame updates, build one big frame buffer
  (240*240*2 = 115200 bytes) and push it in a single DMA transaction.
- For real UI/graphics (text, shapes, images) consider pairing this SPI
  driver with LVGL, using this code inside LVGL's flush callback.
