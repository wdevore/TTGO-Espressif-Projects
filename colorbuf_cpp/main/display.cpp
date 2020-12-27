#include <iostream>
#include <cstring>

#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"

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

void Display::reset(void)
{
  TFT_resetclipwin();

  // Set font attributes
  tft_image_debug = 0;
  tft_bg = TFT_BLACK;

  // -----------------------------------------------------------
  // Allocate color buffer for rendering to.
  // -----------------------------------------------------------
  colorBufArea = width * height;
  colorBufSize = colorBufArea * sizeof(color_t);
  colorBuf = (color_t *)heap_caps_malloc(colorBufSize, MALLOC_CAP_DMA);

  if (colorBuf == NULL)
  {
    std::cout << "Unable to allocate color buffer" << std::endl;
    exit(10);
  }

  std::cout << "Color buffer area: " << colorBufArea << std::endl;
  std::cout << "Color buffer size: " << colorBufSize << std::endl;
}

int Display::Width()
{
  return width;
}

int Display::Height()
{
  return height;
}

void Display::setClearColor(color_t color)
{
  clearColor = color;
}

void Display::clear()
{
  for (int i = 0; i <= colorBufArea; i++)
  {
    colorBuf[i] = clearColor;
  }
}

void Display::setDrawColor(color_t color)
{
  drawColor = color;
}

void Display::setDrawColor(uint8_t r, uint8_t g, uint8_t b)
{
  drawColor = {r, g, b};
}

void Display::setAddrWindow(uint16_t x1, uint16_t x2, uint16_t y1, uint16_t y2)
{
  uint32_t wd;

  // taskDISABLE_INTERRUPTS();
  // Wait for SPI bus ready
  while (tft_disp_spi->host->hw->cmd.usr)
    ;
  gpio_set_level((gpio_num_t)PIN_NUM_DC, 0);

  tft_disp_spi->host->hw->data_buf[0] = (uint32_t)TFT_CASET;
  tft_disp_spi->host->hw->user.usr_mosi_highpart = 0;
  tft_disp_spi->host->hw->mosi_dlen.usr_mosi_dbitlen = 7;
  tft_disp_spi->host->hw->user.usr_mosi = 1;
  tft_disp_spi->host->hw->miso_dlen.usr_miso_dbitlen = 0;
  tft_disp_spi->host->hw->user.usr_miso = 0;

  tft_disp_spi->host->hw->cmd.usr = 1; // Start transfer

  wd = (uint32_t)(x1 >> 8);
  wd |= (uint32_t)(x1 & 0xff) << 8;
  wd |= (uint32_t)(x2 >> 8) << 16;
  wd |= (uint32_t)(x2 & 0xff) << 24;

  while (tft_disp_spi->host->hw->cmd.usr)
    ; // wait transfer end
  gpio_set_level((gpio_num_t)PIN_NUM_DC, 1);
  tft_disp_spi->host->hw->data_buf[0] = wd;
  tft_disp_spi->host->hw->mosi_dlen.usr_mosi_dbitlen = 31;
  tft_disp_spi->host->hw->cmd.usr = 1; // Start transfer

  while (tft_disp_spi->host->hw->cmd.usr)
    ;
  gpio_set_level((gpio_num_t)PIN_NUM_DC, 0);
  tft_disp_spi->host->hw->data_buf[0] = (uint32_t)TFT_PASET;
  tft_disp_spi->host->hw->mosi_dlen.usr_mosi_dbitlen = 7;
  tft_disp_spi->host->hw->cmd.usr = 1; // Start transfer

  wd = (uint32_t)(y1 >> 8);
  wd |= (uint32_t)(y1 & 0xff) << 8;
  wd |= (uint32_t)(y2 >> 8) << 16;
  wd |= (uint32_t)(y2 & 0xff) << 24;

  while (tft_disp_spi->host->hw->cmd.usr)
    ;
  gpio_set_level((gpio_num_t)PIN_NUM_DC, 1);

  tft_disp_spi->host->hw->data_buf[0] = wd;
  tft_disp_spi->host->hw->mosi_dlen.usr_mosi_dbitlen = 31;
  tft_disp_spi->host->hw->cmd.usr = 1; // Start transfer
  while (tft_disp_spi->host->hw->cmd.usr)
    ;
  // taskENABLE_INTERRUPTS();
}

void Display::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
  TFT_drawLine(x0, y0, x1, y1, drawColor); // Direct write api
}

void Display::drawPixel(int16_t x, int16_t y)
{
  if (disp_select() != ESP_OK)
    return;

  uint32_t wd = 0;

  taskDISABLE_INTERRUPTS();

  x += tft_dispWin.x1;
  y += tft_dispWin.y1;

  setAddrWindow(x, x + 1, y, y + 1);

  // Send RAM WRITE command
  gpio_set_level((gpio_num_t)PIN_NUM_DC, 0);
  tft_disp_spi->host->hw->data_buf[0] = (uint32_t)TFT_RAMWR;
  tft_disp_spi->host->hw->mosi_dlen.usr_mosi_dbitlen = 7;
  tft_disp_spi->host->hw->cmd.usr = 1; // Start transfer
  while (tft_disp_spi->host->hw->cmd.usr)
    ; // Wait for SPI bus ready

  wd = (uint32_t)drawColor.r;
  wd |= (uint32_t)drawColor.g << 8;
  wd |= (uint32_t)drawColor.b << 16;

  // Set DC to 1 (data mode);
  gpio_set_level((gpio_num_t)PIN_NUM_DC, 1);

  tft_disp_spi->host->hw->data_buf[0] = wd;
  tft_disp_spi->host->hw->mosi_dlen.usr_mosi_dbitlen = 23;
  tft_disp_spi->host->hw->cmd.usr = 1; // Start transfer
  while (tft_disp_spi->host->hw->cmd.usr)
    ; // Wait for SPI bus ready

  taskENABLE_INTERRUPTS();

  disp_deselect();
}

