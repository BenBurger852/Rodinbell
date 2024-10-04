#ifndef UART_FUNCTIONS_
#define UART_FUNCTIONS_

#include <stdio.h>

#define BUF_SIZE 1024 



void UART2Init();
void UART0Init();

//static void uart0_rx_intr_handler(void *arg);
//static void uart2_rx_intr_handler(void *arg);
static void uart0_rx_task(void *pvParameters);
static void uart2_rx_task(void *pvParameters);
int cPrintf(const char *fmt, ...);
int cPrintf2(const char *fmt, ...);
int dPrintf(const char *fmt, ...);

#endif