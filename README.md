# Smartwatch firmware

This is a structural refactor of the current LVGL smartwatch firmware.

The existing LVGL, GC9A01, Wi-Fi/SNTP, button, page, menu, and app behavior is
kept intact. The refactor only moves responsibilities out of `main.c`.

## Structure

```text
.
├── main/
│   ├── CMakeLists.txt
│   └── main.c
├── include/
│   ├── display.h
│   ├── network.h
│   ├── specs.h
│   └── ui.h
└── src/
    ├── display.c
    ├── network.c
    └── ui.c
```

`main.c` is now only the application entry point. The display module owns the
GC9A01 + LVGL port initialization, the network module owns Wi-Fi/SNTP, and the
UI module owns the existing LVGL screens and navigation.

No new ESP-IDF components are introduced.
