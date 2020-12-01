#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
// #include "esp_task_wdt.h"
#include "sdkconfig.h"

// https://github.com/espressif/esp-idf/blob/master/examples/peripherals/gpio/generic_gpio/main/gpio_example_main.c

// https://github.com/espressif/arduino-esp32/issues/2493
// xTaskCreatePinnedToCore(&locker, "locker", 2048, NULL, 5, NULL, 0);
// or
// https://gist.github.com/Lucianovici/1a671502438c92fc5c049dc0604eb73f
// xTaskCreatePinnedToCore(&firstCore, "firstCore", configMINIMAL_STACK_SIZE * 6, NULL, 2 | portPRIVILEGE_BIT, NULL, PRO_CPU_NUM);
// xTaskCreatePinnedToCore(&secondCore, "secondCore", configMINIMAL_STACK_SIZE * 6, NULL, 2 | portPRIVILEGE_BIT, NULL, APP_CPU_NUM);

// The idea: Once a pin goes high we note which pin. Then we start a
// sequence that ends when we either get the other pin or a timer
// expires.

#define GPIO_INPUT_IO_0 2  // "A" Clock
#define GPIO_INPUT_IO_1 15 // "B" Data

unsigned long currentTime;
unsigned long taskDelayTime;

// WINNER. This work fines, however, it still uses polling.
// Notes: In addition to the internal PullUp mode I also added
// a nF capacitor from the pin to ground. The Cap can range from
// 50nF to 0.1uF or whatever works best for your requirements.
// ~40nF seems to be fine.
// The Caps help with rotaries that have a LOT of switch bouncing.
void app_main(void)
{
    printf("Starting app\n");

    gpio_reset_pin(GPIO_INPUT_IO_0); // Clk
    gpio_set_direction(GPIO_INPUT_IO_0, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_INPUT_IO_0, GPIO_PULLUP_ONLY);

    gpio_reset_pin(GPIO_INPUT_IO_1); // Data
    gpio_set_direction(GPIO_INPUT_IO_1, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_INPUT_IO_1, GPIO_PULLUP_ONLY);

    // char buffer[16];

    int seqState = -1;
    int counter = 0;

    // Previous pin states
    int pClkPin = -1;
    int pDataPin = -1;

    while (1)
    {
        // static uint16_t stateClk = 0, stateData = 0;

        // -------------------------------------------------------
        // This demo delays every 4 seconds rather delaying on
        // every loop. This will keep the watchdog timer from
        // complaining.
        // -------------------------------------------------------
        currentTime = esp_timer_get_time() / 1000;

        if (currentTime >= (taskDelayTime + 4000))
        {
            taskDelayTime = currentTime;
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }

        // -------------------------------------------------------
        // Capture the current pin state.
        // -------------------------------------------------------
        int clkPin = gpio_get_level(GPIO_INPUT_IO_0);
        int dataPin = gpio_get_level(GPIO_INPUT_IO_1);

        // -------------------------------------------------------
        // If a pin goes Low AND we currently NOT in an active
        // sequence then we can correctly transition into an
        // active bit sequence detection algorithm.
        // -------------------------------------------------------
        if ((clkPin == 0 || dataPin == 0) && seqState < 0)
        {
            // printf("Sequence started: Clk (%d), Data(%d)\n", clkPin, dataPin);
            seqState = 1;
            pClkPin = -1;
            pDataPin = -1;
        }

        // -------------------------------------------------------
        // Attempt to detect a bit sequence
        // sequence then we can correctly transition into an
        // active bit sequence detection algorithm.
        // -------------------------------------------------------
        if (seqState == 1)
        {
            // From inspection of the bit streams produced by each pin we can
            // see that CCW occurs if the previous and current pin states are
            // the same:
            // Clk : 111100...11...11111111   Clk and Data sync
            // Data: 111000...11...11111111
            if (clkPin == 1 && pClkPin == 1 && dataPin == 1 && pDataPin == 1)
            {
                counter--;
                printf("CCW Counter %d\n", counter);
                // The sequence is complete. Reset for another sequence.
                seqState = -1;
            }

            // Inspection of the streams we can that CW occurs if the is continously
            // High during the sequence AND the Data changes state from "0" to "1".
            // Clk : 11110...11...111111111   Clk-leads
            // Data: 11100...01...111111111
            if (clkPin == 1 && pClkPin == 1 && pDataPin == 0 && dataPin == 1)
            {
                counter++;
                printf("CW Counter %d\n", counter);
                // The sequence is complete. Reset for another sequence.
                seqState = -1;
            }
        }

        // -------------------------------------------------------
        // Below was for inspecting the stream.
        // -------------------------------------------------------
        // stateClk = (stateClk << 1) | clkPin | 0xe000; // Clk
        // itoa(stateClk, buffer, 2);
        // if (stateClk != 0xffff)
        // {
        //     printf("Clk : ");
        //     for (size_t i = 0; i < 16; i++)
        //     {
        //         buffer[i] == '1' ? printf("1") : printf("0");
        //     }
        //     printf("\n");
        // }

        // stateData = (stateData << 1) | dataPin | 0xe000;
        // itoa(stateData, buffer, 2);
        // if (stateData != 0xffff)
        // {
        //     printf("Data: ");
        //     for (size_t i = 0; i < 16; i++)
        //     {
        //         buffer[i] == '1' ? printf("1") : printf("0");
        //     }
        //     printf("\n");
        // }

        pClkPin = clkPin;
        pDataPin = dataPin;
    }
}