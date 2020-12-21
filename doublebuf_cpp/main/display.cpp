#include <iostream>

#include "freertos/FreeRTOS.h"

#include "../includes/display.h"

// ==========================================================
// Define which spi bus to use TFT_VSPI_HOST or TFT_HSPI_HOST
#define SPI_BUS TFT_HSPI_HOST
// ==========================================================

void Display::init(void)
{
  // ========  PREPARE DISPLAY INITIALIZATION  =========

  esp_err_t ret;

  // === SET GLOBAL VARIABLES ==========================

  // ===================================================
  // ==== Set maximum spi clock for display read    ====
  //      operations, function 'find_rd_speed()'    ====
  //      can be used after display initialization  ====
  tft_max_rdclock = 8000000;
  // ===================================================

  // ====================================================================
  // === Pins MUST be initialized before SPI interface initialization ===
  // ====================================================================
  TFT_PinsInit();

  // ===================================================
  // ====  CONFIGURE SPI DEVICES(s)  ===================
  // ===================================================
  spi_lobo_device_handle_t spi;

  spi_lobo_bus_config_t buscfg = {
      .mosi_io_num = PIN_NUM_MOSI, // set SPI MOSI pin
      .miso_io_num = PIN_NUM_MISO, // set SPI MISO pin
      .sclk_io_num = PIN_NUM_CLK,  // set SPI CLK pin
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = 6 * 1024,
  };

  spi_lobo_device_interface_config_t devcfg = {
      .command_bits = 0,
      .address_bits = 0,
      .dummy_bits = 0,
      .mode = 0, // SPI mode 0
      .duty_cycle_pos = 0,
      .cs_ena_pretrans = 0,
      .cs_ena_posttrans = 0,
      .clock_speed_hz = 26000000,        // Initial clock out at 26 MHz
      .spics_io_num = -1,                // we will use external CS pin
      .spics_ext_io_num = PIN_NUM_CS,    // external CS pin
      .flags = LB_SPI_DEVICE_HALFDUPLEX, // ALWAYS SET to HALF DUPLEX MODE!! for spi displays
      .pre_cb = nullptr,
      .post_cb = nullptr,
      .selected = 0,
  };

  vTaskDelay(500 / portTICK_RATE_MS);
  std::cout << std::endl
            << "==============================" << std::endl;
  std::cout << "TFT display Drawline 12/2020" << std::endl;
  std::cout << "==============================" << std::endl;
  std::cout << "Pins used: miso=" << PIN_NUM_MISO << ", mosi=" << PIN_NUM_MOSI << ", sck=" << PIN_NUM_CLK << ", cs=" << PIN_NUM_CS << std::endl;

  // ==================================================================
  // ==== Initialize the SPI bus and attach the LCD to the SPI bus ====
  // ==================================================================

  ret = spi_lobo_bus_add_device(SPI_BUS, &buscfg, &devcfg, &spi);
  assert(ret == ESP_OK);
  std::cout << "SPI: display device added to spi bus (" << SPI_BUS << ")" << std::endl;
  tft_disp_spi = spi;

  // ==== Test select/deselect ====
  ret = spi_lobo_device_select(spi, 1);
  assert(ret == ESP_OK);
  ret = spi_lobo_device_deselect(spi);
  assert(ret == ESP_OK);

  std::cout << "SPI: attached display device, speed=" << spi_lobo_get_speed(spi) << std::endl;
  std::cout << "SPI: bus uses native pins: " << (spi_lobo_uses_native_pins(spi) ? "true" : "false") << std::endl;

  // ================================
  // ==== Initialize the Display ====
  // ================================
  std::cout << "SPI: display init.." << std::endl;
  TFT_display_init();

  width = tft_width;
  height = tft_height;

  std::cout << "Display dimensions: " << tft_width << "x" << tft_height << std::endl;
  std::cout << "TFT Static offsets: ";
  std::cout << "x1: " << tft_dispWin.x1;
  std::cout << ", y1: " << tft_dispWin.y1;
  std::cout << ", x2: " << tft_dispWin.x2;
  std::cout << ", y2: " << tft_dispWin.y2 << std::endl;

  // ---- Detect maximum read speed ----
  tft_max_rdclock = find_rd_speed();
  std::cout << "SPI: Max rd speed = " << tft_max_rdclock << std::endl;

  // ==== Set SPI clock used for display operations ====
  spi_lobo_set_speed(spi, 26666666); // Max is 26.6MHz
  std::cout << "SPI: Changed speed to " << spi_lobo_get_speed(spi) << std::endl;

  TFT_invertDisplay(1);

  tft_font_rotate = 0;
  tft_text_wrap = 0;
  tft_font_transparent = 0;
  tft_font_forceFixed = 0;
  tft_gray_scale = 0;

  TFT_setGammaCurve(DEFAULT_GAMMA_CURVE);
  TFT_setRotation(PORTRAIT); // Note: Landscape may have a display pixel failure.
  TFT_setFont(DEFAULT_FONT, NULL);
}

