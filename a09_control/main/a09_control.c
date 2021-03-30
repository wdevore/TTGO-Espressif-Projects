/* UART Echo Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

/**
 * This is an example which echos any data it receives on configured UART back to the sender,
 * with hardware flow control turned off. It does not use UART driver event queue.
 *
 * - Port: configured UART
 * - Receive (Rx) buffer: on
 * - Transmit (Tx) buffer: off
 * - Flow control: off
 * - Event queue: off
 * - Pin assignment: see defines below (See Kconfig)
 */

#define ECHO_TEST_TXD (1)
#define ECHO_TEST_RXD (3)
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)

#define ECHO_UART_PORT_NUM      (0)
#define ECHO_UART_BAUD_RATE     (115200)
#define ECHO_TASK_STACK_SIZE    (2048)

#define BUF_SIZE (1024)

#define RESET_GPIO 26       // Pin 22 on FPGA
#define CLOCK_GPIO 27       // Pin 21 on FPGA
#define SELECT0_GPIO 25     // Pin 19 on FPGA   LSB bit
#define SELECT1_GPIO 33     // Pin 20 on FPGA   MSB bit
#define PC_INC_GPIO 32      // Pin 18 on FPGA

static void echo_task(void *arg)
{
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = ECHO_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    int intr_alloc_flags = 0;
    bool pc_inc = false;

#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    ESP_ERROR_CHECK(uart_driver_install(ECHO_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(ECHO_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(ECHO_UART_PORT_NUM, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS));

    // Configure a temporary buffer for the incoming data
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

    /* Configure the IOMUX register for pad BLINK_GPIO (some pads are
       muxed to GPIO on reset already, but some default to other
       functions and need to be switched to GPIO. Consult the
       Technical Reference for a list of pads and their default
       functions.)
    */
    gpio_reset_pin(RESET_GPIO);
    gpio_set_direction(RESET_GPIO, GPIO_MODE_OUTPUT);// Set the GPIO as a push/pull output

    gpio_reset_pin(CLOCK_GPIO);
    gpio_set_direction(CLOCK_GPIO, GPIO_MODE_OUTPUT);

    gpio_reset_pin(SELECT0_GPIO);
    gpio_set_direction(SELECT0_GPIO, GPIO_MODE_OUTPUT);
    gpio_reset_pin(SELECT1_GPIO);
    gpio_set_direction(SELECT1_GPIO, GPIO_MODE_OUTPUT);

    gpio_reset_pin(PC_INC_GPIO);
    gpio_set_direction(PC_INC_GPIO, GPIO_MODE_OUTPUT);

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    gpio_set_level(RESET_GPIO, 1);
    gpio_set_level(CLOCK_GPIO, 0);

    gpio_set_level(SELECT0_GPIO, 0);
    gpio_set_level(SELECT1_GPIO, 0);

    // Default to PC Inc disabled
    gpio_set_level(PC_INC_GPIO, 1);

    printf("\n\nReady:\n");
    printf("'x' = Clock\n");
    printf("'z' = Reset\n");
    printf("'a,s,d,f' = Select (00,01,10,11)\n");
    printf("'c' = Toggle PC Inc\n");

    while (1) {
        // Read data from the UART
        int len = uart_read_bytes(ECHO_UART_PORT_NUM, data, BUF_SIZE, 20 / portTICK_RATE_MS);
        
        if (len > 0) {
            // printf("Len (%d)\n", len);
            switch (*data)
            {
            case 'z':
                printf("Reset\n");
                // Reset is Active-low
                gpio_set_level(RESET_GPIO, 0);

                // Now toggle clock
                gpio_set_level(CLOCK_GPIO, 1);
                vTaskDelay(10 / portTICK_PERIOD_MS);
                gpio_set_level(CLOCK_GPIO, 0);

                // Deactivate Reset
                gpio_set_level(RESET_GPIO, 1);
                break;
            case 'x':
                printf("Clock\n");
                gpio_set_level(CLOCK_GPIO, 1);
                vTaskDelay(10 / portTICK_PERIOD_MS);
                gpio_set_level(CLOCK_GPIO, 0);
                break;
            case 'a':
                printf("Select 00\n");
                gpio_set_level(SELECT0_GPIO, 0);
                gpio_set_level(SELECT1_GPIO, 0);

                gpio_set_level(CLOCK_GPIO, 1);
                vTaskDelay(10 / portTICK_PERIOD_MS);
                gpio_set_level(CLOCK_GPIO, 0);
                break;
            case 's':
                printf("Select 01\n");
                gpio_set_level(SELECT0_GPIO, 1);
                gpio_set_level(SELECT1_GPIO, 0);

                gpio_set_level(CLOCK_GPIO, 1);
                vTaskDelay(10 / portTICK_PERIOD_MS);
                gpio_set_level(CLOCK_GPIO, 0);
                break;
            case 'd':
                printf("Select 10\n");
                gpio_set_level(SELECT0_GPIO, 0);
                gpio_set_level(SELECT1_GPIO, 1);

                gpio_set_level(CLOCK_GPIO, 1);
                vTaskDelay(10 / portTICK_PERIOD_MS);
                gpio_set_level(CLOCK_GPIO, 0);
                break;
            case 'f':
                printf("Select 11\n");
                gpio_set_level(SELECT0_GPIO, 1);
                gpio_set_level(SELECT1_GPIO, 1);

                gpio_set_level(CLOCK_GPIO, 1);
                vTaskDelay(10 / portTICK_PERIOD_MS);
                gpio_set_level(CLOCK_GPIO, 0);
                break;
            case 'c':
                pc_inc = !pc_inc;
                if (pc_inc) {
                    printf("PC Inc enabled\n");
                    gpio_set_level(PC_INC_GPIO, 0);
                } else {
                    printf("PC Inc disabled\n");
                    gpio_set_level(PC_INC_GPIO, 1);
                }
                break;
            default:
                break;
            }
        }
    }
}

void app_main(void)
{
    xTaskCreate(echo_task, "uart_echo_task", ECHO_TASK_STACK_SIZE, NULL, 10, NULL);
}