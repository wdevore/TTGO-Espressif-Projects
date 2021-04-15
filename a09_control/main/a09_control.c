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

#define RUN_CPU_TASK 1

// Memory controls
#define SELECT0_GPIO 25     // Pin 19 on FPGA
#define SELECT1_GPIO 33     // Pin 20 on FPGA
#define PC_INC_GPIO 32      // Pin 18 on FPGA

#ifdef RUN_MEMORY_TASK
static void memory_task(void *arg)
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
#endif

// ALU controls
#define ALU_LD_GPIO 25      // Pin 17 on FPGA
#define ALU_OP0_GPIO 33     // Pin 18 on FPGA  ALU op Bit 0
#define ALU_OP1_GPIO 32     // Pin 19 on FPGA  ALU op Bit 1
#define ALU_OP2_GPIO 21     // Pin 20 on FPGA  ALU op Bit 2
#define ALU_OP3_GPIO 22     // Pin 21 on FPGA  ALU op Bit 3

#ifdef RUN_ALU_TASK
static void alu_task(void *arg)
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
    bool alu_ld = false;

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

    // ----------------------------------------------------
    gpio_reset_pin(ALU_OP0_GPIO);
    gpio_set_direction(ALU_OP0_GPIO, GPIO_MODE_OUTPUT);
    gpio_reset_pin(ALU_OP1_GPIO);
    gpio_set_direction(ALU_OP1_GPIO, GPIO_MODE_OUTPUT);
    gpio_reset_pin(ALU_OP2_GPIO);
    gpio_set_direction(ALU_OP2_GPIO, GPIO_MODE_OUTPUT);
    gpio_reset_pin(ALU_OP3_GPIO);
    gpio_set_direction(ALU_OP3_GPIO, GPIO_MODE_OUTPUT);

    gpio_reset_pin(ALU_LD_GPIO);
    gpio_set_direction(ALU_LD_GPIO, GPIO_MODE_OUTPUT);

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    gpio_set_level(RESET_GPIO, 1);
    gpio_set_level(CLOCK_GPIO, 0);

    gpio_set_level(ALU_OP0_GPIO, 0);
    gpio_set_level(ALU_OP1_GPIO, 0);
    gpio_set_level(ALU_OP2_GPIO, 0);
    gpio_set_level(ALU_OP3_GPIO, 0);

    // Default to ALU load disabled
    gpio_set_level(ALU_LD_GPIO, 1);

    printf("\n\nReady:\n");
    printf("'x' = Clock\n");
    printf("'z' = Reset\n");
    printf("'a,s,d,f,g' = ALU Op: ADD, SUB, AND, OR, XOR\n");
    printf("'c' = Toggle ALU Ld\n");

    while (1) {
        // Read data from the UART
        int len = uart_read_bytes(ECHO_UART_PORT_NUM, data, BUF_SIZE, 20 / portTICK_RATE_MS);
        
        if (len > 0) {
            // printf("Len (%d)\n", len);
            switch (*data)
            {
            case 'h':
                printf("'x' = Clock\n");
                printf("'z' = Reset\n");
                printf("'a,s,d,f,g' = ALU Op: ADD, SUB, AND, OR, XOR\n");
                printf("'c' = Toggle ALU Ld\n");
                break;
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
                printf("ALU ADD\n"); // 0000
                gpio_set_level(ALU_OP0_GPIO, 0);
                gpio_set_level(ALU_OP1_GPIO, 0);
                gpio_set_level(ALU_OP2_GPIO, 0);
                gpio_set_level(ALU_OP3_GPIO, 0);
                break;
            case 's':
                printf("ALU SUB\n");  // 0001
                gpio_set_level(ALU_OP0_GPIO, 1);
                gpio_set_level(ALU_OP1_GPIO, 0);
                gpio_set_level(ALU_OP2_GPIO, 0);
                gpio_set_level(ALU_OP3_GPIO, 0);
                break;
            case 'd':
                printf("ALU AND\n");  // 0010
                gpio_set_level(ALU_OP0_GPIO, 0);
                gpio_set_level(ALU_OP1_GPIO, 1);
                gpio_set_level(ALU_OP2_GPIO, 0);
                gpio_set_level(ALU_OP3_GPIO, 0);
                break;
            case 'f':
                printf("ALU OR\n");  // 0011
                gpio_set_level(ALU_OP0_GPIO, 1);
                gpio_set_level(ALU_OP1_GPIO, 1);
                gpio_set_level(ALU_OP2_GPIO, 0);
                gpio_set_level(ALU_OP3_GPIO, 0);
                break;
            case 'g':
                printf("ALU XOR\n"); // 0100
                gpio_set_level(ALU_OP0_GPIO, 0);
                gpio_set_level(ALU_OP1_GPIO, 0);
                gpio_set_level(ALU_OP2_GPIO, 1);
                gpio_set_level(ALU_OP3_GPIO, 0);
                break;
            case 'c':
                alu_ld = !alu_ld;
                if (alu_ld) {
                    printf("ALU load enabled\n");
                    gpio_set_level(ALU_LD_GPIO, 0);
                } else {
                    printf("ALU load disabled\n");
                    gpio_set_level(ALU_LD_GPIO, 1);
                }
                break;
            default:
                break;
            }
        }
    }
}
#endif

