#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
// #include "esp_task_wdt.h"
#include "sdkconfig.h"

// Base on:
// https://bildr.org/2012/08/rotary-encoder-arduino/

#define ESP_INTR_FLAG_DEFAULT 0

int encoderPin1 = 2;  // "A" Clock
int encoderPin2 = 15; // "B" Data

volatile int lastEncoded = 0;
volatile long encoderValue = 0;

long lastencoderValue = 0;

int lastMSB = 0;
int lastLSB = 0;

static xQueueHandle gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_task_example(void *arg)
{
    uint32_t io_num;
    for (;;)
    {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
        {
            int MSB = gpio_get_level(encoderPin1);
            int LSB = gpio_get_level(encoderPin2);

            int encoded = (MSB << 1) | LSB; //converting the 2 pin value to single number

            int sum = (lastEncoded << 2) | encoded; //adding it to the previous encoded value
            if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011)
                encoderValue++;
            if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000)
                encoderValue--;

            lastEncoded = encoded; //store this value for next time
        }
    }
}

void app_main(void)
{
    gpio_reset_pin(encoderPin1); // Clk
    gpio_set_direction(encoderPin1, GPIO_MODE_INPUT);
    gpio_set_pull_mode(encoderPin1, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(encoderPin1, GPIO_INTR_ANYEDGE);

    gpio_reset_pin(encoderPin2); // Data
    gpio_set_direction(encoderPin2, GPIO_MODE_INPUT);
    gpio_set_pull_mode(encoderPin2, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(encoderPin2, GPIO_INTR_ANYEDGE);

    // create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(1, sizeof(uint32_t));

    // start gpio task
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

    // install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

    // hook isr handler for specific gpio pin
    gpio_isr_handler_add(encoderPin1, gpio_isr_handler, (void *)encoderPin1);
    // hook isr handler for specific gpio pin
    gpio_isr_handler_add(encoderPin2, gpio_isr_handler, (void *)encoderPin2);

    printf("Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size());

    int cnt = 0;

    while (1)
    {
        if (lastencoderValue != encoderValue)
            printf("encoderValue: %ld <%d>\n", encoderValue, cnt);

        lastencoderValue = encoderValue;
        cnt++;

        vTaskDelay(10 / portTICK_RATE_MS);
    }
}

// #define GPIO_INPUT_IO_0 2  // "A" Clock
// #define GPIO_INPUT_IO_1 15 // "B" Data

// #define GPIO_INPUT_PIN_SEL ((1ULL << GPIO_INPUT_IO_0) | (1ULL << GPIO_INPUT_IO_1))

// #define ESP_INTR_FLAG_DEFAULT 0

// // SemaphoreHandle_t xSemaphore = NULL;
// static xQueueHandle gpio_evt_queue = NULL;

// unsigned long currentTime;
// volatile unsigned long eventTime;
// unsigned long taskDelayTime;

// unsigned long eventDelayCnt = 0;
// unsigned long eventDelay = 1000; // 10ms
// volatile bool eventActive = false;

// volatile int ioPin = -1;

// void app_main_doesnt_work(void)
// {
//     // eventDelayCnt = esp_timer_get_time() / 1000; // In milliseconds

//     // Config isn't working yet.
//     // Setup both pins at the same time using config object
//     // gpio_config_t io_conf;
//     // io_conf.intr_type = GPIO_INTR_NEGEDGE;
//     // // bit mask of the BOTH pins
//     // io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL; // specifies both pins
//     // // set as input mode
//     // io_conf.mode = GPIO_MODE_INPUT;
//     // // pull the pin low and let the rotary pulse it high  __/``
//     // // io_conf.pull_down_en = 1;
//     // // pull the pin high and let the rotary pulse it low  ``\__
//     // io_conf.pull_up_en = 1;
//     // gpio_config(&io_conf);

//     gpio_reset_pin(GPIO_INPUT_IO_0); // Clk
//     gpio_set_direction(GPIO_INPUT_IO_0, GPIO_MODE_INPUT);
//     gpio_set_pull_mode(GPIO_INPUT_IO_0, GPIO_PULLUP_ONLY);
//     gpio_set_intr_type(GPIO_INPUT_IO_0, GPIO_INTR_NEGEDGE); // ``\__

//     gpio_reset_pin(GPIO_INPUT_IO_1); // Data
//     gpio_set_direction(GPIO_INPUT_IO_1, GPIO_MODE_INPUT);
//     gpio_set_pull_mode(GPIO_INPUT_IO_1, GPIO_PULLUP_ONLY);
//     gpio_set_intr_type(GPIO_INPUT_IO_1, GPIO_INTR_NEGEDGE); // ``\__

//     // create a queue to handle gpio event from isr
//     gpio_evt_queue = xQueueCreate(1, sizeof(uint32_t));

//     // start gpio task
//     xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

//     // install gpio isr service
//     gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

//     // hook isr handler for specific gpio pin
//     gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void *)GPIO_INPUT_IO_0);
//     // hook isr handler for specific gpio pin
//     gpio_isr_handler_add(GPIO_INPUT_IO_1, gpio_isr_handler, (void *)GPIO_INPUT_IO_1);

//     printf("Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size());

//     int pIoPin = -1;
//     int seqState = -1;
//     int counter = 0;

//     int cnt = 0;
//     while (1)
//     {
//         if (ioPin > 0 && seqState < 0)
//         {
//             seqState = 1;
//             // pIoPin = -1;
//             printf("Pin triggered: (%d) -> (%d) <%d>\n", pIoPin, ioPin, cnt);
//         }

//         if (seqState == 1)
//         {
//             // if (pIoPin == ioPin)
//             // {
//             //     seqState = -1;
//             //     pIoPin = -1;
//             //     // continue;
//             // }

//             if (pIoPin == 2 && ioPin == 15)
//             {
//                 counter++;
//                 printf("CW Counter %d\n", counter);
//                 // The sequence is complete. Reset for another sequence.
//                 seqState = -1;
//             }

//             if (pIoPin == 15 && ioPin == 2)
//             {
//                 counter--;
//                 printf("CCW Counter %d\n", counter);
//                 // The sequence is complete. Reset for another sequence.
//                 seqState = -1;
//             }
//         }

//         pIoPin = ioPin;

//         cnt++;
//         vTaskDelay(10 / portTICK_RATE_MS);
//     }
// }
