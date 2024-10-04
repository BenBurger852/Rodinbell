#include "include/rBell.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include <stdbool.h>
#include "filterTags.h"
#include "sha256.h"
#include "epcDisect.h"
#define EPC96BITS (96 / 8)
#define EPC32BITS (32 / 8)
//-------------------------------------------
// #include "c:\\Espressif\\frameworks\\esp-idf-v5.1.1\\components\\esp_hw_support\\include\\rtc_wdt.h"
#include "rtc_wdt.h"
extern int cPrintf(const char *fmt, ...);
extern uint8_t disectEpcTagToBuf(uint8_t *tagBuffer, uint64_t *vizNum);
void genAPW(char *input_string, unsigned char *hash);
uint8_t tagBufferMain[256];

extern struct
{
    uint8_t dataCount;
    uint8_t head;
    uint8_t tail;
    char dataBuf[128];
    char rxChar;
    bool dataAvailble;
} rxDataS0;

extern struct
{
    uint16_t dataCount;
    uint16_t head;
    uint16_t tail;
    uint8_t dataBuf[2048];
    char rxChar;
    bool dataAvailble;
} rxDataS2;

extern uint8_t disectEpcTagNumber(uint8_t *tagBuffer);
extern bool checkFilterTo(void);
#define NOT_IN_LIST true

//=================================================================================
void writeSgtinTag(void)
{
    /*   To write to tag:
    1- read tag and get EPC nr:
    cmd_set_work_antenna 0x74:
    A0 04 01 74 00 E7
    Response example:
    A0 04 01 74 10 D7
    cmd_real_time_inventory 0x89: (1 round only)
    ..............................................
    USE SESSION 1 TO STOP MULTIPLE READS AS WELL 0x8B:
    A0 06 01 8B 01 00 01 CC
    2024-01-11 05:14:56.459  A0 13 01 8B 10 30 00 30 09 66 37 81 DB 02 97 48 76 FC 6A 67 2B
    2024-01-11 05:14:56.467  A0 0A 01 8B 00 00 19 00 00 00 01 B0
    ...............................
    A0 04 01 89 01 D1
    Response: example:
    A0
    13
    01
    89
    14
    34 00
    11 22 33 44 55 66 77 88 99 00 AA BB
    60 B9
    ??? Not sure why this ----- if phase turned on .......
    A0
    0A
    01  ID
    89 CMD
    00 antenna
    00 1B read rate
    00 00 00 01 total reads
    B0 check

    Select tag based on EPC read 0x85:
    Example:
    A0 11 01 85 00 0C 11 22 33 44 55 66 77 88 99 00 AA BB 5B
    Response:
    A0 04 01 85 10 C6 (10 = success)
    Do block write 0x94:
    A0
    0C
    01
    94
    00 00 00 00 Password
    01 Membank (EPC)
    02 Word Addr (start at 0x2 pass PC and CRC)
    01 Word Count (nr of word = x2 bytes)
    CC DD Data
    12 check*/

    uint8_t localBuffer[50];
    uint8_t tmp = 0;
    memset(localBuffer, 0, sizeof(localBuffer)); // always flush buffer
    localBuffer[tmp++] = 0xA0;
    localBuffer[tmp++] = 0x07;
    localBuffer[tmp++] = 0x01; // address always 1
    localBuffer[tmp++] = 0x8B;
    localBuffer[tmp++] = 1;    // always use session 1
    localBuffer[tmp++] = 0x00; // target
    localBuffer[tmp++] = 0x01; // SL
    localBuffer[tmp++] = 1;    // repeats.... 255 never stops
    localBuffer[tmp] = CheckSum(localBuffer, tmp);
    tmp++;
    rxDataS2.dataAvailble = false;
    rbprintfRAW((char *)localBuffer, tmp);
    // vTaskDelay((nrReads*10)/portTICK_PERIOD_MS);   //this is a blocking delay ???
    while (rxDataS2.dataAvailble == false)
    { // wait for response
        rtc_wdt_feed();
        vTaskDelay(1 / portTICK_PERIOD_MS);
        // cPrintf(".");
    }
    // rbResponse((uint8_t *)rbBuffer, dataSize);
    // getRealtimeInventory((uint8_t *)rxDataS2.dataBuf, rxDataS2.dataCount, 10); // by default read 10 sessions
    rbResponseRAW((uint8_t *)rxDataS2.dataBuf, rxDataS2.dataCount);
    rbResponseRAW((uint8_t *)rxDataS2.dataBuf, rxDataS2.dataCount); // posibly more data in buffer ??
    // data is already in the buffer the ISR grabs it

    rtc_wdt_feed();
}
//=================================================================================
void sha256_compute_hash(const unsigned char *data, size_t data_length, unsigned char *hash)
{
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0); // 0 for SHA-256

    // Feed data into the context
    mbedtls_sha256_update(&ctx, data, data_length);

    // Get the SHA-256 hash
    mbedtls_sha256_finish(&ctx, hash);

    mbedtls_sha256_free(&ctx);
}