// Sequence-Control-Matrix controls
#define SEQCON_IR0_GPIO 33     // Pin 18 on FPGA  ALU op Bit 0
#define SEQCON_IR1_GPIO 32     // Pin 19 on FPGA  ALU op Bit 1
#define SEQCON_IR2_GPIO 21     // Pin 20 on FPGA  ALU op Bit 2
#define SEQCON_IR3_GPIO 22     // Pin 21 on FPGA  ALU op Bit 3

#ifdef RUN_SEQCON_TASK
static void seqcon_task(void *arg)
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

    // ----------------------------------------------------
    gpio_reset_pin(SEQCON_IR0_GPIO);
    gpio_set_direction(SEQCON_IR0_GPIO, GPIO_MODE_OUTPUT);
    gpio_reset_pin(SEQCON_IR1_GPIO);
    gpio_set_direction(SEQCON_IR1_GPIO, GPIO_MODE_OUTPUT);
    gpio_reset_pin(SEQCON_IR2_GPIO);
    gpio_set_direction(SEQCON_IR2_GPIO, GPIO_MODE_OUTPUT);
    gpio_reset_pin(SEQCON_IR3_GPIO);
    gpio_set_direction(SEQCON_IR3_GPIO, GPIO_MODE_OUTPUT);

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    gpio_set_level(RESET_GPIO, 1);
    gpio_set_level(CLOCK_GPIO, 0);

    // Default to LDI
    gpio_set_level(SEQCON_IR0_GPIO, 0);
    gpio_set_level(SEQCON_IR1_GPIO, 0);
    gpio_set_level(SEQCON_IR2_GPIO, 0);
    gpio_set_level(SEQCON_IR3_GPIO, 0);

    printf("\n\nReady:\n");
    printf("'x' = Clock\n");
    printf("'z' = Reset\n");
    printf("'a,s,d,f,g,q' = IR: LDI, OUT, ADD, CMP, BNE, HLT\n");

    while (1) {
        // Read data from the UART
        int len = uart_read_bytes(ECHO_UART_PORT_NUM, data, BUF_SIZE, 20 / portTICK_RATE_MS);
        
        if (len > 0) {
            // printf("Len (%d)\n", len);
            switch (*data)
            {
            case 'h':
                printf("'x' = Clock\n");
                printf("'z' = Reset\n");
                printf("'a,s,d,f,g,q' = IR: LDI, OUT, ADD, CMP, BNE, HLT\n");
                break;
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
                printf("IR LDI\n"); // 0000
                gpio_set_level(SEQCON_IR0_GPIO, 0);
                gpio_set_level(SEQCON_IR1_GPIO, 0);
                gpio_set_level(SEQCON_IR2_GPIO, 0);
                gpio_set_level(SEQCON_IR3_GPIO, 0);
                break;
            case 's':
                printf("IR OUT\n"); // 0001
                gpio_set_level(SEQCON_IR0_GPIO, 1);
                gpio_set_level(SEQCON_IR1_GPIO, 0);
                gpio_set_level(SEQCON_IR2_GPIO, 0);
                gpio_set_level(SEQCON_IR3_GPIO, 0);
                break;
            case 'd':
                printf("IR ADD\n"); // 0010
                gpio_set_level(SEQCON_IR0_GPIO, 0);
                gpio_set_level(SEQCON_IR1_GPIO, 1);
                gpio_set_level(SEQCON_IR2_GPIO, 0);
                gpio_set_level(SEQCON_IR3_GPIO, 0);
                break;
            case 'f':
                printf("IR CMP\n"); // 0011
                gpio_set_level(SEQCON_IR0_GPIO, 1);
                gpio_set_level(SEQCON_IR1_GPIO, 1);
                gpio_set_level(SEQCON_IR2_GPIO, 0);
                gpio_set_level(SEQCON_IR3_GPIO, 0);
                break;
            case 'g':
                printf("IR BNE\n"); // 0100
                gpio_set_level(SEQCON_IR0_GPIO, 0);
                gpio_set_level(SEQCON_IR1_GPIO, 0);
                gpio_set_level(SEQCON_IR2_GPIO, 1);
                gpio_set_level(SEQCON_IR3_GPIO, 0);
                break;
            case 'q':
                printf("IR HLT\n"); // 0101
                gpio_set_level(SEQCON_IR0_GPIO, 1);
                gpio_set_level(SEQCON_IR1_GPIO, 0);
                gpio_set_level(SEQCON_IR2_GPIO, 1);
                gpio_set_level(SEQCON_IR3_GPIO, 0);
                break;
            default:
                break;
            }
        }
    }
}
#endif

