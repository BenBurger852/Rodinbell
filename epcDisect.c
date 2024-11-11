#include "include/rBell.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "filterTags.h"
// #include "c:\Espressif\frameworks\esp-idf-v5.1\components\esp_hw_support\include\rtc_wdt.h"
#include "rtc_wdt.h"
extern volatile uint32_t globalFilterVariable;

extern int cPrintf(const char *fmt, ...);
extern void stopTimer();
extern void startTimer();
//======================================================
typedef union
{
    struct
    {
        uint8_t EPCHeader; // full uint8_t
        uint8_t Filter : 3;
        uint8_t Partition : 3;
        uint8_t GCP[12];
        uint8_t IND[3];
        uint8_t SN[10];
        uint64_t VisualNum;
        uint64_t SGLNNum;
    };
} GS1_BITS;

GS1_BITS gs1Bits;

//=================================================================================
void showVizNum(void)
{
    // dprintf("%05x\r\n",gs1Bits.VisualNum);
    // wifiBtPrintf("%05x\r\n",gs1Bits.VisualNum);
}

// uint8_t tempTagBuffer[12] = {0x30,0x02,0x2E,0xF2,0x82,0xF4,0x9C,0x1C,0x75,0x7C,0xD6,0xC1};
uint8_t disectEpcTagNumber(uint8_t *tagBuffer)
{
    // uint8_t tempTagBuffer[12] = {0x30,0x02,0x2E,0xF2,0x82,0xF4,0x9C,0x1C,0x75,0x7C,0xD6,0xC1};
    // char tempTagBuffer[30]; // = {0x30,0x09,0x66,0x37,0x7F,0x6C,0x02,0xD9,0x60,0xEB,0x10,0x94};
    //*tempTagBuffer = &tagBuffer[0];
    static uint8_t keepTagFiltCount = 0;
    int x = 0;
    uint8_t ret = 0;
    uint8_t GCPbits = 0;
    uint8_t GCPdigits = 0;
    uint8_t IDRbits = 0;
    //    uint8_t LOCRef = 0;
    uint8_t EXTention = 0;
    uint8_t IPRdigits = 0;
    uint8_t SNLenght = 0;
    uint8_t temp = 0;
    uint8_t temp1 = 0;
    uint8_t temp2 = 0;
    uint8_t shiftVal = 0;
    uint8_t leadingBits = 0;
    uint8_t trailingBits = 0;
    uint8_t SNDigits = 0;
    uint8_t GCPDigits = 0;
    // uint8_t innerLoop=0;
    uint8_t index = 0;
    unsigned long long xx, xx0, xx1, xx2, xx3, xx4, xx5 = 0; // must be 64 bits
    uint8_t gTemp = 0;
    switch (tagBuffer[0]) // filter on GS type according to header; make sure its a SGTIN
    {
    case 0x30: // sgtin to be read check opposites SGTIN & SGLN
        // if(tagRxbufStats.readSGLN == 1)         //not a valid read we must read SGTIN
        // {
        // dprintf("ERROR: Got SGTIN not SGLN tag\r\n");
        //     return 0;
        // }
        break;
    case 0x32: // sgln
               //  if(tagRxbufStats.readSGTIN == 1)         //not a valid read we must read SGTIN
               //  {
               // dprintf("ERROR: Got SGLN not SGTIN tag\r\n");
        //     return 0;
        // }
        break;
    default:
        // for(x=0;x<10;x++)
        // {
        // beepWithTo(50);
        // vTaskDelay(100/portTICK_PERIOD_MS);
        // EXT_LED_Toggle();
        // }
        // x=0;
        // while(tagBuffer[x] != 0)
        //    dprintf("%02X ",tagBuffer[x++]);
        // dprintf("\r\n");
        // return 0;
        break;
    }
    // type confirmed now filter
    switch (tagBuffer[0]) // filter on GS type
    {
    case 0x30: // FILTER SGTIN    on length of GCP
        gs1Bits.EPCHeader = tagBuffer[0];
        gs1Bits.Filter = (tagBuffer[1] & 0b11100000) >> 5;
        gs1Bits.Partition = (tagBuffer[1] & 0b00011100) >> 2;
        // GS1 starts with LSB bits left over
        switch (gs1Bits.Partition)
        {
        case 0: // 12 DIGITS
            GCPbits = 40;
            GCPdigits = 12;
            IDRbits = 4;
            IPRdigits = 0;

            break;
        case 1: // 11 digit GCP GTIN-12
            GCPbits = 37;
            GCPdigits = 11;
            IDRbits = 7;
            IPRdigits = 2;
            SNLenght = 38;
            leadingBits = 2;
            trailingBits = 3;
            //
            SNDigits = 6;
            GCPDigits = 6;
            cPrintf("urn:epc:tag:sgtin-96:0.");
            // dprintf("urn:epc:tag:sgtin-96:0.");
            // wifiBtPrintf("urn:epc:tag:sgtin-96:0.");
            xx0 = (long long)(tagBuffer[1] & 0b00000011) << (GCPbits - leadingBits);
            xx1 = (long long)tagBuffer[2] << (24 + trailingBits);
            xx2 = (long long)tagBuffer[3] << (16 + trailingBits);
            xx3 = (long long)tagBuffer[4] << (8 + trailingBits);
            xx4 = tagBuffer[5] << trailingBits; // 3bits at top last shift left
            xx5 = (tagBuffer[6] & 0b11100000) >> (leadingBits + trailingBits);

            xx = xx0;
            xx |= xx1;
            xx |= xx2;
            xx |= xx3;
            xx |= xx4;
            xx |= xx5;
            //======================================================
            // disect EPC bits in loop
            xx0 = 1e10;
            xx1 = 10;
            index = 0;
            while (xx0)
            {
                if (xx0)
                    gTemp = (uint8_t)(xx / xx0);
                else
                    gTemp = gTemp;
                gs1Bits.GCP[index] = gTemp << 4;
                xx = (xx - gTemp * xx0);
                xx0 = (xx0 / 10); // only way seem to work for long long
                if (xx0)
                    gTemp = (uint8_t)(xx / xx0);
                else
                    gTemp = gTemp;
                gs1Bits.GCP[index] |= gTemp;
                xx = (xx - gTemp * xx0);
                xx0 = xx0 / 10; // only way seem to work for long long
                index++;
                if (xx0 == 1)
                    break;
            }
            gTemp = (uint8_t)(xx);
            gs1Bits.GCP[index] = gTemp << 4;

            for (x = 0; x < GCPDigits; x++)
            {
                if (x == 5)
                {
                    cPrintf("%x", gs1Bits.GCP[x] >> 4); // last nibble is odd
                    //  dprintf("%x",gs1Bits.GCP[x]>>4);      //last nibble is odd
                    // wifiBtPrintf("%x",gs1Bits.GCP[x]>>4);      //last nibble is odd
                }
                else
                {
                    cPrintf("%02x", gs1Bits.GCP[x]); // last nibble is odd
                    //    dprintf("%02x",gs1Bits.GCP[x]);      //last nibble is odd
                    // wifiBtPrintf("%02x",gs1Bits.GCP[x]);      //last nibble is odd
                }
            }
            cPrintf(".");
            // dprintf(".");
            // wifiBtPrintf(".");
            //=============================================================================

            // now for indicator digits
            gs1Bits.IND[0] = 0x00;                    // always 0
            temp1 = tagBuffer[6] & 0b00011111;        // mask last 5 bits off [6];
            temp2 = (tagBuffer[7] & 0b11000000) >> 6; // mask top 2 bits; indicator must be 7 bits then shift right
            gs1Bits.IND[1] = (temp1 | temp2);
            cPrintf("%01d%01d.", gs1Bits.IND[0], gs1Bits.IND[1]);
            // dprintf("%01d%01d.",gs1Bits.IND[0],gs1Bits.IND[1]);
            // wifiBtPrintf("%01d%01d.",gs1Bits.IND[0],gs1Bits.IND[1]);
            xx0 = (long long)(tagBuffer[7] & 0b00111111) << (SNLenght - IDRbits + 1); // add 1 bit for bit 0
            xx1 = (long long)tagBuffer[8] << 24;
            xx2 = (long long)tagBuffer[9] << 16;
            xx3 = (long long)tagBuffer[10] << 8;
            xx4 = tagBuffer[11];

            xx = xx0;
            xx |= xx1;
            xx |= xx2;
            xx |= xx3;
            xx |= xx4;

            xx0 = 1e11;
            xx1 = 10;
            index = 0;
            while (xx0)
            {
                if (xx0)
                    gTemp = (uint8_t)(xx / xx0);
                else
                    gTemp = gTemp;
                gs1Bits.SN[index] = gTemp << 4;
                xx = (xx - gTemp * xx0);
                xx0 = (xx0 / 10); // only way seem to work for long long
                if (xx0)
                    gTemp = (uint8_t)(xx / xx0);
                else
                    gTemp = gTemp;
                gs1Bits.SN[index] |= gTemp;
                xx = (xx - gTemp * xx0);
                xx0 = xx0 / 10; // only way seem to work for long long
                index++;
                if (xx0 == 1)
                    break;
                // index++;
            }
            gTemp = (uint8_t)(xx);
            gs1Bits.SN[index] = gTemp << 4;

            for (x = 0; x < SNDigits; x++)
            { // 38bits 6 digits
                cPrintf("%02x", gs1Bits.SN[x]);
                // dprintf("%02x",gs1Bits.SN[x]);
                // wifiBtPrintf("%02x",gs1Bits.SN[x]);
            }
            gs1Bits.VisualNum = (gs1Bits.SN[3] & 0x0f) << 16 | gs1Bits.SN[4] << 8 | gs1Bits.SN[5];
            // beepWithTo((uint)1000);
            // if(!rbReaderStat.waterReader)
            // {
            // dprintf("\r\n");
            // if(sysFlags.lineFeed)
            //   wifiBtPrintf("\r\n");
            // else
            //    wifiBtPrintf("\r");
            // MOTOR_ON_OFF_SetHigh();
            // EXT_LED_SetHigh();
            // while(!msDelay(250));
            // EXT_LED_SetLow();
            // MOTOR_ON_OFF_SetLow();
            // beepWithTo(500);
            // }
            cPrintf("\r\n");
            break;
            // we use GCP 10 digits for now -------------------------------> We use this one
        case 2: // 10 digit GCP GTIN-13
            // TOTAL BITS = (47-3 = 44) - 34 = 10(INDICATOR)
            // Partition 2
            // GCP(bits 34 or digits 10)
            // INDICATOR(bits 10 or digits 3))
            // dprintf("urn:epc:tag:sgtin-96:0.");
            // wifiBtPrintf("urn:epc:tag:sgtin-96:0.");
            cPrintf("urn:epc:tag:sgtin-96:0.");
            GCPbits = 34;
            GCPDigits = 10;
            IDRbits = 10;
            IPRdigits = 2;
            SNLenght = 38;
            leadingBits = 2;
            trailingBits = 0;
            GCPdigits = 5;
            //==============================================================================
            // mask off header, filter and partition
            // GCP starts here
            // remove filter 3bits and partition 3 bits
            // this leaves last 2 bits
            xx0 = 0;
            xx0 = (long long)(tagBuffer[1] & 0b00000011) << (GCPbits - leadingBits);
            xx1 = (long long)tagBuffer[2] << (24 + trailingBits);
            xx2 = (long long)tagBuffer[3] << (16 + trailingBits);
            xx3 = (long long)tagBuffer[4] << (8 + trailingBits);
            xx4 = tagBuffer[5];

            xx = xx0;
            xx |= xx1;
            xx |= xx2;
            xx |= xx3;
            xx |= xx4;
            // xx |= xx5;
            // this give a decimal value of GCP
            //==============================================================================
            // Convert GCP to decimal xx contains GCP as hexadecimal
            // 10 digits
            xx0 = 1000000000; // 1e9;
            xx1 = 10;         // divisor
            index = 0;
            while (xx0)
            {
                if (xx0)
                    gTemp = (uint8_t)(xx / xx0);
                else
                    gTemp = gTemp;
                gs1Bits.GCP[index] = gTemp << 4;
                xx = (xx - gTemp * xx0);
                xx0 = xx0 / 10; // only way seem to work for long long
                if (xx0)
                    gTemp = (uint8_t)(xx / xx0);
                else
                    gTemp = gTemp;
                gs1Bits.GCP[index] |= gTemp;
                xx = (xx - gTemp * xx0);
                xx0 = xx0 / 10; // only way seem to work for long long
                index++;
                if (xx0 == 0)
                    break;
                // index++;
            }
            //==============================================================================
            for (x = 0; x < GCPdigits; x++)
            { // div by 2 each nibble =  1 digit
                cPrintf("%02x", gs1Bits.GCP[x]);
                // dprintf("%02x",gs1Bits.GCP[x]);
                // wifiBtPrintf("%02x",gs1Bits.GCP[x]);            //show as hex??
            }

            cPrintf(".");
            // dprintf(".");
            // wifiBtPrintf(".");
            //==============================================================================
            // now for indicator digits 3 digits but first is always 0
            // first always 0 as per spec
            // remember these are stored as digits NOT hex or binary
            xx0 = (tagBuffer[6] << 2) | (tagBuffer[7] & 0b11000000) >> 6;
            gs1Bits.IND[0] = (xx0 / 100); //((tagBuffer[6] & 0b11100000) >> 4);      //will always be 0 as per spec worst case is 99
            // indicator will be 10 bit but can`t fit in 1 uint8_t so must be split in 2 nibbles then
            // xx0 = (tagBuffer[6] & 0b00001111);      //0000 0010
            // xx1 = (tagBuffer[7] & 0b11000000);      //10xx xxxx
            // x = xx0|xx1;
            gs1Bits.IND[1] = (xx0 / 10);                  // ie 10/10 = 1
            gs1Bits.IND[2] = (xx0 - gs1Bits.IND[1] * 10); // ie 10-(10/10) = 0
            // digit will be gs1Bits.IND[1]*10 + gs1Bits.IND[2]
            // temp = gs1Bits.IND[1]*10+gs1Bits.IND[2];
            // dprintf("0%2d.",temp);     //pad with 0 then indicator 1 is decimal 0 to 99 !!!
            // wifiBtPrintf("%1d%1d%1d.",gs1Bits.IND[0],gs1Bits.IND[1],gs1Bits.IND[2]);     //pad with 0 then indicator 1 is decimal 0 to 99 !!!
            cPrintf("%1d%1d%1d.", gs1Bits.IND[0], gs1Bits.IND[1], gs1Bits.IND[2]);

            //==============================================================================
            // serial number is 38bits
            // 2 bits remain from tagBuffer7
            // xx0 =  (long long)(tagBuffer[7] & 0b00111111) << (32);
            xx0 = (long long)(tagBuffer[7] & 0b00111111) << 32; // split off first 2 bits
            xx1 = (long long)tagBuffer[8] << 24;
            xx2 = (long long)tagBuffer[9] << 16;
            xx3 = (long long)tagBuffer[10] << 8;
            xx4 = tagBuffer[11];

            xx = xx0;
            xx |= xx1;
            xx |= xx2;
            xx |= xx3;
            xx |= xx4;

            //======================================================
            // dicect EPC bits in loop
            // only 5 uint8_ts index

            gs1Bits.SN[0] = 0;
            gs1Bits.SN[1] = 0;
            gs1Bits.SN[2] = 0;
            gs1Bits.SN[3] = 0;
            gs1Bits.SN[4] = 0;
            gs1Bits.SN[5] = 0;
            gs1Bits.SN[6] = 0;
            gs1Bits.SN[7] = 0;
            gs1Bits.SN[8] = 0;
            gs1Bits.SN[9] = 0;
            // 12 digits - 1 = 1e11
            xx0 = 100000000000; //(long long)(10e11); //SN is 12 digits long use 12-1 = 100,000,000,000
            xx1 = 10;           // divisor
            index = 0;
            while (xx0)
            {
                if (xx0)
                    gTemp = (uint8_t)(xx / xx0);
                else
                    gTemp = gTemp;
                gs1Bits.SN[index] = gTemp << 4;
                xx = (xx - gTemp * xx0);
                xx0 = (xx0 / 10); // only way seem to work for long long
                if (xx0)          // avoid divide by 0
                    gTemp = (uint8_t)(xx / xx0);
                else
                    gTemp = gTemp;
                gs1Bits.SN[index] |= gTemp;
                xx = (xx - gTemp * xx0);
                xx0 = xx0 / 10; // only way seem to work for long long
                index++;
                if (xx0 == 1)
                    break;
            }
            gs1Bits.SN[index] |= xx;

            for (x = 0; x <= 5; x++)
            {
                // wifiBtPrintf("%02x",gs1Bits.SN[x]);          //show as hex (actually decimal)
                // dprintf("%02x",gs1Bits.SN[x]);
                cPrintf("%02x", gs1Bits.SN[x]);
            }

            gs1Bits.VisualNum = (gs1Bits.SN[3] & 0x0f) << 16 | gs1Bits.SN[4] << 8 | gs1Bits.SN[5];
            // beepWithTo((UINT)1000);
            /*  if(!rbReaderStat.waterReader)
              {
                  if(sysFlags.lineFeed)
                      wifiBtPrintf("\r\n");
                  else
                      wifiBtPrintf("\r");
                  MOTOR_ON_OFF_SetHigh();
                  EXT_LED_SetHigh();
                  while(!msDelay(250));
                  MOTOR_ON_OFF_SetLow();
                  EXT_LED_SetLow();
                  beepWithTo(500);
                 //dprintf("\r\n");
              }*/
            cPrintf("\r\n");

            ret = 1;
            break;
        case 3:
            GCPbits = 30;
            GCPdigits = 9;
            IDRbits = 14;
            IPRdigits = 4;
            break;
        case 4:
            GCPbits = 27;
            GCPdigits = 8;
            IDRbits = 17;
            IPRdigits = 5;
            break;
        case 5:
            GCPbits = 24;
            GCPdigits = 7;
            IDRbits = 20;
            IPRdigits = 6;
            break;
        case 6:
            GCPbits = 20;
            GCPdigits = 6;
            IDRbits = 24;
            IPRdigits = 7;
            break;
        default:
            GCPbits = 40;
            GCPdigits = 12;
            IDRbits = 4;
            IPRdigits = 1;
            break;
        }

        break;
    case 0x32: // sgln
        gs1Bits.Filter = (tagBuffer[1] & 0b11100000) >> 5;
        gs1Bits.Partition = (tagBuffer[1] & 0b00011100) >> 2;
        switch (gs1Bits.Partition)
        {
        case 0: // 12 DIGITS
            GCPbits = 40;
            GCPdigits = 12;
            IDRbits = 4;
            IPRdigits = 1;
            EXTention = 41;
            break;
        case 1: // 11 digit GCP GTIN-12
            GCPbits = 37;
            GCPdigits = 11;
            // LOCRef = 4;
            EXTention = 41;
            cPrintf("urn:epc:tag:sgln-96:0."); // filter always 0.
            // dprintf("urn:epc:tag:sgln-96:0.");     //filter always 0.
            // wifiBtPrintf("urn:epc:tag:sgln-96:0.");     //filter always 0.
            // xx =  (long long)(tagBuffer[1] & 0b00000011) << 32 | tagBuffer[2] << 24 | tagBuffer[3] << 16 | tagBuffer[4] << 8 | tagBuffer[5];
            //==============================================================
            xx0 = (long long)(tagBuffer[1] & 0b00000011) << (GCPbits - 2); // 2 = leading bits
            xx1 = (long long)tagBuffer[2] << (24 + 3);                     // 3 is training bits
            xx2 = (long long)tagBuffer[3] << (16 + 3);
            xx3 = (long long)tagBuffer[4] << (8 + 3);
            xx4 = (long long)tagBuffer[5] << 3; // 2 bits at top last shift left
            xx5 = (tagBuffer[6] & 0b11100000) >> 5;

            xx = xx0;
            xx |= xx1;
            xx |= xx2;
            xx |= xx3;
            xx |= xx4;
            xx |= xx5;
            // 11 digit gcp 5.5uint8_ts
            //======================================================
            // dicect EPC bits in loop
            // only 5.5 uint8_ts index
            xx0 = 1e10;
            xx1 = 10; // divisor
            index = 0;
            while (xx0)
            {
                gTemp = (uint8_t)(xx / xx0);
                gs1Bits.GCP[index] = gTemp << 4;
                xx = (xx - gTemp * xx0);
                xx0 = (xx0 / 10); // only way seem to work for long long
                gTemp = (uint8_t)(xx / xx0);
                gs1Bits.GCP[index] |= gTemp;
                xx = (xx - gTemp * xx0);
                xx0 = xx0 / 10; // only way seem to work for long long
                index++;
                if (xx0 == 1)
                    break;
            }
            gTemp = (uint8_t)(xx); // 11 digits
            gs1Bits.GCP[5] = gTemp << 4;

            for (x = 0; x < 6; x++)
            {
                // wifiBtPrintf("%02x",gs1Bits.GCP[x]);
                //  dprintf("%02x",gs1Bits.GCP[x]);
                cPrintf("%02x", gs1Bits.GCP[x]);
            }
            //  wifiBtPrintf(".");
            // dprintf(".");
            cPrintf(".");
            //==============================================================
            // now for location reference
            // 4 bits only
            // temp1 = tagBuffer[6];
            gs1Bits.IND[0] = (tagBuffer[6] & 0b00011110) >> 1; // mask off location
            // dprintf("%01d.",gs1Bits.IND[0]);       //can be only 0 to 9
            cPrintf("%01d.", gs1Bits.IND[0]); // can be only 0 to 9
            // wifiBtPrintf("%01d.",gs1Bits.IND[0]);       //can be only 0 to 9
            //==============================================================
            // Extension/serial number 41 bits
            shiftVal = (((EXTention / 8) - 1) * 8) + 1;
            xx0 = (long long)(tagBuffer[6] & 0b00000001) << shiftVal; // 41/8bits = 5 but -1 for uint8_t 0 + for leading bit
            xx1 = (long long)tagBuffer[7] << (32);
            xx2 = (long long)tagBuffer[8] << 24; // 0 is training bits
            xx3 = (long long)tagBuffer[9] << 16;
            xx4 = (long long)tagBuffer[10] << 8;
            xx5 = (long long)tagBuffer[11];
            xx = xx0;
            xx |= xx1;
            xx |= xx2;
            xx |= xx3;
            xx |= xx4;
            xx |= xx5;
            //======================================================
            // dicect EPC bits in loop
            // only 5.5 uint8_ts index
            gs1Bits.SN[0] = 0;
            gs1Bits.SN[1] = 0;
            gs1Bits.SN[2] = 0;
            gs1Bits.SN[3] = 0;
            gs1Bits.SN[4] = 0;
            gs1Bits.SN[5] = 0;
            gs1Bits.SN[6] = 0;
            gs1Bits.SN[7] = 0;
            gs1Bits.SN[8] = 0;
            gs1Bits.SN[9] = 0;

            xx0 = 1e11;
            xx1 = 10; // divisor
            index = 0;
            while (xx0)
            {
                gTemp = (uint8_t)(xx / xx0);
                gs1Bits.SN[index] = gTemp << 4;
                xx = (xx - gTemp * xx0);
                xx0 = (xx0 / xx1); // only way seem to work for long long
                gTemp = (uint8_t)(xx / xx0);
                gs1Bits.SN[index] |= gTemp;
                xx = (xx - gTemp * xx0);
                xx0 = xx0 / xx1; // only way seem to work for long long
                index++;
                if (xx0 == 1)
                    break;
            }
            gs1Bits.SN[index] |= xx; // last digit*/

            for (x = 0; x <= 5; x++)
            {
                // wifiBtPrintf("%02x",gs1Bits.SN[x]);
                // dprintf("%02x",gs1Bits.SN[x]);
                cPrintf("%02x", gs1Bits.SN[x]);
            }
            gs1Bits.SGLNNum = (long long)(gs1Bits.SN[2] & 0x0f) << 32 | (gs1Bits.SN[3] & 0x0f) << 16 | gs1Bits.SN[4] << 8 | gs1Bits.SN[5];
            // beepWithTo((UINT)1000);
            // if(!rbReaderStat.waterReader)
            // {
            // dprintf("\r\n");
            // if(sysFlags.lineFeed)
            //     wifiBtPrintf("\r\n");
            // else
            //    wifiBtPrintf("\r");
            // MOTOR_ON_OFF_SetHigh();
            // EXT_LED_SetHigh();
            // while(!msDelay(250));
            // EXT_LED_SetLow();
            // MOTOR_ON_OFF_SetLow();
            // beepWithTo(500);
            //}
            cPrintf("\r\n");
            ret = 1;
            break;
        case 2: // 10 digit GCP GTIN-13
            // TOTAL BITS = (47-3 = 44) - 34 = 10(INDICATOR)
            // wifiBtPrintf("urn:epc:tag:sgln-96:0.");     //filter always 0.
            // dprintf("urn:epc:tag:sgln-96:0.");     //filter always 0.
            // gprintf("urn:epc:tag:sgln-96:0.");     //filter always 0.
            cPrintf("urn:epc:tag:sgln-96:0."); // filter always 0.
            GCPbits = 34;                      // 10bit gcp
            GCPdigits = 10;
            IDRbits = 10;
            IPRdigits = 3;
            EXTention = 41;
            leadingBits = 2; // 2 leading

            // GCP 4 uint8_ts
            //==============================================================================
            xx0 = (long long)(tagBuffer[1] & 0b00000011) << (GCPbits - leadingBits); // 2 = leading bits
            xx1 = (long long)tagBuffer[2] << 24;                                     // 0 trailing bits
            xx2 = (long long)tagBuffer[3] << 16;
            xx3 = (long long)tagBuffer[4] << 8;
            xx4 = (long long)tagBuffer[5];
            xx = xx0;
            xx |= xx1;
            xx |= xx2;
            xx |= xx3;
            xx |= xx4;
            //============================================================================
            // disect EPC bits in loop
            // only 5.5 uint8_ts index
            xx0 = 1e9;
            xx1 = 10; // divisor
            index = 0;
            while (xx0)
            {
                gTemp = (uint8_t)(xx / xx0);
                gs1Bits.GCP[index] = gTemp << 4;
                xx = (xx - gTemp * xx0);
                xx0 = (xx0 / 10); // only way seem to work for long long
                gTemp = (uint8_t)(xx / xx0);
                gs1Bits.GCP[index] |= gTemp;
                xx = (xx - gTemp * xx0);
                xx0 = xx0 / 10; // only way seem to work for long long
                index++;
                if (xx0 == 1)
                    break;
            }
            gs1Bits.SN[index] |= xx; // last digit*/
            for (x = 0; x < 5; x++)
            {
                //   dprintf("%02x",gs1Bits.GCP[x]);
                //     wifiBtPrintf("%02x",gs1Bits.GCP[x]);
                cPrintf("%02x", gs1Bits.GCP[x]);
            }
            cPrintf(".");
            // dprintf(".");
            // wifiBtPrintf(".");
            // now for location reference
            // 44 - 3 = 41 3 partition
            // 44 - 3 - GCP(34) = 7 bits left for ref
            gs1Bits.IND[0] = 0x00; // always 0
            // location ref only 7 bits
            temp1 = tagBuffer[6];
            gs1Bits.IND[1] = (tagBuffer[6] & 0b01111111) >> 1; // last 7 bits only
            // wifiBtPrintf("0%01d.",gs1Bits.IND[1]);   //fisrt digit must be 0
            // dprintf("0%01d.",gs1Bits.IND[1]);   //fisrt digit must be 0
            cPrintf("0%01d.", gs1Bits.IND[1]); // fisrt digit must be 0

            //======================================================
            // disect EPC bits in loop
            // only 5.5 uint8_ts index
            shiftVal = (((EXTention / 8) - 1) * 8) + 1;
            xx0 = (long long)(tagBuffer[6] & 0b00000001) << shiftVal; // 41/8bits = 5 but -1 for uint8_t 0 + for leading bit
            xx1 = (long long)tagBuffer[7] << (32);
            xx2 = (long long)tagBuffer[8] << 24; // 0 is training bits
            xx3 = (long long)tagBuffer[9] << 16;
            xx4 = (long long)tagBuffer[10] << 8;
            xx5 = (long long)tagBuffer[11];
            xx = xx0;
            xx |= xx1;
            xx |= xx2;
            xx |= xx3;
            xx |= xx4;
            xx |= xx5;

            xx0 = 1e11;
            xx1 = 10; // divisor
            index = 0;
            while (xx0)
            {
                gTemp = (uint8_t)(xx / xx0);
                gs1Bits.SN[index] = gTemp << 4;
                xx = (xx - gTemp * xx0);
                xx0 = (xx0 / 10); // only way seem to work for long long
                gTemp = (uint8_t)(xx / xx0);
                gs1Bits.SN[index] |= gTemp;
                xx = (xx - gTemp * xx0);
                xx0 = xx0 / 10; // only way seem to work for long long
                index++;
                if (xx0 == 1)
                    break;
            }
            gs1Bits.SN[index] |= xx; // last digit*/
            for (x = 0; x <= 5; x++)
            {
                // wifiBtPrintf("%02x",gs1Bits.SN[x]);
                // dprintf("%02x",gs1Bits.SN[x]);
                cPrintf("%02x", gs1Bits.SN[x]);
            }
            gs1Bits.SGLNNum = (long long)(gs1Bits.SN[2] & 0x0f) << 32 | (gs1Bits.SN[3] & 0x0f) << 16 | gs1Bits.SN[4] << 8 | gs1Bits.SN[5];
            // beepWithTo(1000);
            /* if(!rbReaderStat.waterReader)
             {
                //dprintf("\r\n");
                 if(sysFlags.lineFeed)
                     wifiBtPrintf("\r\n");
                 else
                     wifiBtPrintf("\r");
                 MOTOR_ON_OFF_SetHigh();
                 EXT_LED_SetHigh();
                 while(!msDelay(250));
                 EXT_LED_SetLow();
                 MOTOR_ON_OFF_SetLow();
                 beepWithTo(500);
             }*/
            cPrintf("\r\n");
            ret = 1;
        }
        break;
    default:
        ret = 0;
        /* if(rbReaderStat.waterReader)
             dprintf("");
         else
             dprintf("ERROR:No tag found\r\n");*/
        // tagCount--;
        cPrintf("ERROR:Tag nr not VALID\r\n");
        // for(gTemp=0;gTemp<10;gTemp++)           //loop 10time no tag
        // {
        // beepWithTo(50);
        // while(!msDelay(100));
        // }
        // wifiBtPrintf("ERROR:No Tag detected\r\n");

        break;
    }
    return ret;
}
//===================================================
// add buffers to get sgtin; and serial nr
// must also add ability to reconstruct EPC nr with new value
uint8_t disectEpcTagToBuf(uint8_t *tagBuffer, uint64_t *vizNum)
{
    // uint8_t tempTagBuffer[12] = {0x30,0x02,0x2E,0xF2,0x82,0xF4,0x9C,0x1C,0x75,0x7C,0xD6,0xC1};
    // char tempTagBuffer[30]; // = {0x30,0x09,0x66,0x37,0x7F,0x6C,0x02,0xD9,0x60,0xEB,0x10,0x94};
    //*tempTagBuffer = &tagBuffer[0];
    static uint8_t keepTagFiltCount = 0;
    int x = 0;
    uint8_t ret = 0;
    uint8_t GCPbits = 0;
    uint8_t GCPdigits = 0;
    uint8_t IDRbits = 0;
    //    uint8_t LOCRef = 0;
    uint8_t EXTention = 0;
    uint8_t IPRdigits = 0;
    uint8_t SNLenght = 0;
    uint8_t temp = 0;
    uint8_t temp1 = 0;
    uint8_t temp2 = 0;
    uint8_t shiftVal = 0;
    uint8_t leadingBits = 0;
    uint8_t trailingBits = 0;
    uint8_t SNDigits = 0;
    uint8_t GCPDigits = 0;
    // uint8_t innerLoop=0;
    uint8_t index = 0;
    unsigned long long xx, xx0, xx1, xx2, xx3, xx4, xx5 = 0; // must be 64 bits
    uint8_t gTemp = 0;
    switch (tagBuffer[0]) // filter on GS type according to header; make sure its a SGTIN
    {
    case 0x30: // sgtin to be read check opposites SGTIN & SGLN
        // if(tagRxbufStats.readSGLN == 1)         //not a valid read we must read SGTIN
        // {
        // dprintf("ERROR: Got SGTIN not SGLN tag\r\n");
        //     return 0;
        // }
        break;
    case 0x32: // sgln
               //  if(tagRxbufStats.readSGTIN == 1)         //not a valid read we must read SGTIN
               //  {
               // dprintf("ERROR: Got SGLN not SGTIN tag\r\n");
        //     return 0;
        // }
        break;
    default:
        // for(x=0;x<10;x++)
        // {
        // beepWithTo(50);
        // vTaskDelay(100/portTICK_PERIOD_MS);
        // EXT_LED_Toggle();
        // }
        // x=0;
        // while(tagBuffer[x] != 0)
        //    dprintf("%02X ",tagBuffer[x++]);
        // dprintf("\r\n");
        // return 0;
        break;
    }
    // type confirmed now filter
    switch (tagBuffer[0]) // filter on GS type
    {
    case 0x30: // FILTER SGTIN    on length of GCP
        gs1Bits.EPCHeader = tagBuffer[0];
        gs1Bits.Filter = (tagBuffer[1] & 0b11100000) >> 5;
        gs1Bits.Partition = (tagBuffer[1] & 0b00011100) >> 2;
        // GS1 starts with LSB bits left over
        switch (gs1Bits.Partition)
        {
        case 0: // 12 DIGITS
            GCPbits = 40;
            GCPdigits = 12;
            IDRbits = 4;
            IPRdigits = 0;

            break;
        case 1: // 11 digit GCP GTIN-12
            GCPbits = 37;
            GCPdigits = 11;
            IDRbits = 7;
            IPRdigits = 2;
            SNLenght = 38;
            leadingBits = 2;
            trailingBits = 3;
            //
            SNDigits = 6;
            GCPDigits = 6;
            cPrintf("urn:epc:tag:sgtin-96:0.");
            // dprintf("urn:epc:tag:sgtin-96:0.");
            // wifiBtPrintf("urn:epc:tag:sgtin-96:0.");
            xx0 = (long long)(tagBuffer[1] & 0b00000011) << (GCPbits - leadingBits);
            xx1 = (long long)tagBuffer[2] << (24 + trailingBits);
            xx2 = (long long)tagBuffer[3] << (16 + trailingBits);
            xx3 = (long long)tagBuffer[4] << (8 + trailingBits);
            xx4 = tagBuffer[5] << trailingBits; // 3bits at top last shift left
            xx5 = (tagBuffer[6] & 0b11100000) >> (leadingBits + trailingBits);

            xx = xx0;
            xx |= xx1;
            xx |= xx2;
            xx |= xx3;
            xx |= xx4;
            xx |= xx5;
            //======================================================
            // disect EPC bits in loop
            xx0 = 1e10;
            xx1 = 10;
            index = 0;
            while (xx0)
            {
                if (xx0)
                    gTemp = (uint8_t)(xx / xx0);
                else
                    gTemp = gTemp;
                gs1Bits.GCP[index] = gTemp << 4;
                xx = (xx - gTemp * xx0);
                xx0 = (xx0 / 10); // only way seem to work for long long
                if (xx0)
                    gTemp = (uint8_t)(xx / xx0);
                else
                    gTemp = gTemp;
                gs1Bits.GCP[index] |= gTemp;
                xx = (xx - gTemp * xx0);
                xx0 = xx0 / 10; // only way seem to work for long long
                index++;
                if (xx0 == 1)
                    break;
            }
            gTemp = (uint8_t)(xx);
            gs1Bits.GCP[index] = gTemp << 4;

            for (x = 0; x < GCPDigits; x++)
            {
                if (x == 5)
                {
                    cPrintf("%x", gs1Bits.GCP[x] >> 4); // last nibble is odd
                    //  dprintf("%x",gs1Bits.GCP[x]>>4);      //last nibble is odd
                    // wifiBtPrintf("%x",gs1Bits.GCP[x]>>4);      //last nibble is odd
                }
                else
                {
                    cPrintf("%02x", gs1Bits.GCP[x]); // last nibble is odd
                    //    dprintf("%02x",gs1Bits.GCP[x]);      //last nibble is odd
                    // wifiBtPrintf("%02x",gs1Bits.GCP[x]);      //last nibble is odd
                }
            }
            cPrintf(".");
            // dprintf(".");
            // wifiBtPrintf(".");
            //=============================================================================

            // now for indicator digits
            gs1Bits.IND[0] = 0x00;                    // always 0
            temp1 = tagBuffer[6] & 0b00011111;        // mask last 5 bits off [6];
            temp2 = (tagBuffer[7] & 0b11000000) >> 6; // mask top 2 bits; indicator must be 7 bits then shift right
            gs1Bits.IND[1] = (temp1 | temp2);
            cPrintf("%01d%01d.", gs1Bits.IND[0], gs1Bits.IND[1]);
            // dprintf("%01d%01d.",gs1Bits.IND[0],gs1Bits.IND[1]);
            // wifiBtPrintf("%01d%01d.",gs1Bits.IND[0],gs1Bits.IND[1]);
            xx0 = (long long)(tagBuffer[7] & 0b00111111) << (SNLenght - IDRbits + 1); // add 1 bit for bit 0
            xx1 = (long long)tagBuffer[8] << 24;
            xx2 = (long long)tagBuffer[9] << 16;
            xx3 = (long long)tagBuffer[10] << 8;
            xx4 = tagBuffer[11];

            xx = xx0;
            xx |= xx1;
            xx |= xx2;
            xx |= xx3;
            xx |= xx4;

            xx0 = 1e11;
            xx1 = 10;
            index = 0;
            while (xx0)
            {
                if (xx0)
                    gTemp = (uint8_t)(xx / xx0);
                else
                    gTemp = gTemp;
                gs1Bits.SN[index] = gTemp << 4;
                xx = (xx - gTemp * xx0);
                xx0 = (xx0 / 10); // only way seem to work for long long
                if (xx0)
                    gTemp = (uint8_t)(xx / xx0);
                else
                    gTemp = gTemp;
                gs1Bits.SN[index] |= gTemp;
                xx = (xx - gTemp * xx0);
                xx0 = xx0 / 10; // only way seem to work for long long
                index++;
                if (xx0 == 1)
                    break;
                // index++;
            }
            gTemp = (uint8_t)(xx);
            gs1Bits.SN[index] = gTemp << 4;

            for (x = 0; x < SNDigits; x++)
            { // 38bits 6 digits
                cPrintf("%02x", gs1Bits.SN[x]);
                // dprintf("%02x",gs1Bits.SN[x]);
                // wifiBtPrintf("%02x",gs1Bits.SN[x]);
            }
            gs1Bits.VisualNum = (gs1Bits.SN[3] & 0x0f) << 16 | gs1Bits.SN[4] << 8 | gs1Bits.SN[5];
            // beepWithTo((uint)1000);
            // if(!rbReaderStat.waterReader)
            // {
            // dprintf("\r\n");
            // if(sysFlags.lineFeed)
            //   wifiBtPrintf("\r\n");
            // else
            //    wifiBtPrintf("\r");
            // MOTOR_ON_OFF_SetHigh();
            // EXT_LED_SetHigh();
            // while(!msDelay(250));
            // EXT_LED_SetLow();
            // MOTOR_ON_OFF_SetLow();
            // beepWithTo(500);
            // }
            cPrintf("\r\n");
            break;
            // we use GCP 10 digits for now -------------------------------> We use this one
        case 2: // 10 digit GCP GTIN-13
            // TOTAL BITS = (47-3 = 44) - 34 = 10(INDICATOR)
            // Partition 2
            // GCP(bits 34 or digits 10)
            // INDICATOR(bits 10 or digits 3))
            // dprintf("urn:epc:tag:sgtin-96:0.");
            // wifiBtPrintf("urn:epc:tag:sgtin-96:0.");
            cPrintf("urn:epc:tag:sgtin-96:0.");
            GCPbits = 34;
            GCPDigits = 10;
            IDRbits = 10;
            IPRdigits = 2;
            SNLenght = 38;
            leadingBits = 2;
            trailingBits = 0;
            GCPdigits = 5;
            //==============================================================================
            // mask off header, filter and partition
            // GCP starts here
            // remove filter 3bits and partition 3 bits
            // this leaves last 2 bits
            xx0 = 0;
            xx0 = (long long)(tagBuffer[1] & 0b00000011) << (GCPbits - leadingBits);
            xx1 = (long long)tagBuffer[2] << (24 + trailingBits);
            xx2 = (long long)tagBuffer[3] << (16 + trailingBits);
            xx3 = (long long)tagBuffer[4] << (8 + trailingBits);
            xx4 = tagBuffer[5];

            xx = xx0;
            xx |= xx1;
            xx |= xx2;
            xx |= xx3;
            xx |= xx4;
            // xx |= xx5;
            // this give a decimal value of GCP
            //==============================================================================
            // Convert GCP to decimal xx contains GCP as hexadecimal
            // 10 digits
            xx0 = 1000000000; // 1e9;
            xx1 = 10;         // divisor
            index = 0;
            while (xx0)
            {
                if (xx0)
                    gTemp = (uint8_t)(xx / xx0);
                else
                    gTemp = gTemp;
                gs1Bits.GCP[index] = gTemp << 4;
                xx = (xx - gTemp * xx0);
                xx0 = xx0 / 10; // only way seem to work for long long
                if (xx0)
                    gTemp = (uint8_t)(xx / xx0);
                else
                    gTemp = gTemp;
                gs1Bits.GCP[index] |= gTemp;
                xx = (xx - gTemp * xx0);
                xx0 = xx0 / 10; // only way seem to work for long long
                index++;
                if (xx0 == 0)
                    break;
                // index++;
            }
            //==============================================================================
            for (x = 0; x < GCPdigits; x++)
            { // div by 2 each nibble =  1 digit
                cPrintf("%02x", gs1Bits.GCP[x]);
                // dprintf("%02x",gs1Bits.GCP[x]);
                // wifiBtPrintf("%02x",gs1Bits.GCP[x]);            //show as hex??
            }

            cPrintf(".");
            // dprintf(".");
            // wifiBtPrintf(".");
            //==============================================================================
            // now for indicator digits 3 digits but first is always 0
            // first always 0 as per spec
            // remember these are stored as digits NOT hex or binary
            xx0 = (tagBuffer[6] << 2) | (tagBuffer[7] & 0b11000000) >> 6;
            gs1Bits.IND[0] = (xx0 / 100); //((tagBuffer[6] & 0b11100000) >> 4);      //will always be 0 as per spec worst case is 99
            // indicator will be 10 bit but can`t fit in 1 uint8_t so must be split in 2 nibbles then
            // xx0 = (tagBuffer[6] & 0b00001111);      //0000 0010
            // xx1 = (tagBuffer[7] & 0b11000000);      //10xx xxxx
            // x = xx0|xx1;
            gs1Bits.IND[1] = (xx0 / 10);                  // ie 10/10 = 1
            gs1Bits.IND[2] = (xx0 - gs1Bits.IND[1] * 10); // ie 10-(10/10) = 0
            // digit will be gs1Bits.IND[1]*10 + gs1Bits.IND[2]
            // temp = gs1Bits.IND[1]*10+gs1Bits.IND[2];
            // dprintf("0%2d.",temp);     //pad with 0 then indicator 1 is decimal 0 to 99 !!!
            // wifiBtPrintf("%1d%1d%1d.",gs1Bits.IND[0],gs1Bits.IND[1],gs1Bits.IND[2]);     //pad with 0 then indicator 1 is decimal 0 to 99 !!!
            cPrintf("%1d%1d%1d.", gs1Bits.IND[0], gs1Bits.IND[1], gs1Bits.IND[2]);

            //==============================================================================
            // serial number is 38bits
            // 2 bits remain from tagBuffer7
            // xx0 =  (long long)(tagBuffer[7] & 0b00111111) << (32);
            xx0 = (long long)(tagBuffer[7] & 0b00111111) << 32; // split off first 2 bits
            xx1 = (long long)tagBuffer[8] << 24;
            xx2 = (long long)tagBuffer[9] << 16;
            xx3 = (long long)tagBuffer[10] << 8;
            xx4 = tagBuffer[11];

            xx = xx0;
            xx |= xx1;
            xx |= xx2;
            xx |= xx3;
            xx |= xx4;

            //======================================================
            // dicect EPC bits in loop
            // only 5 uint8_ts index

            gs1Bits.SN[0] = 0;
            gs1Bits.SN[1] = 0;
            gs1Bits.SN[2] = 0;
            gs1Bits.SN[3] = 0;
            gs1Bits.SN[4] = 0;
            gs1Bits.SN[5] = 0;
            gs1Bits.SN[6] = 0;
            gs1Bits.SN[7] = 0;
            gs1Bits.SN[8] = 0;
            gs1Bits.SN[9] = 0;
            // 12 digits - 1 = 1e11
            xx0 = 100000000000; //(long long)(10e11); //SN is 12 digits long use 12-1 = 100,000,000,000
            xx1 = 10;           // divisor
            index = 0;
            while (xx0)
            {
                if (xx0)
                    gTemp = (uint8_t)(xx / xx0);
                else
                    gTemp = gTemp;
                gs1Bits.SN[index] = gTemp << 4;
                xx = (xx - gTemp * xx0);
                xx0 = (xx0 / 10); // only way seem to work for long long
                if (xx0)          // avoid divide by 0
                    gTemp = (uint8_t)(xx / xx0);
                else
                    gTemp = gTemp;
                gs1Bits.SN[index] |= gTemp;
                xx = (xx - gTemp * xx0);
                xx0 = xx0 / 10; // only way seem to work for long long
                index++;
                if (xx0 == 1)
                    break;
            }
            gs1Bits.SN[index] |= xx;
           *vizNum = gs1Bits.VisualNum;

            for (x = 0; x <= 5; x++)
            {
                // wifiBtPrintf("%02x",gs1Bits.SN[x]);          //show as hex (actually decimal)
                // dprintf("%02x",gs1Bits.SN[x]);
                cPrintf("%02x", gs1Bits.SN[x]);
            }

            gs1Bits.VisualNum = (gs1Bits.SN[3] & 0x0f) << 16 | gs1Bits.SN[4] << 8 | gs1Bits.SN[5];

            // beepWithTo((UINT)1000);
            /*  if(!rbReaderStat.waterReader)
              {
                  if(sysFlags.lineFeed)
                      wifiBtPrintf("\r\n");
                  else
                      wifiBtPrintf("\r");
                  MOTOR_ON_OFF_SetHigh();
                  EXT_LED_SetHigh();
                  while(!msDelay(250));
                  MOTOR_ON_OFF_SetLow();
                  EXT_LED_SetLow();
                  beepWithTo(500);
                 //dprintf("\r\n");
              }*/
            cPrintf("\r\n");

            ret = 1;
            break;
        case 3:
            GCPbits = 30;
            GCPdigits = 9;
            IDRbits = 14;
            IPRdigits = 4;
            break;
        case 4:
            GCPbits = 27;
            GCPdigits = 8;
            IDRbits = 17;
            IPRdigits = 5;
            break;
        case 5:
            GCPbits = 24;
            GCPdigits = 7;
            IDRbits = 20;
            IPRdigits = 6;
            break;
        case 6:
            GCPbits = 20;
            GCPdigits = 6;
            IDRbits = 24;
            IPRdigits = 7;
            break;
        default:
            GCPbits = 40;
            GCPdigits = 12;
            IDRbits = 4;
            IPRdigits = 1;
            break;
        }

        break;
    case 0x32: // sgln
        gs1Bits.Filter = (tagBuffer[1] & 0b11100000) >> 5;
        gs1Bits.Partition = (tagBuffer[1] & 0b00011100) >> 2;
        switch (gs1Bits.Partition)
        {
        case 0: // 12 DIGITS
            GCPbits = 40;
            GCPdigits = 12;
            IDRbits = 4;
            IPRdigits = 1;
            EXTention = 41;
            break;
        case 1: // 11 digit GCP GTIN-12
            GCPbits = 37;
            GCPdigits = 11;
            // LOCRef = 4;
            EXTention = 41;
            cPrintf("urn:epc:tag:sgln-96:0."); // filter always 0.
            // dprintf("urn:epc:tag:sgln-96:0.");     //filter always 0.
            // wifiBtPrintf("urn:epc:tag:sgln-96:0.");     //filter always 0.
            // xx =  (long long)(tagBuffer[1] & 0b00000011) << 32 | tagBuffer[2] << 24 | tagBuffer[3] << 16 | tagBuffer[4] << 8 | tagBuffer[5];
            //==============================================================
            xx0 = (long long)(tagBuffer[1] & 0b00000011) << (GCPbits - 2); // 2 = leading bits
            xx1 = (long long)tagBuffer[2] << (24 + 3);                     // 3 is training bits
            xx2 = (long long)tagBuffer[3] << (16 + 3);
            xx3 = (long long)tagBuffer[4] << (8 + 3);
            xx4 = (long long)tagBuffer[5] << 3; // 2 bits at top last shift left
            xx5 = (tagBuffer[6] & 0b11100000) >> 5;

            xx = xx0;
            xx |= xx1;
            xx |= xx2;
            xx |= xx3;
            xx |= xx4;
            xx |= xx5;
            // 11 digit gcp 5.5uint8_ts
            //======================================================
            // dicect EPC bits in loop
            // only 5.5 uint8_ts index
            xx0 = 1e10;
            xx1 = 10; // divisor
            index = 0;
            while (xx0)
            {
                gTemp = (uint8_t)(xx / xx0);
                gs1Bits.GCP[index] = gTemp << 4;
                xx = (xx - gTemp * xx0);
                xx0 = (xx0 / 10); // only way seem to work for long long
                gTemp = (uint8_t)(xx / xx0);
                gs1Bits.GCP[index] |= gTemp;
                xx = (xx - gTemp * xx0);
                xx0 = xx0 / 10; // only way seem to work for long long
                index++;
                if (xx0 == 1)
                    break;
            }
            gTemp = (uint8_t)(xx); // 11 digits
            gs1Bits.GCP[5] = gTemp << 4;

            for (x = 0; x < 6; x++)
            {
                // wifiBtPrintf("%02x",gs1Bits.GCP[x]);
                //  dprintf("%02x",gs1Bits.GCP[x]);
                cPrintf("%02x", gs1Bits.GCP[x]);
            }
            //  wifiBtPrintf(".");
            // dprintf(".");
            cPrintf(".");
            //==============================================================
            // now for location reference
            // 4 bits only
            // temp1 = tagBuffer[6];
            gs1Bits.IND[0] = (tagBuffer[6] & 0b00011110) >> 1; // mask off location
            // dprintf("%01d.",gs1Bits.IND[0]);       //can be only 0 to 9
            cPrintf("%01d.", gs1Bits.IND[0]); // can be only 0 to 9
            // wifiBtPrintf("%01d.",gs1Bits.IND[0]);       //can be only 0 to 9
            //==============================================================
            // Extension/serial number 41 bits
            shiftVal = (((EXTention / 8) - 1) * 8) + 1;
            xx0 = (long long)(tagBuffer[6] & 0b00000001) << shiftVal; // 41/8bits = 5 but -1 for uint8_t 0 + for leading bit
            xx1 = (long long)tagBuffer[7] << (32);
            xx2 = (long long)tagBuffer[8] << 24; // 0 is training bits
            xx3 = (long long)tagBuffer[9] << 16;
            xx4 = (long long)tagBuffer[10] << 8;
            xx5 = (long long)tagBuffer[11];
            xx = xx0;
            xx |= xx1;
            xx |= xx2;
            xx |= xx3;
            xx |= xx4;
            xx |= xx5;
            //======================================================
            // dicect EPC bits in loop
            // only 5.5 uint8_ts index
            gs1Bits.SN[0] = 0;
            gs1Bits.SN[1] = 0;
            gs1Bits.SN[2] = 0;
            gs1Bits.SN[3] = 0;
            gs1Bits.SN[4] = 0;
            gs1Bits.SN[5] = 0;
            gs1Bits.SN[6] = 0;
            gs1Bits.SN[7] = 0;
            gs1Bits.SN[8] = 0;
            gs1Bits.SN[9] = 0;

            xx0 = 1e11;
            xx1 = 10; // divisor
            index = 0;
            while (xx0)
            {
                gTemp = (uint8_t)(xx / xx0);
                gs1Bits.SN[index] = gTemp << 4;
                xx = (xx - gTemp * xx0);
                xx0 = (xx0 / xx1); // only way seem to work for long long
                gTemp = (uint8_t)(xx / xx0);
                gs1Bits.SN[index] |= gTemp;
                xx = (xx - gTemp * xx0);
                xx0 = xx0 / xx1; // only way seem to work for long long
                index++;
                if (xx0 == 1)
                    break;
            }
            gs1Bits.SN[index] |= xx; // last digit*/

            for (x = 0; x <= 5; x++)
            {
                // wifiBtPrintf("%02x",gs1Bits.SN[x]);
                // dprintf("%02x",gs1Bits.SN[x]);
                cPrintf("%02x", gs1Bits.SN[x]);
            }
            gs1Bits.SGLNNum = (long long)(gs1Bits.SN[2] & 0x0f) << 32 | (gs1Bits.SN[3] & 0x0f) << 16 | gs1Bits.SN[4] << 8 | gs1Bits.SN[5];
            // beepWithTo((UINT)1000);
            // if(!rbReaderStat.waterReader)
            // {
            // dprintf("\r\n");
            // if(sysFlags.lineFeed)
            //     wifiBtPrintf("\r\n");
            // else
            //    wifiBtPrintf("\r");
            // MOTOR_ON_OFF_SetHigh();
            // EXT_LED_SetHigh();
            // while(!msDelay(250));
            // EXT_LED_SetLow();
            // MOTOR_ON_OFF_SetLow();
            // beepWithTo(500);
            //}
            cPrintf("\r\n");
            ret = 1;
            break;
        case 2: // 10 digit GCP GTIN-13
            // TOTAL BITS = (47-3 = 44) - 34 = 10(INDICATOR)
            // wifiBtPrintf("urn:epc:tag:sgln-96:0.");     //filter always 0.
            // dprintf("urn:epc:tag:sgln-96:0.");     //filter always 0.
            // gprintf("urn:epc:tag:sgln-96:0.");     //filter always 0.
            cPrintf("urn:epc:tag:sgln-96:0."); // filter always 0.
            GCPbits = 34;                      // 10bit gcp
            GCPdigits = 10;
            IDRbits = 10;
            IPRdigits = 3;
            EXTention = 41;
            leadingBits = 2; // 2 leading

            // GCP 4 uint8_ts
            //==============================================================================
            xx0 = (long long)(tagBuffer[1] & 0b00000011) << (GCPbits - leadingBits); // 2 = leading bits
            xx1 = (long long)tagBuffer[2] << 24;                                     // 0 trailing bits
            xx2 = (long long)tagBuffer[3] << 16;
            xx3 = (long long)tagBuffer[4] << 8;
            xx4 = (long long)tagBuffer[5];
            xx = xx0;
            xx |= xx1;
            xx |= xx2;
            xx |= xx3;
            xx |= xx4;
            //============================================================================
            // disect EPC bits in loop
            // only 5.5 uint8_ts index
            xx0 = 1e9;
            xx1 = 10; // divisor
            index = 0;
            while (xx0)
            {
                gTemp = (uint8_t)(xx / xx0);
                gs1Bits.GCP[index] = gTemp << 4;
                xx = (xx - gTemp * xx0);
                xx0 = (xx0 / 10); // only way seem to work for long long
                gTemp = (uint8_t)(xx / xx0);
                gs1Bits.GCP[index] |= gTemp;
                xx = (xx - gTemp * xx0);
                xx0 = xx0 / 10; // only way seem to work for long long
                index++;
                if (xx0 == 1)
                    break;
            }
            gs1Bits.SN[index] |= xx; // last digit*/
            for (x = 0; x < 5; x++)
            {
                //   dprintf("%02x",gs1Bits.GCP[x]);
                //     wifiBtPrintf("%02x",gs1Bits.GCP[x]);
                cPrintf("%02x", gs1Bits.GCP[x]);
            }
            cPrintf(".");
            // dprintf(".");
            // wifiBtPrintf(".");
            // now for location reference
            // 44 - 3 = 41 3 partition
            // 44 - 3 - GCP(34) = 7 bits left for ref
            gs1Bits.IND[0] = 0x00; // always 0
            // location ref only 7 bits
            temp1 = tagBuffer[6];
            gs1Bits.IND[1] = (tagBuffer[6] & 0b01111111) >> 1; // last 7 bits only
            // wifiBtPrintf("0%01d.",gs1Bits.IND[1]);   //fisrt digit must be 0
            // dprintf("0%01d.",gs1Bits.IND[1]);   //fisrt digit must be 0
            cPrintf("0%01d.", gs1Bits.IND[1]); // fisrt digit must be 0

            //======================================================
            // disect EPC bits in loop
            // only 5.5 uint8_ts index
            shiftVal = (((EXTention / 8) - 1) * 8) + 1;
            xx0 = (long long)(tagBuffer[6] & 0b00000001) << shiftVal; // 41/8bits = 5 but -1 for uint8_t 0 + for leading bit
            xx1 = (long long)tagBuffer[7] << (32);
            xx2 = (long long)tagBuffer[8] << 24; // 0 is training bits
            xx3 = (long long)tagBuffer[9] << 16;
            xx4 = (long long)tagBuffer[10] << 8;
            xx5 = (long long)tagBuffer[11];
            xx = xx0;
            xx |= xx1;
            xx |= xx2;
            xx |= xx3;
            xx |= xx4;
            xx |= xx5;

            xx0 = 1e11;
            xx1 = 10; // divisor
            index = 0;
            while (xx0)
            {
                gTemp = (uint8_t)(xx / xx0);
                gs1Bits.SN[index] = gTemp << 4;
                xx = (xx - gTemp * xx0);
                xx0 = (xx0 / 10); // only way seem to work for long long
                gTemp = (uint8_t)(xx / xx0);
                gs1Bits.SN[index] |= gTemp;
                xx = (xx - gTemp * xx0);
                xx0 = xx0 / 10; // only way seem to work for long long
                index++;
                if (xx0 == 1)
                    break;
            }
            gs1Bits.SN[index] |= xx; // last digit*/
            for (x = 0; x <= 5; x++)
            {
                // wifiBtPrintf("%02x",gs1Bits.SN[x]);
                // dprintf("%02x",gs1Bits.SN[x]);
                cPrintf("%02x", gs1Bits.SN[x]);
            }
            gs1Bits.SGLNNum = (long long)(gs1Bits.SN[2] & 0x0f) << 32 | (gs1Bits.SN[3] & 0x0f) << 16 | gs1Bits.SN[4] << 8 | gs1Bits.SN[5];
            // beepWithTo(1000);
            /* if(!rbReaderStat.waterReader)
             {
                //dprintf("\r\n");
                 if(sysFlags.lineFeed)
                     wifiBtPrintf("\r\n");
                 else
                     wifiBtPrintf("\r");
                 MOTOR_ON_OFF_SetHigh();
                 EXT_LED_SetHigh();
                 while(!msDelay(250));
                 EXT_LED_SetLow();
                 MOTOR_ON_OFF_SetLow();
                 beepWithTo(500);
             }*/
            cPrintf("\r\n");
            ret = 1;
        }
        break;
    default:
        ret = 0;
        /* if(rbReaderStat.waterReader)
             dprintf("");
         else
             dprintf("ERROR:No tag found\r\n");*/
        // tagCount--;
        cPrintf("ERROR:Tag nr not VALID\r\n");
        // for(gTemp=0;gTemp<10;gTemp++)           //loop 10time no tag
        // {
        // beepWithTo(50);
        // while(!msDelay(100));
        // }
        // wifiBtPrintf("ERROR:No Tag detected\r\n");

        break;
    }
    return ret;
}
