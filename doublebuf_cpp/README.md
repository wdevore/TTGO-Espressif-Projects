# TFT eSPI
I am current using. Jeremy's library:
https://github.com/jeremyjh/ESP32_TFT_library

Notes:
Double buffer looks like you populate a "**color_t *buf**" and then call *tftspi.c*' **send_data**(), where x1,y1 are always 0 and x2,y2 are width and height.

Use *TFT_bmp_image*() as an example.

The buffer is a "stretched" out 2D array into a 1D array of *uint8_t*. The width and height tell the display how to "cut" the 1D array into pixels lines. For double-buffer the width and height **IS** the width and height of the display.


# How the project was built

1) Create new project using: ```> idf.py create-project doublebuf```.
2) Create *components* folder.
3) Copy Jeremy's "**components/spidriver**" and "**components/tft**" into root of the *components* folder.
4) Run ```idf.py menuconfig``` and select "*component config -> TFT Display -> TTGO T-DISPLAY (ST7789V)*"

## directory structure
```
doublebuf
  components
    spidriver
    tft
  main
    doublebuf.c
```



