/* UART Events Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
//new compile on asus
//test commit
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "include/rBell.h"
#include "include/errors.h"
#include "include/rbCommandProc.h"

#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_http_server.h"

#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "include/uartFunctions.h"
#include "include/rtc_wdt.h"
#include "filterTags.h"
// #include "c:\\Espressif\\frameworks\\esp-idf-v5.1.1\\components\\esp_hw_support\\include\\rtc_wdt.h"
//----------------------------------------------------------------------
void stopTimer();
void startTimer();
void timerCallback(TimerHandle_t xTimer);
int cPrintf(const char *fmt, ...);
extern void genAPW(char *input_string, unsigned char *hash);
//----------------------------------------------------------------------
TimerHandle_t timer;
volatile uint32_t globalFilterVariable = 0;
static const char *TAG = "uart_events";
TaskHandle_t ISR = NULL;
bool dataAvailable0 = false;
static httpd_handle_t httpServerInstance = NULL;
//------------------------------------------------
#define EX_UART_NUM UART_NUM_2
#define _ESP_WIFI_SSID CONFIG_ESP_WIFI_SSID
#define _ESP_WIFI_PASS CONFIG_ESP_WIFI_PASSWORD
#define _ESP_WIFI_CHANNEL CONFIG_ESP_WIFI_CHANNEL
#define _MAX_STA_CONN CONFIG_ESP_MAX_STA_CONN
#define SERVER_PORT 3500
#define HTTP_METHOD HTTP_POST
#define URI_STRING "/test"
#define MBEDTLS_SHA256_C
//------------------------------------------------
//------------------------------------------------
//------------------------------------------------
extern struct
{
    uint8_t dataCount;
    uint8_t head;
    uint8_t tail;
    char dataBuf[128];
    char rxChar;
    bool dataAvailble;
} rxDataS0;
//------------------------------------------------
extern struct
{
    uint16_t dataCount;
    uint16_t head;
    uint16_t tail;
    uint8_t dataBuf[2048];
    char rxChar;
    bool dataAvailble;
} rxDataS2;

//===============================================================
//===============================================================
//===============================================================
//===============================================================
//===============================================================
static esp_err_t methodHandler(httpd_req_t *httpRequest)
{
    ESP_LOGI("HANDLER", "This is the handler for the <%s> URI", httpRequest->uri);
    return ESP_OK;
}
//===================================================================================================
static httpd_uri_t testUri = {
    .uri = URI_STRING,
    .method = HTTP_METHOD,
    .handler = methodHandler,
    .user_ctx = NULL,
};
//===================================================================================================
static void startHttpServer(void)
{
    httpd_config_t httpServerConfiguration = HTTPD_DEFAULT_CONFIG();
    httpServerConfiguration.server_port = SERVER_PORT;
    if (httpd_start(&httpServerInstance, &httpServerConfiguration) == ESP_OK)
    {
        ESP_ERROR_CHECK(httpd_register_uri_handler(httpServerInstance, &testUri));
    }
    else
    {
        // ESP_LOGI("WIFI","start failed");
    }
}
//===================================================================================================
static void stopHttpServer(void)
{
    if (httpServerInstance != NULL)
    {
        // ESP_ERROR_CHECK(httpd_stop(httpServerInstance));
    }
}
//=====================================================================================================================
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        startHttpServer();
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        stopHttpServer();
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}
//===================================================================================================
void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = _ESP_WIFI_SSID,
            .ssid_len = strlen(_ESP_WIFI_SSID),
            .channel = _ESP_WIFI_CHANNEL,
            .password = _ESP_WIFI_PASS,
            .max_connection = _MAX_STA_CONN,
#ifdef CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT
            .authmode = WIFI_AUTH_WPA3_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
#else /* CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT */
            .authmode = WIFI_AUTH_WPA2_PSK,