void genAPW(char *input_string, unsigned char *hash)
{
    // Convert ASCII-encoded string to byte array
    const unsigned char *input_bytes = (const unsigned char *)input_string;

    // Compute SHA-256 hash for the input string
    sha256_compute_hash(input_bytes, strlen(input_string), hash);
}

//=================================================================================
unsigned char CheckSum(unsigned char *uBuff, unsigned char uBuffLen)
{
    unsigned char i, uSum = 0;
    for (i = 0; i < uBuffLen; i++)
    {
        uSum = uSum + uBuff[i];
    }
    uSum = (~uSum) + 1;
    return uSum;
}
//=================================================================================
int rbprintfRAW(char *tBuf, uint8_t sizeOfData)
{
    rxDataS2.dataAvailble = false;
    memset(rxDataS2.dataBuf, 0, sizeof(rxDataS2.dataBuf)); // reset buffer so that RX data can work with response disect command
    rxDataS2.head = 0;
    while (sizeOfData--)
    {
        vTaskDelay(1 / portTICK_PERIOD_MS);
        uart_write_bytes(UART_NUM_2, (const void *)tBuf, 1);        
        //cPrintf("%x ", *tBuf);
        tBuf++;
    }
    //cPrintf("-> %x ", *tBuf);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    return 0;
}
//==================================================================================
//=========================================================================
//  Description:
//  Parameters:    none
//  Returns:           non-zero for an error
//=======================================================================
//===================================================================================
int getRealtimeInventory(uint8_t *rbBuffer, uint8_t dataSize, uint8_t nrReads)
{
    uint16_t bufIndex = 0;
    uint8_t nrOfBytes = 0;
    uint8_t deviceID = 0;
    uint8_t command = 0;
    uint8_t len = 0;
    uint8_t freqAnt = 0;
    uint16_t pcCount = 0;
    uint8_t localEPCBuf[EPC96BITS];
    uint8_t rssi = 0;
    uint8_t checkDigit = 0;
    int ret = -1;

    // data is already in the buffer the ISR grabs it
    while (nrReads > 0 )
    {
        if(ret != -1)
            break;
        nrReads--;

        rtc_wdt_feed();
        //cPrintf("buf:[%x]",rbBuffer[bufIndex]);
        switch (rbBuffer[bufIndex++]) // always start at buf zero
        {
        case 0xA0:                      // start of frame
            len = rbBuffer[bufIndex++]; // tag buffer lenght including checksum
            //cPrintf("len=%d ", len);
            nrOfBytes = len;
            deviceID = rbBuffer[bufIndex++];
            //cPrintf("len=%d ", deviceID);
            nrOfBytes--;
            command = rbBuffer[bufIndex++]; // will be cmd as requested
            //cPrintf("Command=%d ", command);
            nrOfBytes--;
            switch (command) //
            {
            case 0x89: // realtime inventory
            case 0x8B:
                //cPrintf("Lenght of buffer:%d\r\n", len);
                while (nrOfBytes) // repeat for each inventory
                {
                    freqAnt = rbBuffer[bufIndex++];
                    nrOfBytes--;
                    pcCount = (uint16_t)(rbBuffer[bufIndex]) << 8 | rbBuffer[bufIndex + 1]; // read PC word
                    bufIndex += 2;
                    nrOfBytes -= 2;
                    for (len = 0; len < (EPC96BITS); len++) // read 12 epc bytes
                    {
                        localEPCBuf[len] = rbBuffer[bufIndex++];
                        // cPrintf("(%02d)%02X:", len, localEPCBuf[len]);
                        nrOfBytes--;
                    }
                    rssi = rbBuffer[bufIndex++];
                    checkDigit = rbBuffer[bufIndex++];  // read check digit move to next index
                    vTaskDelay(5 / portTICK_PERIOD_MS); // wait for buffer to fill ?????
                    // cPrintf("Rssi:%x[%x]\r\n",rssi, checkDigit);
                    nrOfBytes = 0; // rssi & checkdigit
                    // check digit
                    bufIndex = 0; // reset for next invetory
                                  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                                  // add filter on EPC number only then disect if a new tag is found
                                  /*if (tagReadFlags.readKraalTag == true)
                                  {
                                      if (localEPCBuf[0] != 0xBB) // not a valid sgtin nr
                                      {
                                          break;
                                      }
                                      // cPrintf("PCCount:%04X",pcCount);
                                  }
                                  if (tagReadFlags.readEPCTag == true)
                                  {
                                      if (localEPCBuf[0] != 0x30 && localEPCBuf[1] != 0x09) // not a valid sgtin nr
                                      {
                                          break;
                                      }
                                      // cPrintf("PCCount:%04X",pcCount);
                                  }*/
                    if (pcCount == 0x1000)
                    {
                        if (tagReadFlags.readKraalTag == true)
                        {
                            if (localEPCBuf[0] != 0xBB) // not a valid kraal tag
                            {
                                // do nothing
                            }
                            else
                            {
                                if (filter((uint8_t *)localEPCBuf, EPC96BITS, 1) == NOT_IN_LIST) // false tag was not in list always try 12 bytes
                                {
                                    for (uint8_t temp = 0; temp < EPC32BITS; temp++) // read 4 epc bytes
                                    {
                                        cPrintf("%02X", localEPCBuf[temp]);
                                    }
                                    ret = 1;
                                    cPrintf("\r\n");
                                }
                            }
                        }
                    }
                    if (pcCount == 0x3000) // will be 0 for no tag read
                    {
                        if (filter((uint8_t *)localEPCBuf, EPC96BITS, 1) == NOT_IN_LIST) // false tag was not in list always try 12 bytes
                        {                                                                 // for now only 1 antenna; 0 not reported; 1 reported
                            if (tagReadFlags.countTags == true)
                            {
                                cPrintf("%d-", tagReadFlags.tagKeepCount);
                            }
                            if (tagReadFlags.showRawEPC == true) // print RAW EPC
                            {
                                for (uint8_t temp = 0; temp < EPC96BITS; temp++) // read 12 epc bytes
                                {
                                    cPrintf("%02X", localEPCBuf[temp]);
                                }
                                cPrintf("\r\n");
                            }
                            else // disect into SGTIN
                            {
                                if (tagReadFlags.readEPCTag == true)
                                {
                                    disectEpcTagNumber((uint8_t *)localEPCBuf);
                                }
                            }
                        }
                        else
                        {
                            // if(!checkFilterTo())
                            //{
                            // deleteTag((uint8_t *)localEPCBuf);
                            //    exitFilter();
                            // }
                        }
                    }
                }
                break;
            default:
                // cPrintf("Device ID2:%d index[%x]\r\n", deviceID, rbBuffer[bufIndex]);
                break;
            }
        default:
            break;
        }
    }
    return ret;
}
//===================================================================================
void GetInventoryTagCount(uint8_t *rbBuffer, uint16_t dataSize)
{

    uint8_t localBuffer[50];
    uint8_t tmp = 0;
    memset(localBuffer, 0, sizeof(localBuffer)); // always flush buffer

    //----------------------------------------------------------------------
    // now send command to read buffer
    localBuffer[tmp++] = 0xA0;
    localBuffer[tmp++] = 0x03;
    localBuffer[tmp++] = 0x01; // make 0xff to get correct length
    localBuffer[tmp++] = 0x92;
    localBuffer[tmp] = CheckSum(localBuffer, tmp);
    tmp++;
    rbprintfRAW((char *)localBuffer, tmp);
    // vTaskDelay(100 / portTICK_PERIOD_MS);
    while (rxDataS2.dataAvailble == false)
    { // wait for response
        rtc_wdt_feed();
    }
    rbResponse((uint8_t *)rxDataS2.dataBuf, rxDataS2.dataCount);
}
//===================================================================================
void ResetInventoryBuffer(uint8_t *rbBuffer, uint16_t dataSize)
{

    uint8_t localBuffer[50];
    uint8_t tmp = 0;
    memset(localBuffer, 0, sizeof(localBuffer)); // always flush buffer
    localBuffer[tmp++] = 0xA0;
    localBuffer[tmp++] = 0x03;
    localBuffer[tmp++] = 0x01; // make 0xff to get correct length
    localBuffer[tmp++] = 0x93;
    localBuffer[tmp] = CheckSum(localBuffer, tmp);
    tmp++;
    rbprintfRAW((char *)localBuffer, tmp);
    // vTaskDelay(100 / portTICK_PERIOD_MS);
    while (rxDataS2.dataAvailble == false)
    { // wait for response
        rtc_wdt_feed();
    }
    rbResponse((uint8_t *)rxDataS2.dataBuf, rxDataS2.dataCount);
}
//===================================================================================
void GetInventoryBuffer(uint8_t *rbBuffer, uint16_t dataSize)
{

    uint8_t localBuffer[50];
    uint8_t tmp = 0;
    memset(localBuffer, 0, sizeof(localBuffer)); // always flush buffer
    localBuffer[tmp++] = 0xA0;
    localBuffer[tmp++] = 0x03;
    localBuffer[tmp++] = 0x01; // make 0xff to get correct length
    localBuffer[tmp++] = 0x90;
    localBuffer[tmp] = CheckSum(localBuffer, tmp);
    tmp++;
    rbprintfRAW((char *)localBuffer, tmp);
    // vTaskDelay(100 / portTICK_PERIOD_MS);
    while (rxDataS2.dataAvailble == false)
    { // wait for response
        rtc_wdt_feed();
    }
    rbResponse((uint8_t *)rxDataS2.dataBuf, rxDataS2.dataCount);
}
//===================================================================================
void SetWorkAntenna(uint8_t *rbBuffer, uint16_t dataSize, uint8_t antenna)
{

    uint8_t localBuffer[50];
    uint8_t tmp = 0;
    memset(localBuffer, 0, sizeof(localBuffer)); // always flush buffer
    localBuffer[tmp++] = 0xA0;
    localBuffer[tmp++] = 0x04;
    localBuffer[tmp++] = 0x01; // make 0xff to get correct length
    localBuffer[tmp++] = 0x74;
    localBuffer[tmp++] = antenna;
    localBuffer[tmp] = CheckSum(localBuffer, tmp);
    tmp++;
    rbprintfRAW((char *)localBuffer, tmp);
    // vTaskDelay(100 / portTICK_PERIOD_MS);
    while (rxDataS2.dataAvailble == false)
    { // wait for response
        rtc_wdt_feed();
    }
    rbResponse((uint8_t *)rxDataS2.dataBuf, rxDataS2.dataCount);
}
//==================================================================================
int InventoryReadTagSession(uint8_t *rbBuffer, uint16_t dataSize, uint8_t session)
{
    uint8_t localBuffer[50];
    uint8_t tmp = 0;
    int invRet = -1;
    memset(localBuffer, 0, sizeof(localBuffer)); // always flush buffer
    localBuffer[tmp++] = 0xA0;
    localBuffer[tmp++] = 0x07;
    localBuffer[tmp++] = 0x01; // address always 1
    localBuffer[tmp++] = 0x8B;
    localBuffer[tmp++] = session; // session
    localBuffer[tmp++] = 0x00;    // target
    localBuffer[tmp++] = 0x00;    // SL
    localBuffer[tmp++] = 100;     // repeats
    localBuffer[tmp] = CheckSum(localBuffer, tmp);
    tmp++;
    rxDataS2.dataAvailble = false;
    rbprintfRAW((char *)localBuffer, tmp);
    // vTaskDelay((nrReads*10)/portTICK_PERIOD_MS);   //this is a blocking delay ???
    while (rxDataS2.dataAvailble == false)
    { // wait for response
        rtc_wdt_feed();
        vTaskDelay(1 / portTICK_PERIOD_MS);
        // cPrintf(".");
    }
    // rbResponse((uint8_t *)rbBuffer, dataSize);
    invRet = getRealtimeInventory((uint8_t *)rxDataS2.dataBuf, rxDataS2.dataCount, 100); // by default read 10 sessions

    return invRet;

    // rbResponse((uint8_t *)rxDataS2.dataBuf, rxDataS2.dataCount);
    //  getRBTags();                //special return tags read ~!!! do not use rbResponse
}
//==================================================================================
void ReadTagWithSession(uint8_t *rbBuffer, uint16_t dataSize, uint8_t session)
{
    uint8_t localBuffer[50];
    uint8_t tmp = 0;
    memset(localBuffer, 0, sizeof(localBuffer)); // always flush buffer
    localBuffer[tmp++] = 0xA0;
    localBuffer[tmp++] = 0x07;
    localBuffer[tmp++] = 0x01; // address always 1
    localBuffer[tmp++] = 0x8B;
    localBuffer[tmp++] = session; // session
    localBuffer[tmp++] = 0x00;    // target
    localBuffer[tmp++] = 0x00;    // SL
    localBuffer[tmp++] = 255;     // repeats
    localBuffer[tmp] = CheckSum(localBuffer, tmp);
    tmp++;
    rxDataS2.dataAvailble = false;
    rbprintfRAW((char *)localBuffer, tmp);
    // vTaskDelay((nrReads*10)/portTICK_PERIOD_MS);   //this is a blocking delay ???
    while (rxDataS2.dataAvailble == false)
    { // wait for response
        rtc_wdt_feed();
        vTaskDelay(1 / portTICK_PERIOD_MS);
        // cPrintf(".");
    }
    // rbResponse((uint8_t *)rbBuffer, dataSize);
    // getRealtimeInventory((uint8_t *)rxDataS2.dataBuf, rxDataS2.dataCount, 10); // by default read 10 sessions
    rbResponse((uint8_t *)rxDataS2.dataBuf, rxDataS2.dataCount);
    // getRBTags();                //special return tags read ~!!! do not use rbResponse
}
//==================================================================================
void InventoryReadTagRealTime(uint8_t *rbBuffer, uint16_t dataSize, uint8_t nrReads)
{
    uint8_t localBuffer[50];
    uint8_t tmp = 0;
    memset(localBuffer, 0, sizeof(localBuffer)); // always flush buffer
    localBuffer[tmp++] = 0xA0;
    localBuffer[tmp++] = 0x04;
    localBuffer[tmp++] = 0x01; // make 0xff to get correct length
    localBuffer[tmp++] = 0x89;
    localBuffer[tmp++] = nrReads;
    localBuffer[tmp] = CheckSum(localBuffer, tmp);
    tmp++;
    rxDataS2.dataAvailble = false;
    rbprintfRAW((char *)localBuffer, tmp);
    while (rxDataS2.dataAvailble == false)
    { // wait for response
        rtc_wdt_feed();
    }
    getRealtimeInventory((uint8_t *)rxDataS2.dataBuf, rxDataS2.dataCount, nrReads);
}
//==================================================================================
void SetReaderRegion(uint8_t *rbBuffer, uint16_t dataSize)
{
    uint8_t localBuffer[50];
    uint8_t tmp = 0;
    memset(localBuffer, 0, sizeof(localBuffer)); // always flush buffer
    localBuffer[tmp++] = 0xA0;
    localBuffer[tmp++] = 0x06;
    localBuffer[tmp++] = 0x01; // make 0xff to get correct length
    localBuffer[tmp++] = 0x78;
    localBuffer[tmp++] = 0x02; // region
    localBuffer[tmp++] = 0x00; // start freq
    localBuffer[tmp++] = 0x06; // stop freq
    localBuffer[tmp] = CheckSum(localBuffer, tmp);
    tmp++;
    rbprintfRAW((char *)localBuffer, tmp);
    // vTaskDelay(100 / portTICK_PERIOD_MS);
    while (rxDataS2.dataAvailble == false)
    { // wait for response
        rtc_wdt_feed();
    }
    rbResponse((uint8_t *)rxDataS2.dataBuf, rxDataS2.dataCount);
}
//==================================================================================
void GetReaderTxPower(uint8_t *rbBuffer, uint16_t dataSize)
{
    uint8_t localBuffer[50];
    uint8_t tmp = 0;
    memset(localBuffer, 0, sizeof(localBuffer)); // always flush buffer
    localBuffer[tmp++] = 0xA0;
    localBuffer[tmp++] = 0x03;
    localBuffer[tmp++] = 0x01; // make 0xff to get correct length
    localBuffer[tmp++] = 0x77;
    localBuffer[tmp] = CheckSum(localBuffer, tmp); // store checksum
    tmp++;                                         // add 1
    rbprintfRAW((char *)localBuffer, tmp);         // send command
                                                   // vTaskDelay(100 / portTICK_PERIOD_MS);
    while (rxDataS2.dataAvailble == false)
    { // wait for response
        rtc_wdt_feed();
    }
    rbResponse((uint8_t *)rxDataS2.dataBuf, rxDataS2.dataCount);
}
//==================================================================================
void SetReaderTxPower(uint8_t *rbBuffer, uint16_t dataSize, uint8_t powerVal)
{
    uint8_t localBuffer[50];
    uint8_t tmp = 0;
    memset(localBuffer, 0, sizeof(localBuffer)); // always flush buffer
    localBuffer[tmp++] = 0xA0;
    localBuffer[tmp++] = 0x04;
    localBuffer[tmp++] = 0x01; // make 0xff to get correct length
    localBuffer[tmp++] = 0x76;
    localBuffer[tmp++] = powerVal;
    localBuffer[tmp] = CheckSum(localBuffer, tmp);
    tmp++;
    rbprintfRAW((char *)localBuffer, tmp);
    // vTaskDelay(100 / portTICK_PERIOD_MS);
    while (rxDataS2.dataAvailble == false)
    { // wait for response
        rtc_wdt_feed();
    }
    rbResponse((uint8_t *)rxDataS2.dataBuf, rxDataS2.dataCount);
}
//==================================================================================
void GetReaderTemperature(uint8_t *rbBuffer, uint16_t dataSize)
{
    uint8_t localBuffer[20];
    uint8_t tmp = 0;
    memset(localBuffer, 0, sizeof(localBuffer));   // always flush buffer
    localBuffer[tmp++] = 0xA0;                     // 0>>
    localBuffer[tmp++] = 0x03;                     // 1>>
    localBuffer[tmp++] = 0x01;                     // 2>>
    localBuffer[tmp++] = 0x7B;                     // 3>>
    localBuffer[tmp] = CheckSum(localBuffer, tmp); // 4
    tmp++;                                         // add 1 for byte 0
    rbprintfRAW((char *)localBuffer, tmp);
    // vTaskDelay(100 / portTICK_PERIOD_MS);
    while (rxDataS2.dataAvailble == false)
    { // wait for response
        rtc_wdt_feed();
    }
    rbResponse((uint8_t *)rxDataS2.dataBuf, rxDataS2.dataCount);
}
//=================================================================================
void GetFirmwareVersion(uint8_t *rbBuffer, uint16_t dataSize)
{
    uint8_t localBuffer[20];
    uint8_t tmp = 0;
    memset(localBuffer, 0, sizeof(localBuffer)); // always flush buffer
    localBuffer[tmp++] = 0xA0;
    localBuffer[tmp++] = 0x03;
    localBuffer[tmp++] = 0x01; // always reader 1
    localBuffer[tmp++] = 0x72;
    localBuffer[tmp] = CheckSum(localBuffer, tmp);
    tmp++;
    rbprintfRAW((char *)localBuffer, tmp);
    // vTaskDelay(10 / portTICK_PERIOD_MS);

    //   cPrintf("(%x)", rxDataS2.dataAvailble);
    while (rxDataS2.dataAvailble == false)
    { // wait for response
        rtc_wdt_feed();
        // vTaskDelay(1 / portTICK_PERIOD_MS);
    }
    // cPrintf("Rx availble\r\n");
    rbResponse((uint8_t *)rxDataS2.dataBuf, rxDataS2.dataCount);
}
//==================================================================================
void InventoryCommand(uint8_t *rbBuffer, uint16_t dataSize, uint8_t nrReads)
{
    uint8_t localBuffer[50];
    uint8_t tmp = 0;
    memset(localBuffer, 0, sizeof(localBuffer)); // always flush buffer
    localBuffer[tmp++] = 0xA0;
    localBuffer[tmp++] = 0x04;
    localBuffer[tmp++] = 0x01; // make 0xff to get correct length
    localBuffer[tmp++] = 0x80;
    localBuffer[tmp++] = nrReads;
    localBuffer[tmp] = CheckSum(localBuffer, tmp);
    tmp++;
    rbprintfRAW((char *)localBuffer, tmp);
    // vTaskDelay(100 / portTICK_PERIOD_MS);
    while (rxDataS2.dataAvailble == false)
    { // wait for response
        rtc_wdt_feed();
    }
    rbResponse((uint8_t *)rxDataS2.dataBuf, rxDataS2.dataCount);
}
//===================================================================================
void checkForErrors(uint8_t errorCode)
{
    switch (errorCode)
    {
    case 0x10:
        cPrintf("Command OK\r\n");
        break;
    default: // error by default
        cPrintf("Command error:%x\r\n", errorCode);
        break;
    }
}
//===================================================================================
int rbResponse(uint8_t *rbBuffer, uint16_t dataSize)
{
    uint16_t bufIndex = 0;
    uint8_t nrOfBytes = 0;
    uint8_t deviceID = 0;
    uint8_t command = 0;
    uint8_t len = 0;
    uint8_t freqAnt = 0;
    uint16_t pcCount = 0;
    uint8_t localEPCBuf[EPC96BITS];
    uint8_t rssi = 0;
    uint8_t checkDigit = 0;
    // ESP_LOGI("RB response->", "%d", dataSize);
    // cPrintf("-Response-\r\n");
    switch (rbBuffer[bufIndex++]) // always start at buf zero
    {
    case 0xA0: // start of frame
        len = rbBuffer[bufIndex++];
        nrOfBytes = len;
        deviceID = rbBuffer[bufIndex++];
        nrOfBytes--;
        command = rbBuffer[bufIndex++];
        nrOfBytes--;
        // ESP_LOGI("RB cmd:", "%02x", command);
        // cPrintf("\r\nResponse=%x-%x-%x\r\n", nrOfBytes, deviceID, command);

        switch (command)
        {
        case 0x72: // no error code returned
            // head len id cmd major minor check
            cPrintf("Version: %d%d\r\n", rbBuffer[4], rbBuffer[5]);
            break;
        case 0x74:
            checkForErrors(rbBuffer[4]);
            break;
        case 0x75: // no error code returned
            while (nrOfBytes-- != 1)
            { // skip last one as its check digit
              // cPrintf("%d:%02x ", nrOfBytes, rbBuffer[bufIndex++]);
            }
            break;
        case 0x76:                       // set tx power
            checkForErrors(rbBuffer[4]); // check for errors
            break;
        case 0x77: // get tx power
            if (len == 4)
            { // 4 bytes returned
                cPrintf("PWR=(1)%02d\r\n", rbBuffer[4]);
            }
            else
            { // 7 bytes returned
                cPrintf("More than 1 antenna power");
            }
            break;
        case 0x78:                       // set tx power
            checkForErrors(rbBuffer[4]); // check for errors
            break;
        case 0x7B:
            cPrintf("T=%02d\r\n", rbBuffer[5]);
            break;
        case 0x80:
            cPrintf("Nr of Bytes 0x80:%d:", rbBuffer[bufIndex++]);
            while (nrOfBytes-- != 1)
            { // skip last one as its check digit
                cPrintf("%02x ", rbBuffer[bufIndex++]);
            }
            break;
        case 0x89:
            cPrintf("Nr of Bytes 0x89:%d:", len);
            while (nrOfBytes-- != 1)
            { // skip last one as its check digit
                cPrintf("%02x ", rbBuffer[bufIndex++]);
            }
            break;
        case 0x90:
            checkForErrors(rbBuffer[4]);
            cPrintf("Nr of Bytes:%d:", len);
            while (nrOfBytes-- != 1)
            { // skip last one as its check digit
                cPrintf("%02x ", rbBuffer[bufIndex++]);
            }
            break;
        case 0x92:
            checkForErrors(rbBuffer[4]);
            cPrintf("Nr of Bytes:%d:", len);
            while (nrOfBytes-- != 1)
            { // skip last one as its check digit
                cPrintf("%02x ", rbBuffer[bufIndex++]);
            }
            break;
        case 0x93:
            checkForErrors(rbBuffer[4]);
            cPrintf("Nr of Bytes:%d:", len);
            while (nrOfBytes-- != 1)
            { // skip last one as its check digit
                cPrintf("%02x ", rbBuffer[bufIndex++]);
            }
            break;
        case 0x8B: // read realtime inventory
            // cPrintf("Lenght of buffer:%d\r\n", len);
            while (nrOfBytes) // repeat for each inventory
            {
                freqAnt = rbBuffer[bufIndex++];
                nrOfBytes--;
                pcCount = (uint16_t)(rbBuffer[bufIndex]) << 8 | rbBuffer[bufIndex + 1]; // read PC word
                bufIndex += 2;
                nrOfBytes -= 2;
                for (len = 0; len < EPC96BITS; len++) // read 12 epc bytes
                {
                    localEPCBuf[len] = rbBuffer[bufIndex++];
                    // cPrintf("L2=%02x ", localEPCBuf[len]);
                    nrOfBytes--;
                }
                rssi = rbBuffer[bufIndex++];
                checkDigit = rbBuffer[bufIndex++];  // read check digit move to next index
                vTaskDelay(5 / portTICK_PERIOD_MS); // wait for buffer to fill ?????
                // cPrintf("Rssi:%x[%x]\r\n",rssi, checkDigit);
                nrOfBytes = 0; // rssi & checkdigit
                // check digit
                bufIndex = 0; // reset for next invetory
                if (filter((uint8_t *)localEPCBuf, len, 1) == NOT_IN_LIST)
                { // for now only 1 antenna; 0 not reported; 1 reported
                    disectEpcTagNumber((uint8_t *)localEPCBuf);
                }
                else
                    cPrintf("\r\nTag filtered");
                // disectEpcTagNumber((uint8_t *)localEPCBuf);
            }
            break;

        default:
            cPrintf("No Command support\r\n");
            break;
        }
        break; // 0xA0
    default:
        cPrintf("Error for device:%d\r\n", deviceID);
        break;
    }

    return 0;
}
//===================================================================================
int rbResponseRAW(uint8_t *rbBuffer, uint16_t dataSize)
{

    uint16_t bufIndex = 0;
    uint8_t nrOfBytes = 0;
    uint8_t deviceID = 0;
    uint8_t command = 0;
    uint8_t len = 0;
    uint8_t freqAnt = 0;
    uint16_t pcCount = 0;
    uint8_t localEPCBuf[EPC96BITS];
    uint64_t vizNum = 0;
    uint8_t rssi = 0;
    uint8_t checkDigit = 0;
    cPrintf("Raw disect\r\n");
    switch (rbBuffer[bufIndex++]) // always start at buf zero
    {
    case 0xA0:                      // start of frame
        len = rbBuffer[bufIndex++]; // tag buffer lenght including checksum
        nrOfBytes = len;
        deviceID = rbBuffer[bufIndex++];
        nrOfBytes--;
        command = rbBuffer[bufIndex++]; // will be cmd as requested
        nrOfBytes--;
        // cPrintf("Len:%d",len);
        switch (command) //
        {
        case 0x89: // realtime inventory
        case 0x8B: // with session
            // cPrintf("Lenght of buffer:%d\r\n", len);
            memset(localEPCBuf, 0, sizeof(localEPCBuf));
            while (nrOfBytes) // repeat for each inventory
            {
                freqAnt = rbBuffer[bufIndex++];
                nrOfBytes--;
                pcCount = (uint16_t)(rbBuffer[bufIndex]) << 8 | rbBuffer[bufIndex + 1]; // read PC word
                bufIndex += 2;
                nrOfBytes -= 2;
                for (len = 0; len < EPC96BITS; len++) // read 12 epc bytes
                {
                    localEPCBuf[len] = rbBuffer[bufIndex++];
                    // cPrintf("%02x ", localEPCBuf[len]);
                    nrOfBytes--;
                }
                rssi = rbBuffer[bufIndex++];
                checkDigit = rbBuffer[bufIndex++];  // read check digit move to next index
                vTaskDelay(5 / portTICK_PERIOD_MS); // wait for buffer to fill ?????
                // cPrintf("Rssi:%x[%x]\r\n",rssi, checkDigit);
                nrOfBytes = 0; // rssi & checkdigit
                // check digit
                bufIndex = 0; // reset for next invetory
                //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                // uu dddd nnnnn
                if (pcCount) // will be 0 for no tag read
                {
                    for (uint8_t temp = 0; temp < EPC96BITS; temp++) // read 12 epc bytes
                    {
                        cPrintf("%02X", localEPCBuf[temp]);
                    }
                    cPrintf("\r\n");
                    // todo
                    disectEpcTagToBuf((uint8_t *)localEPCBuf, &vizNum); // disect epc into subcomponents here
                    cPrintf("Viznum:%lX\r\n", vizNum);
                }
                else
                {
                    cPrintf("Error No tag found");
                }
            }
        }
        break;

    default:
        // cPrintf("Device ID1:%d\r\n", deviceID);
        break;
    }
    return 0;
}