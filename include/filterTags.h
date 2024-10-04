#ifndef FILTERTAGS_H
#define	FILTERTAGS_H

#include <stdio.h>
#include <stdbool.h>
bool filter(uint8_t *number, uint8_t length, uint8_t antenna);
void exitFilter(void);
bool checkFilterTo(void);
uint8_t initFilter(uint8_t mSec, uint16_t minVal);
//-----------------------------------------------------------
extern struct
{
    bool showRawEPC;
    bool countTags;
    int tagKeepCount;
    bool readKraalTag;
    bool readEPCTag;
} tagReadFlags;
#endif