#ifdef RUN_CPU_TASK
bool clockEnabled = false;

static void clock_task(void *arg)
{
    for (;;) {
        if (clockEnabled) {
            gpio_set_level(CLOCK_GPIO, 1);
            vTaskDelay(10 / portTICK_PERIOD_MS);
            gpio_set_level(CLOCK_GPIO, 0);
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

static void cpu_task(void *arg)
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

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    gpio_set_level(RESET_GPIO, 1);
    gpio_set_level(CLOCK_GPIO, 0);

    printf("\n\nReady:\n");
    printf("'x' = Clock\n");
    printf("'z' = Reset\n");
    printf("'c' = Toggle clock\n");

    int clkCnt = 0;

    while (1) {
        // Read data from the UART
        int len = uart_read_bytes(ECHO_UART_PORT_NUM, data, BUF_SIZE, 20 / portTICK_RATE_MS);
        
        if (len > 0) {
            // printf("Len (%d)\n", len);
            switch (*data)
            {
            case 'h':
                printf("'x' = Clock\n");
                printf("'z' = Reset\n");
                printf("'c' = Toggle clock\n");
                break;
            case 'z':
                // Reset is Active-low
                gpio_set_level(RESET_GPIO, 0);

                // Now toggle clock
                gpio_set_level(CLOCK_GPIO, 1);
                vTaskDelay(10 / portTICK_PERIOD_MS);
                gpio_set_level(CLOCK_GPIO, 0);
                clkCnt = 1;

                // Deactivate Reset
                gpio_set_level(RESET_GPIO, 1);

                printf("Reset %d\n", clkCnt);
                break;
            case 'x':
                clockEnabled = false;
                gpio_set_level(CLOCK_GPIO, 1);
                vTaskDelay(10 / portTICK_PERIOD_MS);
                gpio_set_level(CLOCK_GPIO, 0);
                clkCnt++;

                printf("Clocked %d\n", clkCnt);
                break;
            case 'c':
                clockEnabled = !clockEnabled;
                printf("Clock enabled: %d\n", clockEnabled);
                break;

            default:
                break;
            }
        }
    }
}
#endif

void app_main(void)
{
#ifdef RUN_MEMORY_TASK
    xTaskCreate(memory_task, "a_task", ECHO_TASK_STACK_SIZE, NULL, 10, NULL);
#endif

#ifdef RUN_ALU_TASK
    xTaskCreate(alu_task, "a_task", ECHO_TASK_STACK_SIZE, NULL, 10, NULL);
#endif

#ifdef RUN_SEQCON_TASK
    xTaskCreate(seqcon_task, "a_task", ECHO_TASK_STACK_SIZE, NULL, 10, NULL);
#endif

#ifdef RUN_CPU_TASK
    xTaskCreate(clock_task, "clock_task", ECHO_TASK_STACK_SIZE, NULL, 10, NULL);
    xTaskCreate(cpu_task, "a_task", ECHO_TASK_STACK_SIZE, NULL, 5, NULL);
#endif
}
