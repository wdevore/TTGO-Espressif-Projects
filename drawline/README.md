# TFT eSPI
I am current using. Jeremy's library:
https://github.com/jeremyjh/ESP32_TFT_library

Notes:
Double buffer looks like you populate a "**color_t *buf**" and then call *tftspi.c*' **send_data**(), where x1,y1 are always 0 and x2,y2 are width and height.

Use *TFT_bmp_image*() as an example.

The buffer is a "stretched" out 2D array into a 1D array. The width and height tell the display how to "cut" the 1D array into pixels lines. For double-buffer the width and height **IS** the width and height of the display.

-------------------------------------------------------------------------------
Everything below is old research.
------------------------------

```
https://github.com/loboris/ESP32_NEW_SPI_MASTER_EXAMPLE
Merge above with
ST7789_x.h files

TFT_eSPI.h is from the Adafruit Arduino library
~Arduino/libraries/TFT_eSPI/.h,.cpp

Pure eps32
https://github.com/loboris/ESP32_TFT_library

```

# ST7789 and *_2_* variant

## Files from ~Arduino/libraries/TFT_eSPI/TFT_Drivers
* ST7789_Init.h
* ST7789_Defines.h
* ST7789_Rotation.h

## Initialize
* Command sequence to initialize display: ST7789_2_init.h

# Notes
Hardware/Esp32/TFT/ESP32_TFT_library/components/tft/tft.c has triangle filling.
A demo of ESP32_TFT_library is also available in main/tft_demo.c

In Setup25_TTGO_T_Display.h:
#define SPI_READ_FREQUENCY  6000000 // 6 MHz is the maximum SPI read speed for the ST7789V

## Dependencies
```
TFT_eSPI.h
  SPI.H    <- .arduino15/packages/esp32/hardware/esp32/1.0.4/libraries/SPI/src
    <stdlib.h>
    pins_arduino.h <- .arduino15/.../esp32/1.0.4/variants/ttgo-t1/
      variant.h
        WVariant.h
    esp32-hal-spi.h
  User_Setup_Select.h
  pgmspace.h
  Processors/TFT_eSPI_ESP32.h
  Fonts/...

TFT_eSPI.cpp
  Processors/TFT_eSPI_ESP32.c

```