void Display::fillDisplay()
{
  if (disp_select() != ESP_OK)
    return;

  uint32_t wd = 0;

  taskDISABLE_INTERRUPTS();

  setAddrWindow(tft_dispWin.x1, tft_dispWin.x1 + (width - 1), tft_dispWin.y1, tft_dispWin.y1 + (height - 1));

  // Send RAM WRITE command
  gpio_set_level((gpio_num_t)PIN_NUM_DC, 0);
  tft_disp_spi->host->hw->data_buf[0] = (uint32_t)TFT_RAMWR;
  tft_disp_spi->host->hw->mosi_dlen.usr_mosi_dbitlen = 7;
  tft_disp_spi->host->hw->cmd.usr = 1; // Start transfer
  while (tft_disp_spi->host->hw->cmd.usr)
    ; // Wait for SPI bus ready

  wd = (uint32_t)drawColor.r;
  wd |= (uint32_t)drawColor.g << 8;
  wd |= (uint32_t)drawColor.b << 16;

  // Set DC to 1 (data mode);
  gpio_set_level((gpio_num_t)PIN_NUM_DC, 1);

  tft_disp_spi->host->hw->data_buf[0] = wd;
  tft_disp_spi->host->hw->mosi_dlen.usr_mosi_dbitlen = 23;

  for (int i = 0; i <= width * height; i++)
  {
    tft_disp_spi->host->hw->cmd.usr = 1; // Start transfer
    while (tft_disp_spi->host->hw->cmd.usr)
      ; // Wait for SPI bus ready
  }

  taskENABLE_INTERRUPTS();

  disp_deselect();
}

void Display::setPixel(int x, int y)
{
  // x = column, y = row
  colorBuf[x + y * width] = drawColor;
}

void Display::setRect(int x, int y, int w, int h)
{
  // x = column, y = row
  for (int row = y; row < y+h; row++)
  {
    for (int col = x; col < x+w; col++)
    {
      colorBuf[col + row * width] = drawColor;
    }
  }
}

void Display::drawSqr(int16_t x, int16_t y, int16_t w, int16_t h)
{
  if (disp_select() != ESP_OK)
    return;

  uint32_t wd = 0;

  taskDISABLE_INTERRUPTS();

  x += tft_dispWin.x1;
  y += tft_dispWin.y1;

  setAddrWindow(x, x + (w - 1), y, y + (h - 1));

  // Send RAM WRITE command
  gpio_set_level((gpio_num_t)PIN_NUM_DC, 0);
  tft_disp_spi->host->hw->data_buf[0] = (uint32_t)TFT_RAMWR;
  tft_disp_spi->host->hw->mosi_dlen.usr_mosi_dbitlen = 7;
  tft_disp_spi->host->hw->cmd.usr = 1; // Start transfer
  while (tft_disp_spi->host->hw->cmd.usr)
    ; // Wait for SPI bus ready

  wd = (uint32_t)drawColor.r;
  wd |= (uint32_t)drawColor.g << 8;
  wd |= (uint32_t)drawColor.b << 16;

  // Set DC to 1 (data mode);
  gpio_set_level((gpio_num_t)PIN_NUM_DC, 1);

  tft_disp_spi->host->hw->data_buf[0] = wd;
  tft_disp_spi->host->hw->mosi_dlen.usr_mosi_dbitlen = 23;

  for (int i = 0; i <= w * h; i++)
  {
    tft_disp_spi->host->hw->cmd.usr = 1; // Start transfer
    while (tft_disp_spi->host->hw->cmd.usr)
      ; // Wait for SPI bus ready
  }

  taskENABLE_INTERRUPTS();

  disp_deselect();
}

void Display::blit(void)
{
  if (disp_select() != ESP_OK)
    return;

  taskDISABLE_INTERRUPTS();

  setAddrWindow(
      tft_dispWin.x1, tft_dispWin.x2,
      tft_dispWin.y1, tft_dispWin.y2);

  // Send RAM WRITE command
  gpio_set_level((gpio_num_t)PIN_NUM_DC, 0);

  tft_disp_spi->host->hw->data_buf[0] = (uint32_t)TFT_RAMWR;
  tft_disp_spi->host->hw->mosi_dlen.usr_mosi_dbitlen = 7;
  tft_disp_spi->host->hw->cmd.usr = 1; // Start transfer
  while (tft_disp_spi->host->hw->cmd.usr)
    ; // Wait for SPI bus ready

  // Set DC to 1 (data mode);
  gpio_set_level((gpio_num_t)PIN_NUM_DC, 1);

  tft_disp_spi->host->hw->mosi_dlen.usr_mosi_dbitlen = 23;

  for (int i = 0; i < colorBufArea; i++)
  {
    tft_disp_spi->host->hw->data_buf[0] = (uint32_t)colorBuf[i].r | (uint32_t)colorBuf[i].g << 8 | (uint32_t)colorBuf[i].b << 16;

    tft_disp_spi->host->hw->cmd.usr = 1; // Start transfer
    while (tft_disp_spi->host->hw->cmd.usr)
      ; // Wait for SPI bus ready
  }

  taskENABLE_INTERRUPTS();

  disp_deselect();
}
