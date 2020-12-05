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

int encoderPinAClk = 2;   // "A" Clock
int encoderPinAData = 15; // "B" Data

int encoderPinBClk = 13;  // "A" Clock
int encoderPinBData = 12; // "B" Data

volatile int lastAEncoded = 0;
volatile int lastBEncoded = 0;
volatile long encoderAValue = 0;
volatile long encoderBValue = 0;
volatile int preASum = 0;
volatile int preBSum = 0;

long lastencoderAValue = 0;
long lastencoderBValue = 0;

static xQueueHandle gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

void encodeAValue()
{
    int MSB = gpio_get_level(encoderPinAClk);
    int LSB = gpio_get_level(encoderPinAData);

    int encoded = (MSB << 1) | LSB; //converting the 2 pin value to single number

    int sum = (lastAEncoded << 2) | encoded; //adding it to the previous encoded value
    // printf("A sum %x, %x\n", preSumA, sum);

    if (preASum == 0b0010 && sum == 0b1011)
        encoderAValue++;
    else if (preASum == 0b1000 && sum == 0b0011)
        encoderAValue--;

    // if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011)
    //     encoderAValue++;
    // if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000)
    //     encoderAValue--;

    lastAEncoded = encoded; //store this value for next time
    preASum = sum;
}

void encodeBValue()
{
    int MSB = gpio_get_level(encoderPinBClk);
    int LSB = gpio_get_level(encoderPinBData);

    int encoded = (MSB << 1) | LSB; //converting the 2 pin value to single number

    int sum = (lastBEncoded << 2) | encoded; //adding it to the previous encoded value
    // printf("B sum %x, %x\n", preSumB, sum);

    if (preBSum == 0b0010 && sum == 0b1011)
        encoderBValue++;
    else if (preBSum == 0b1000 && sum == 0b0011)
        encoderBValue--;

    // if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011)
    //     encoderBValue++;
    // if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000)
    //     encoderBValue--;

    lastBEncoded = encoded; //store this value for next time
    preBSum = sum;
}

static void encoder_task(void *arg)
{
    uint32_t io_num;
    while (1)
    {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
        {
            if (io_num == encoderPinAClk || io_num == encoderPinAData)
                encodeAValue();
            else if (io_num == encoderPinBClk || io_num == encoderPinBData)
                encodeBValue();
        }
    }
}

void configEncoderPins(int Clk, int Data)
{
    gpio_reset_pin(Clk); // Clk
    gpio_set_direction(Clk, GPIO_MODE_INPUT);
    gpio_set_pull_mode(Clk, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(Clk, GPIO_INTR_ANYEDGE);

    gpio_reset_pin(Data); // Data
    gpio_set_direction(Data, GPIO_MODE_INPUT);
    gpio_set_pull_mode(Data, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(Data, GPIO_INTR_ANYEDGE);

    // hook isr handler for specific gpio pin
    gpio_isr_handler_add(Clk, gpio_isr_handler, (void *)Clk);
    // hook isr handler for specific gpio pin
    gpio_isr_handler_add(Data, gpio_isr_handler, (void *)Data);
}

void setupTaskAndQueue()
{
    // create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(1, sizeof(uint32_t));

    // start gpio task
    xTaskCreate(encoder_task, "encoder_task", 2048, NULL, 10, NULL);

    // install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
}

void app_main(void)
{
    setupTaskAndQueue();

    configEncoderPins(encoderPinAClk, encoderPinAData);
    configEncoderPins(encoderPinBClk, encoderPinBData);

    printf("Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size());

    int cnt = 0;

    while (1)
    {
        if (lastencoderAValue != encoderAValue)
            printf("A value: %ld <%d>\n", encoderAValue, cnt);
        lastencoderAValue = encoderAValue;

        if (lastencoderBValue != encoderBValue)
            printf("B value: %ld <%d>\n", encoderBValue, cnt);
        lastencoderBValue = encoderBValue;

        cnt++;

        vTaskDelay(10 / portTICK_RATE_MS);
    }
}
