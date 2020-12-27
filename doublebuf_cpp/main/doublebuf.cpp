#include <iostream>

#include "freertos/FreeRTOS.h"

extern "C"
{
    void app_main(void);
}

#include "display.h"

void app_main(void)
{
    Display disp;

    disp.init();
    disp.reset();

    disp.setClearColor(TFT_GREEN);

    // disp.clear();

    // disp.drawSplash();

    // vTaskDelay(1000 / portTICK_RATE_MS);

    disp.clear();

    disp.setDrawColor(0, 255, 0);
    disp.drawLine(10, 100, 120, 100);

    disp.setDrawColor(0, 0, 255);
    disp.drawLine(10, 110, 120, 110);

    disp.setDrawColor(255, 0, 0);
    disp.drawLine(10, 120, 120, 120);

    disp.setDrawColor(TFT_ORANGE);
    disp.drawLine(10, 130, 120, 130);

    // vTaskDelay(1000 / portTICK_RATE_MS);

    // std::cout << "Blitting..." << std::endl;
    // disp.blit2();
    // std::cout << "Blitted" << std::endl;

    disp.setDrawColor(TFT_DARKGREY);
    int w = 135-1;
    int h = 240-1;
    int x = 0;
    int y = 0;
    disp.fillDisplay();

    disp.setDrawColor(TFT_WHITE);
    for (int i = 0; i < w; i++)
    {
        disp.drawPixel(x, y);
        x++;
        y++;
    }

    disp.setDrawColor(255, 0, 255);
    disp.drawLine(10, 120, 120, 120);
}
