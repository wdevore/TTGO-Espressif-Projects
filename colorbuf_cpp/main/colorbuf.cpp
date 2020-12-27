#include <iostream>

#include "freertos/FreeRTOS.h"

extern "C"
{
    void app_main(void);
}

#include "display.h"

void app_main(void)
{
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    int64_t time_us_1 = 0;
    int64_t time_us_2 = 0;
    int x = 0;
    int y = 0;
    int xs = 1;
    int ys = 1;

    Display disp;

    disp.init();
    disp.reset();

    disp.setClearColor(TFT_BLACK);

    // vTaskDelay(1000 / portTICK_RATE_MS);

    disp.clear();

    disp.setDrawColor(TFT_WHITE);
    // disp.setDrawColor(TFT_BLUE);
    disp.setPixel(0, 0);
    // disp.setDrawColor(TFT_RED);
    // disp.setPixel(1, 0);
    // disp.setDrawColor(TFT_BLUE);
    // disp.setPixel(0, 1);
    disp.setDrawColor(TFT_WHITE);
    // disp.setPixel(1, 1);
    disp.setPixel(25, 25);
    disp.setPixel(disp.Width() - 1, disp.Height() - 1);

    // vTaskDelay(1000 / portTICK_RATE_MS);

    while (1)
    {
        disp.clear();
        disp.setRect(x, y, 5, 5);

        if (x > 50)
        {
            xs = -1;
            ys = -1;
        }
        else if (x == 0)
        {
            xs = 1;
            ys = 1;
        }
        x += xs;
        y += ys;

        gettimeofday(&tv_now, NULL);
        time_us_1 = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;
        // time_us_1 = (int64_t)tv_now.tv_usec;

        disp.blit();

        gettimeofday(&tv_now, NULL);
        time_us_2 = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;
        // time_us_2 = (int64_t)tv_now.tv_usec;
        std::cout << "RT: " << (time_us_2 - time_us_1) / 1000L << std::endl;
    }
}