void Display::drawHeader(std::string text)
{
  TFT_resetclipwin();

  tft_fg = TFT_YELLOW;
  // tft_bg = (color_t){64, 64, 64};
  tft_bg = TFT_BLACK;

  if (tft_width < 240)
    TFT_setFont(DEF_SMALL_FONT, NULL);
  else
    TFT_setFont(DEFAULT_FONT, NULL);

  TFT_fillRect(0, 0, tft_width - 1, TFT_getfontheight() + 8, tft_bg);
  TFT_drawRect(0, 0, tft_width - 1, TFT_getfontheight() + 8, TFT_CYAN);

  // TFT_fillRect(0, tft_height - TFT_getfontheight() - 9, tft_width - 1, TFT_getfontheight() + 8, tft_bg);
  // TFT_drawRect(0, tft_height - TFT_getfontheight() - 9, tft_width - 1, TFT_getfontheight() + 8, TFT_CYAN);

  TFT_print(text.c_str(), CENTER, 4);
}

void Display::drawSplash(void)
{
  drawHeader("Draw line DEMO");

  TFT_setclipwin(0, TFT_getfontheight() + 9, tft_width - 1, tft_height - TFT_getfontheight() - 10);

  TFT_setFont(COMIC24_FONT, NULL);
  int tempy = TFT_getfontheight() + 4;

  tft_fg = TFT_ORANGE;
  TFT_print("ESP32", CENTER, (tft_dispWin.y2 - tft_dispWin.y1) / 2 - tempy);
  TFT_setFont(UBUNTU16_FONT, NULL);

  tft_fg = TFT_CYAN;
  TFT_print("Demo", CENTER, LASTY + tempy);
}

void Display::reset(void)
{
  TFT_resetclipwin();

  // Set font attributes
  tft_image_debug = 0;
  tft_bg = TFT_BLACK;

  std::cout << "color_t size: " << sizeof(color_t) << std::endl;

  // Allocate back buffer for bliting to display
  width = 30;
  height = 30;
  bufArea = width * height;
  bufSize = bufArea * sizeof(color_t);

  backBuf = (color_t *)heap_caps_malloc(bufSize, MALLOC_CAP_DMA);

  if (backBuf == NULL)
  {
    std::cout << "Unable to allocate backbuffer" << std::endl;
    exit(10);
  }

  std::cout << "Allocated back buffer area: " << bufArea << std::endl;
  std::cout << "Allocated back buffer of size: " << bufSize << std::endl;
}

void Display::setClearColor(color_t color)
{
  clearColor = color;
}

void Display::clear()
{
  // TFT_fillScreen(clearColor);

  for (int i = 0; i < bufArea; i++)
  {
    backBuf[i].r = clearColor.r;
    backBuf[i].g = clearColor.g;
    backBuf[i].b = clearColor.b;
  }
  // memset(buf, 0, len*sizeof(color_t));
}

void Display::setDrawColor(color_t color)
{
  drawColor = color;
}

void Display::setDrawColor(uint8_t r, uint8_t g, uint8_t b)
{
  drawColor = {r, g, b};
}

void Display::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
  TFT_drawLine(x0, y0, x1, y1, drawColor); // Direct write api
}

void Display::blit(void)
{
  if (disp_select() != ESP_OK)
    return;

  std::cout << "Sending data..." << std::endl;

  int x = 10;
  int y = 100;
  for (int i = 0; i < bufArea; i++)
  {
    backBuf[i].r = 255;
    backBuf[i].g = 0;
    backBuf[i].b = 0;
  }

  send_data(x + tft_dispWin.x1, y + tft_dispWin.y1, x + width + tft_dispWin.x1, y + height + tft_dispWin.y1, bufArea, backBuf);
  disp_deselect();

  // if (disp_select() != ESP_OK)
  //   return;

  // for (int i = 0; i < bufArea; i++)
  // {
  //   backBuf[i].r = 0;
  //   backBuf[i].g = 0;
  //   backBuf[i].b = 255;
  // }

  // y = 135;
  // send_data(x + tft_dispWin.x1, y + tft_dispWin.y1, x + width + tft_dispWin.x1, y + height + tft_dispWin.y1, bufArea, backBuf);
  // std::cout << "Sent" << std::endl;

  // disp_deselect();
}