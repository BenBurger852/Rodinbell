#ifndef __RBELL_DEINES__
#define __RBELL_DEFINES__


#ifdef __cplusplus
extern "C" {
#endif


#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"

uint8_t CheckSum(uint8_t *uBuff, uint8_t uBuffLen);
void GetFirmwareVersion(uint8_t *rbBuffer, uint16_t dataSize);
int rbResponse(uint8_t *rbBuffer, uint16_t dataSize);
void GetReaderTemperature(uint8_t *dataBuf, uint16_t dataSize);

void SetReaderTxPower(uint8_t *rbBuffer, uint16_t dataSize, uint8_t powerVal);
void GetReaderTxPower(uint8_t *rbBuffer, uint16_t dataSize);
void SetReaderRegion(uint8_t *rbBuffer, uint16_t dataSize);
void InventoryReadTagRealTime(uint8_t *rbBuffer, uint16_t dataSize, uint8_t nrReads);

void SetWorkAntenna(uint8_t *rbBuffer, uint16_t dataSize, uint8_t antenna);

int InventoryReadTagSession(uint8_t *rbBuffer, uint16_t dataSize, uint8_t nrReads);
void GetInventoryTagCount(uint8_t *rbBuffer, uint16_t dataSize);
void GetInventoryBuffer(uint8_t *rbBuffer, uint16_t dataSize);
void InventoryCommand(uint8_t *rbBuffer, uint16_t dataSize, uint8_t nrReads);
void ResetInventoryBuffer(uint8_t *rbBuffer, uint16_t dataSize);
void ReadTagWithSession(uint8_t *rbBuffer, uint16_t dataSize, uint8_t session);
int rbprintfRAW(char *tBuf, uint8_t sizeOfData);
int rbResponseRAW(uint8_t *rbBuffer, uint16_t dataSize);
int getRBTags(void);
void writeSgtinTag(void);


#ifdef __cplusplus
}
#endif


#endif /* __ESP_LOG_H__ */