#include <stdio.h>
#include "freertos/FreeRTOS.h"

#include "tftspi.h"
#include "tft.h"

// ==========================================================
// Define which spi bus to use TFT_VSPI_HOST or TFT_HSPI_HOST
#define SPI_BUS TFT_HSPI_HOST
// ==========================================================

// static char tmp_buff[64];

static void disp_header(char *info)
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

    TFT_print(info, CENTER, 4);
}

void displaySplash()
{
    disp_header("Draw line DEMO");

    TFT_setclipwin(0, TFT_getfontheight() + 9, tft_width - 1, tft_height - TFT_getfontheight() - 10);

    TFT_setFont(COMIC24_FONT, NULL);
    int tempy = TFT_getfontheight() + 4;

    tft_fg = TFT_ORANGE;
    TFT_print("ESP32", CENTER, (tft_dispWin.y2 - tft_dispWin.y1) / 2 - tempy);
    TFT_setFont(UBUNTU16_FONT, NULL);

    tft_fg = TFT_CYAN;
    TFT_print("Demo", CENTER, LASTY + tempy);
}

void initDisplay()
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
        .miso_io_num = PIN_NUM_MISO, // set SPI MISO pin
        .mosi_io_num = PIN_NUM_MOSI, // set SPI MOSI pin
        .sclk_io_num = PIN_NUM_CLK,  // set SPI CLK pin
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 6 * 1024,
    };
    spi_lobo_device_interface_config_t devcfg = {
        .clock_speed_hz = 26000000,         // Initial clock out at 26 MHz
        .mode = 0,                         // SPI mode 0
        .spics_io_num = -1,                // we will use external CS pin
        .spics_ext_io_num = PIN_NUM_CS,    // external CS pin
        .flags = LB_SPI_DEVICE_HALFDUPLEX, // ALWAYS SET  to HALF DUPLEX MODE!! for display spi
    };

    vTaskDelay(500 / portTICK_RATE_MS);
    printf("\r\n==============================\n");
    printf("TFT display Drawline 12/2020\n");
    printf("==============================\n");
    printf("Pins used: miso=%d, mosi=%d, sck=%d, cs=%d\n", PIN_NUM_MISO, PIN_NUM_MOSI, PIN_NUM_CLK, PIN_NUM_CS);

    // ==================================================================
    // ==== Initialize the SPI bus and attach the LCD to the SPI bus ====
    // ==================================================================

    ret = spi_lobo_bus_add_device(SPI_BUS, &buscfg, &devcfg, &spi);
    assert(ret == ESP_OK);
    printf("SPI: display device added to spi bus (%d)\r\n", SPI_BUS);
    tft_disp_spi = spi;

    // ==== Test select/deselect ====
    ret = spi_lobo_device_select(spi, 1);
    assert(ret == ESP_OK);
    ret = spi_lobo_device_deselect(spi);
    assert(ret == ESP_OK);

    printf("SPI: attached display device, speed=%u\r\n", spi_lobo_get_speed(spi));
    printf("SPI: bus uses native pins: %s\r\n", spi_lobo_uses_native_pins(spi) ? "true" : "false");

    // ================================
    // ==== Initialize the Display ====
    // ================================
    printf("SPI: display init..\n");
    TFT_display_init();

    printf("Dispaly dimensions: %dx%d\n", tft_width, tft_height);

    // ---- Detect maximum read speed ----
    tft_max_rdclock = find_rd_speed();
    printf("SPI: Max rd speed = %u\n", tft_max_rdclock);

    // ==== Set SPI clock used for display operations ====
    spi_lobo_set_speed(spi, 26666666);   // Max is 26.6MHz
    printf("SPI: Changed speed to %u\n", spi_lobo_get_speed(spi));

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

void reset()
{
    TFT_resetclipwin();

    // Set font attributes
    tft_image_debug = 0;
    tft_bg = TFT_BLACK;
}

void app_main(void)
{
    initDisplay();

    reset();

	TFT_fillScreen(TFT_BLACK);

    displaySplash();

    vTaskDelay(1500 / portTICK_RATE_MS);

	TFT_fillScreen(TFT_BLACK);

    TFT_drawLine(10,10,100,100,TFT_ORANGE);
}
