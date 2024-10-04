#include "driver/uart.h"
#include "driver/gpio.h"
#include "include/uartFunctions.h"
#include <stdbool.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define UART_BAUD_RATE 115200    // UART baud rate
#define BUF_SIZE 1024            // UART receive buffer size
#define UART0_RX_PIN GPIO_NUM_3  // GPIO number for UART RX pin
#define UART0_TX_PIN GPIO_NUM_1  // GPIO number for UART TX pin
#define UART2_RX_PIN GPIO_NUM_16 // GPIO number for UART RX pin
#define UART2_TX_PIN GPIO_NUM_17 // GPIO number for UART TX pin
#define UART_BAUD_RATE 115200    // UART baud rate

static QueueHandle_t uart0_queue;
static QueueHandle_t uart2_queue;
//==========================================================
//declare struct here
struct
{
    uint8_t dataCount;
    uint8_t head;
    uint8_t tail;
    char dataBuf[128];
    char rxChar;
    bool dataAvailble;
} rxDataS0;
//==========================================================
struct
{
    uint16_t dataCount;
    uint16_t head;
    uint16_t tail;
    uint8_t dataBuf[2048];
    char rxChar;
    bool dataAvailble;
} rxDataS2;
//===========================================================
void UART0Init()
{
    uart_config_t uart0_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    uart_param_config(UART_NUM_0, &uart0_config);
    uart_driver_install(UART_NUM_0, BUF_SIZE, BUF_SIZE, 10, &uart0_queue, 0);
    uart_enable_rx_intr(UART_NUM_0); // Enable UART RX interrupt
    uart_set_pin(UART_NUM_0, UART0_TX_PIN, UART0_RX_PIN, -1, -1);
    xTaskCreate(uart0_rx_task, "uart0_rx_task", 2048, NULL, 10, NULL);
}
//===========================================================
void UART2Init()
{
    uart_config_t uart2_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    uart_param_config(UART_NUM_2, &uart2_config);
    uart_driver_install(UART_NUM_2, BUF_SIZE, BUF_SIZE, 10, &uart2_queue, 0);
    uart_enable_rx_intr(UART_NUM_2); // Enable UART RX interrupt
    uart_set_pin(UART_NUM_2, UART2_TX_PIN, UART2_RX_PIN, -1, -1);
    xTaskCreate(uart2_rx_task, "uart2_rx_task", 2048, NULL, 10, NULL);
}
//=========================================================================
static void IRAM_ATTR uart0_rx_intr_handler(void *arg)
{
    char data;
    int rxDataLen = 0;
    uart_get_buffered_data_len(UART_NUM_0, (size_t *)&rxDataLen);
    //printf("Nr Bytes 1:%d ", rxDataLen);

    // Process received data
    // Here, you can perform any required operations with the received data
    for (int x = 0; x < rxDataLen; x++)
    {
        uart_read_bytes(UART_NUM_0, &data, 1, (uint32_t)100000);
        cPrintf("%c", data);         //echo command do not comment out
        rxDataS0.dataBuf[rxDataS0.head++] = data;
        if (rxDataS0.head >= sizeof(rxDataS0.dataBuf))
            rxDataS0.head = 0;
        rxDataS0.rxChar = data;
    }
    rxDataS0.dataAvailble = true;
}
//=========================================================================
static void IRAM_ATTR uart2_rx_intr_handler(void *arg)
{
    char data;
    int rxDataLen = 0;
    uart_get_buffered_data_len(UART_NUM_2, (size_t *)&rxDataLen);
    //printf("Nr Bytes 2:%d \r\n", rxDataLen);

    // Process received data
    // Here, you can perform any required operations with the received data
    rxDataS2.dataCount = rxDataLen;
    
    for (int x = 0; x < rxDataLen; x++)
    {
        uart_read_bytes(UART_NUM_2, &data, 1, (uint32_t)100000);
        //cPrintf("%x-", data);
        rxDataS2.dataBuf[rxDataS2.head++] = data;
        if (rxDataS2.head >= sizeof(rxDataS2.dataBuf))
            rxDataS2.head = 0;
        rxDataS2.rxChar = data;
    }
    //cPrintf("-d2a");
    rxDataS2.dataAvailble = true;
    //cPrintf("\r\n");
}
//=========================================================================
static void uart0_rx_task(void *pvParameters)
{
    uart_event_t event;
    while (1)
    {
        if (xQueueReceive(uart0_queue, (void *)&event, portMAX_DELAY))
        {
            if (event.type == UART_DATA)
            {
                uart0_rx_intr_handler(NULL);
            }
        }
    }
}
//=========================================================================
static void uart2_rx_task(void *pvParameters)
{
    uart_event_t event;
    while (1)
    {
        if (xQueueReceive(uart2_queue, (void *)&event, portMAX_DELAY))
        {
            if (event.type == UART_DATA)
            {
                uart2_rx_intr_handler(NULL);
            }
        }
    }
}
//===============================================================
int cPrintf(const char *fmt, ...)
{
    // int x;
    // UINT tOut;
    char tempBuf[128];
    memset(tempBuf, 0, 16);
    char *tBuf = &tempBuf[0];
    va_list argptr;
    va_start(argptr, fmt);
    vsprintf(tBuf, fmt, argptr);
    va_end(argptr);
    // x=0;
    while (*tBuf)
    {
        uart_write_bytes(UART_NUM_0, (const void *)tBuf, 1);
        tBuf++;
    }
    // free(tBuf);
    return 0;
}
//===============================================================
int cPrintf2(const char *fmt, ...)
{
    // int x;
    // UINT tOut;
    char tempBuf[16];
    memset(tempBuf, 0, 16);
    char *tBuf = &tempBuf[0];
    va_list argptr;
    va_start(argptr, fmt);
    vsprintf(tBuf, fmt, argptr);
    va_end(argptr);
    // x=0;
    while (*tBuf)
    {
        uart_write_bytes(UART_NUM_2, (const void *)tBuf, 1);
        tBuf++;
    }
    // free(tBuf);
    return 0;
}
//===============================================================
int dPrintf(const char *fmt, ...)
{
    // int x;
    // UINT tOut;
    char tempBuf[16];
    memset(tempBuf, 0, 16);
    char *tBuf = &tempBuf[0];
    va_list argptr;
    va_start(argptr, fmt);
    vsprintf(tBuf, fmt, argptr);
    va_end(argptr);
    // x=0;
    while (*tBuf)
    {
        uart_write_bytes(UART_NUM_2, (const void *)tBuf, 1);
        tBuf++;
    }
    // free(tBuf);
    return 0;
}