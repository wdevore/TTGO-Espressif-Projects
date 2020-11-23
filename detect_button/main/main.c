#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#define BUTTON0_GPIO 0

// https://docs.google.com/document/d/1qJsHvUZiQqfQZAhLdKXWlpeWXXTLuYCv2ecpG2NcMAY/edit#heading=h.62245t3i0rr5
// for how to setup and build this example

void app_main(void)
{
    int count = 0;
    gpio_reset_pin(BUTTON0_GPIO);

    // Set the GPIO as an input
    gpio_set_direction(BUTTON0_GPIO, GPIO_MODE_INPUT);

    while (1)
    {
        int buttonState = gpio_get_level(BUTTON0_GPIO);

        if (buttonState == 1) {
            printf("Button off");
            count -= 1;
            if (count < 0) {
                count = 0;
            }
        } else {
            printf("Button on");
            count += 1;
        }

        printf(", Count: %d\n", count);

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