#endif
            .pmf_cfg = {
                .required = true,
            },
        },
    };
    if (strlen(_ESP_WIFI_PASS) == 0)
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             _ESP_WIFI_SSID, _ESP_WIFI_PASS, _ESP_WIFI_CHANNEL);
}
//===================================================================================================
// Timer callback function
void timerCallback(TimerHandle_t xTimer) // 100mS ticker
{
    // Update the global variable
    globalFilterVariable++;
}
//=====================================================================================================================
// Function to stop the timer
void stopTimer()
{
    if (xTimerStop(timer, 0) != pdPASS)
    {
        // Handle error
    }
}
//=====================================================================================================================
// Function to stop the timer
void startTimer()
{
    if (xTimerStart(timer, 0) != pdPASS)
    {
        // Handle error
    }
}
//=====================================================================================================================
//=====================================================================================================================
//=====================================================================================================================
void app_main(void)
{
    // esp_log_level_set(TAG, ESP_LOG_INFO);
    uint8_t x = 0;
    static char localBuf[128];
    // u_int8_t touT = 0;
    //  Create a timer with a 1-second period (1000 ms)

    timer = xTimerCreate(
        "MyTimer",         // Timer name
        pdMS_TO_TICKS(10), // Timer period in ticks (1 tick = 1 ms)
        pdTRUE,            // Auto-reload timer
        (void *)0,         // Timer ID
        timerCallback      // Timer callback function
    );

    //--------------------------------------------------------
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
    }
    ESP_ERROR_CHECK(ret);

    // Check if the timer was created successfully
    if (timer == NULL)
    {
        // Handle error
    }
    else
    {
        startTimer();
    }

    UART0Init();
    UART2Init();
    cPrintf("Gigalot(Pty/Ltd) UHF RFID Reader\r\n");
    vTaskDelay(500 / portTICK_PERIOD_MS);
    GetFirmwareVersion((u_int8_t *)localBuf, 0); // send buffer and lenght to function
    GetReaderTxPower((u_int8_t *)localBuf, 0);
    tagReadFlags.showRawEPC = false;
    tagReadFlags.countTags = false;

    char *input_string = "600988042800001Gigalot64";
    u_int8_t apw[32];

    genAPW(input_string, apw);
    cPrintf("SHA-256 Hash: ");
    for (int i = 0; i < 4; i++)
    {
        cPrintf("%02X", apw[i]);
    }
    cPrintf("\n");
    tagReadFlags.readKraalTag = false;
    tagReadFlags.readEPCTag = true;
    //------------------------------------------------------------------------------------------------

    // ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    // wifi_init_softap();
    while (1)
    {
        //-----------------------------------------------------------------
        if (rxDataS0.tail != rxDataS0.head)
        {
            // printf("Data Avaiable..");
            while (rxDataS0.tail != rxDataS0.head)
            {
                // cPrintf("%02X ", rxDataS0.dataBuf[rxDataS0.tail]); // echo local typec char
                localBuf[x++] = rxDataS0.dataBuf[rxDataS0.tail++];
                if (rxDataS0.tail >= sizeof(rxDataS0.dataBuf))
                {
                    rxDataS0.tail = 0;
                }
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
            if (strstr((const char *)localBuf, "\r")) // test fro CR
            {
                localBuf[x] = 0; // always write next char as null
                // cPrintf("\r\n%d -> CR Found, process command:%s",x,localBuf);          //echo local typec char
                cPrintf("\n"); // add newline for consictancy
                // vTaskDelay(50 / portTICK_PERIOD_MS);
                processCmd((char *)localBuf);
                vTaskDelay(50 / portTICK_PERIOD_MS);
                x = 0;
                memset(localBuf, 0, sizeof(localBuf));
            }
            else
            {
                vTaskDelay(10 / portTICK_PERIOD_MS);
                // do nothing; wait for CR
                // cPrintf("%s\r\n",localBuf);
            }
        }
        //-----------------------------------------------------------------
        // no BT for now
        /* if (btDataS.dataAvailble == true)
         {
             if (waitWrite == true && btHandle != 0)
             {
                 btDataS.dataBuf[btDataS.dataCount] = '\r'; // add CR to end
                 cPrintf("\r\n");
                 processCmd((char *)btDataS.dataBuf);
                 vTaskDelay(50 / portTICK_PERIOD_MS);
                 memset(btDataS.dataBuf, 0, sizeof(btDataS.dataBuf));
             }
             btDataS.dataAvailble = false;
         }*/
        // cPrintf("...");
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
