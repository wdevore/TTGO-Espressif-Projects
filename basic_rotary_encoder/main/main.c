#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_task_wdt.h"
#include "sdkconfig.h"

#define BUTTON0_GPIO 0

// https://docs.google.com/document/d/1qJsHvUZiQqfQZAhLdKXWlpeWXXTLuYCv2ecpG2NcMAY/edit#heading=h.62245t3i0rr5
// for how to setup and build this example

// https://github.com/espressif/arduino-esp32/issues/2493
// xTaskCreatePinnedToCore(&locker, "locker", 2048, NULL, 5, NULL, 0);
// or
// https://gist.github.com/Lucianovici/1a671502438c92fc5c049dc0604eb73f
// xTaskCreatePinnedToCore(&firstCore, "firstCore", configMINIMAL_STACK_SIZE * 6, NULL, 2 | portPRIVILEGE_BIT, NULL, PRO_CPU_NUM);
// xTaskCreatePinnedToCore(&secondCore, "secondCore", configMINIMAL_STACK_SIZE * 6, NULL, 2 | portPRIVILEGE_BIT, NULL, APP_CPU_NUM);

// ----------------------------------------------------------
// Note: pin assignment is important. If pins A/B are assigned
// backwards it doesn't detect rotation.
// Watchdog timer needs to be disabled so we don't get messages
// during runtime:
// > idf.py menuconfig
// Goto "Component config"->"Common ESP-related" and uncheck
// "Initialize Task Watchdog Timer on startup"
// ----------------------------------------------------------
#define pin_A 15 // The chosen "lead" pin based on the loop code.
#define pin_B 2

int brightness = 0; // how bright the LED is, start at half brightness
int pBrightness = 0;
const int maxValue = 10;
const int fadeAmount = 1; // how many points to fade the LED by
unsigned long currentTime;
unsigned long loopTime;
unsigned long taskDelayTime;
int encoder_A;
int encoder_B;
int encoder_A_prev = 0;

void app_main(void)
{
    gpio_reset_pin(pin_A);
    gpio_set_direction(pin_A, GPIO_MODE_INPUT);
    gpio_set_pull_mode(pin_A, GPIO_PULLUP_ONLY);

    gpio_reset_pin(pin_B);
    gpio_set_direction(pin_B, GPIO_MODE_INPUT);
    gpio_set_pull_mode(pin_B, GPIO_PULLUP_ONLY);

    currentTime = esp_timer_get_time() / 1000; // In microseconds
    loopTime = currentTime;
    taskDelayTime = currentTime;

    while (1)
    {
        currentTime = esp_timer_get_time() / 1000;
        // printf("%lu\n", currentTime);

        if (currentTime >= (taskDelayTime + 4000))
        {
            // printf("delaying...\n");
            taskDelayTime = currentTime;
            vTaskDelay(10 / portTICK_PERIOD_MS);
            // taskYIELD();
        }

        if (currentTime >= (loopTime + 5))  // 5 = 200Hz, 1 = 1KHz
        {
            // 5ms since last check of encoder = 200Hz
            encoder_A = gpio_get_level(pin_A); // Read encoder pins
            encoder_B = gpio_get_level(pin_B);

            if ((!encoder_A) && (encoder_A_prev))
            {
                // A has gone from high to low
                if (!encoder_B)
                {
                    // B is high so clockwise
                    // increase the brightness, dont go over maxValue
                    if (brightness + fadeAmount <= maxValue)
                        brightness += fadeAmount;
                }
                else
                {
                    // B is low so counter-clockwise
                    // decrease the brightness, dont go below 0
                    if (brightness - fadeAmount >= 0)
                        brightness -= fadeAmount;
                }
            }
            encoder_A_prev = encoder_A; // Store value of A for next time

            if (brightness != pBrightness)
            {
                printf("brightness %d\n", brightness);
                pBrightness = brightness;
            }

            loopTime = currentTime; // Updates loopTime
        }
    }
}
