#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <inttypes.h>
#include <ctype.h>
#include "include/errors.h"

#include "driver/gpio.h"
#include "include/rbCommandProc.h"
#include "include/uartFunctions.h"
#include "include/rBell.h"
#include "include/filterTags.h"
// #include "c:\\Espressif\\frameworks\\esp-idf-v5.1.1\\components\\esp_hw_support\\include\\rtc_wdt.h"
#include "rtc_wdt.h"
#include "freertos/FreeRTOS.h"

//==========================================================================
uint8_t dataBuf[BUF_SIZE];
// uint8_t dataBuf0[BUF_SIZE];
uint16_t dataSize = 0;
// extern bool dataAvailable;
// extern bool dataAvailable0;
extern volatile uint32_t globalFilterVariable; //*multiples of 100
uint32_t filterReloadValue = 0;                // set to 0 by default report only once
extern void stopTimer(void);
extern void startTimer(void);
extern void exitFilter(void);
extern uint8_t initFilter(uint8_t seconds, uint16_t minVal);
//=========================================================================
typedef int (*FN)(char *s);
#define P_NUMBER(x) (0x8000 + x) // plus maximum length
#define P_NUM 0x8000
#define P_HEX(x) (0x4000 + x) // plus maximum length
#define P_HX 0x4000
#define P_ASCII(x) (0x2000 + x) // plus maximum length
#define P_UPPER(x) (0x1000 + x) // Caps, plus maximum length
#define P_UP 0x1000
#define P_NONE 0x800   // nothing following
#define P_NONE_Q 0x400 // nothing or a ?
#define P_N_Q 0x200    // digit(s) and ?
#define P_N_E 0x100    // digit(s) and =
extern bool countTag;
//=======================================
static struct
{
    long commands; // commands processed
    long errors;
    long triggerCount;
    long writeCount;
} stats;

//=========================================================================
typedef struct
{
    char *command;
    uint8_t flag;
    FN func;
} COMMANDS;
//=============================================================================
extern struct
{
    uint8_t dataCount;
    uint8_t head;
    uint8_t tail;
    char dataBuf[128];
    char rxChar;
    bool dataAvailble;
} rxDataS0;
//=========================================================================
//  Description:
//  Parameters:    none
//  Returns:           non-zero for an error
//=======================================================================
static int setWorkAnt(char *s)
{
    int ret = -1;
    char *p;
    s = s;
    p = strtok(s, "=");
    if (p)
    {
        if ((atoi(p) <= 4))
        {
            SetWorkAntenna((uint8_t *)dataBuf, dataSize, (uint8_t)atoi(p));
        }
        else
        {
            ret = ERR_BAD_PARAMETERS;
        }
    }
    else
    {
        ret = ERR_BAD_PARAMETERS;
    }
    return ret;
}
//=========================================================================
//  Description:
//  Parameters:    none
//  Returns:           non-zero for an error
//=======================================================================
static int setTagCount(char *s)
{
    int ret = -1;
    // char *p;
    s = s;
    tagReadFlags.countTags = true;
    tagReadFlags.tagKeepCount = 0;
    return ret;
}
//=========================================================================
//  Description:
//  Parameters:    none
//  Returns:           non-zero for an error
//=======================================================================
static int clearTagCount(char *s)
{
    int ret = -1;
    char *p;
    s = s;
    tagReadFlags.countTags = false;
    return ret;
}
//=========================================================================
//  Description:
//  Parameters:    none
//  Returns:           non-zero for an error
//=======================================================================
static int setTagFilter(char *s)
{
    int ret = -1;
    char *p;
    s = s;
    p = strtok(s, "=");
    if (p)
    {
        if ((atoi(p) <= 9999))
        {
            // stopTimer();
            initFilter(atoi(p) / 10, 0);
            // startTimer();
        }
        else
        {
            ret = ERR_BAD_PARAMETERS;
        }
    }
    else
    {
        ret = ERR_BAD_PARAMETERS;
    }
    return ret;
}
//=========================================================================
//  Description:
//  Parameters:    none
//  Returns:           non-zero for an error
//=======================================================================
static int readTagBufferInventory(char *s)
{
    int ret = -1;
    s = s;
    ResetInventoryBuffer((uint8_t *)dataBuf, dataSize);
    InventoryCommand((uint8_t *)dataBuf, dataSize, 10);
    GetInventoryBuffer((uint8_t *)dataBuf, dataSize);
    return ret;
}
//=========================================================================
//  Description:
//  Parameters:    none
//  Returns:           non-zero for an error
//=======================================================================
static int readTagSession(char *s)
{
    int ret = -1;
    s = s;
    char *p;
    p = strtok(s, "=");
    if (p)
    {
        if ((atoi(p) <= 4))
        {
            InventoryReadTagSession((uint8_t *)dataBuf, dataSize, atoi(p));
        }
        else
        {
            ret = ERR_BAD_PARAMETERS;
        }
    }
    else
    {
        ret = ERR_BAD_PARAMETERS;
    }
    return ret;
}
//=========================================================================
//  Description:
//  Parameters:    none
//  Returns:           non-zero for an error
//=======================================================================
static int readTagOnce(char *s)
{
    int ret = -1;
    s = s;
    exitFilter();
    initFilter(0, 0);
    InventoryReadTagRealTime((uint8_t *)dataBuf, dataSize, 10);
    return ret;
}
//=========================================================================
//  Description:
//  Parameters:    none
//  Returns:           non-zero for an error
//=======================================================================
static int readRawEPC(char *s)
{
    int ret = -1;
    s = s;
    exitFilter();
    tagReadFlags.showRawEPC = true;
    InventoryReadTagRealTime((uint8_t *)dataBuf, dataSize, 10);
    tagReadFlags.showRawEPC = false;
    return ret;
}
//=========================================================================
//  Description:
//  Parameters:    none
//  Returns:           non-zero for an error
//=======================================================================
static int readTagOnceOnly(char *s)
{
    int ret = -1;
    s = s;
    exitFilter();
    InventoryReadTagRealTime((uint8_t *)dataBuf, dataSize, 1);
    return ret;
}
//=========================================================================
//  Description:
//  Parameters:    none
//  Returns:           non-zero for an error
//=======================================================================
static int readContinues(char *s)
{
    int ret = -1;
    s = s;
    char *p;
    bool breakOut = false;
    char localBuf[5];
    uint8_t x = 0;
    uint16_t timeoutVal = 0;
    p = strtok(s, "=");
    exitFilter();
    if (p)
    {
        if ((atoi(p) >= 100) && (atoi(p) <= 9999))
        {
            timeoutVal = atoi(p);
            tagReadFlags.readKraalTag = false;
            tagReadFlags.readEPCTag = true;
            while (breakOut == false)
            {
                vTaskDelay(timeoutVal / portTICK_PERIOD_MS);
                InventoryReadTagRealTime(NULL, 0, 10); // read 100 session for 100 tags
                                                       // vTaskDelay(atoi(p) / portTICK_PERIOD_MS);
                // rtc_wdt_feed();
                while ((rxDataS0.tail != rxDataS0.head))
                {
                    localBuf[x++] = rxDataS0.dataBuf[rxDataS0.tail++];
                    if (x >= sizeof(localBuf)) // failsafe just in case
                        x = 0;
                    if (rxDataS0.tail >= sizeof(rxDataS0.dataBuf))
                    {
                        rxDataS0.tail = 0;
                    }
                    if (strstr((const char *)localBuf, "q") || strstr((const char *)localBuf, "Q")) // test fro CR
                    {
                        breakOut = true;
                        cPrintf("\r\n");
                    }
                    if (strstr((const char *)localBuf, "c") || strstr((const char *)localBuf, "C")) // test fro CR
                    {
                        cPrintf("\r\n");
                        exitFilter();
                    }
                }
            }
            exitFilter();
        }
        else
        {
            ret = ERR_BAD_PARAMETERS;
        }
    }
    else
    {
        ret = ERR_BAD_PARAMETERS;
    }

    // memset(rxDataS0.dataBuf, 0, sizeof(rxDataS0.dataBuf));
    // rxDataS0.head = 0;

    // cPrintf("\r\n");
    return ret;
}
//=========================================================================
//  Description:
//  Parameters:    none
//  Returns:           non-zero for an error
//=======================================================================
static int readKraalTag(char *s)
{
    int ret = -1;
    int retInvertory = -1;
    s = s;
    //  char *p;
    // bool breakOut = false;
    //  char localBuf[5];
    uint16_t x = 0;
    // p = strtok(s, "=");
    initFilter(100, 10);
    exitFilter();
    // if (p)
    //  {
    // if ((atoi(p) >= 100) && (atoi(p) <= 9999))
    //  {
    tagReadFlags.readKraalTag = true;
    tagReadFlags.readEPCTag = false;
    
    // InventoryReadTagRealTime(NULL, 0, 100); // first read read 100 session for 100 tags
    while (x < 1000)
    {       
        x++;         
        retInvertory = InventoryReadTagSession(NULL, 0, 1);
        if(retInvertory == 1)
            break;
        vTaskDelay(10 / portTICK_PERIOD_MS); 
    }
    //}
    // memset(rxDataS0.dataBuf, 0, sizeof(rxDataS0.dataBuf));
    // rxDataS0.head = 0;

    // cPrintf("\r\n");
    return ret;
}
//=========================================================================
//  Description:
//  Parameters:    none
//  Returns:           non-zero for an error
//=======================================================================
static int readRawEPCCont(char *s)
{
    int ret = -1;
    s = s;
    char *p;
    bool breakOut = false;
    char localBuf[5];
    uint8_t x = 0;

    p = strtok(s, "=");
    exitFilter();

    if (p)
    {
        if ((atoi(p) >= 100) && (atoi(p) <= 9999))
        {
            tagReadFlags.showRawEPC = true;
            while (breakOut == false)
            {
                InventoryReadTagRealTime(NULL, 0, 10); // read 100 session for 100 tags
                                                       // vTaskDelay(atoi(p) / portTICK_PERIOD_MS);
                rtc_wdt_feed();
                while ((rxDataS0.tail != rxDataS0.head))
                {
                    localBuf[x++] = rxDataS0.dataBuf[rxDataS0.tail++];
                    if (x >= sizeof(localBuf)) // failsafe just in case
                        x = 0;
                    if (rxDataS0.tail >= sizeof(rxDataS0.dataBuf))
                    {
                        rxDataS0.tail = 0;
                    }
                    if (strstr((const char *)localBuf, "q") || strstr((const char *)localBuf, "Q")) // test fro CR
                    {
                        cPrintf("\r\n");
                        breakOut = true;
                    }
                    if (strstr((const char *)localBuf, "c") || strstr((const char *)localBuf, "C")) // test fro CR
                    {
                        cPrintf("\r\n");
                        exitFilter();
                    }
                }
            }
            tagReadFlags.showRawEPC = false;
            exitFilter();
        }
        else
        {
            ret = ERR_BAD_PARAMETERS;
        }
    }
    else
    {
        ret = ERR_BAD_PARAMETERS;
    }

    // memset(rxDataS0.dataBuf, 0, sizeof(rxDataS0.dataBuf));
    // rxDataS0.head = 0;

    // cPrintf("\r\n");
    return ret;
}
//=========================================================================
//  Description:
//  Parameters:    none
//  Returns:           non-zero for an error
//=======================================================================
static int readTagInv(char *s)
{
    int ret = -1;
    uint8_t tempCount = 0;
    uint32_t tempFiltVal = 0;
    char *p;
    s = s;
    p = strtok(s, "=");
    exitFilter();
    tempFiltVal = filterReloadValue;
    filterReloadValue = 0; // reset filter
    if (p)
    {
        if ((atoi(p) >= 1) && (atoi(p) <= 255))
        {
            tempCount = (uint8_t)atoi(p);
            while (tempCount--)
            {
                InventoryReadTagRealTime(NULL, 0, 100); // read 100 session to catch all tags
            }
        }
        else
        {
            ret = ERR_BAD_PARAMETERS;
        }
    }
    else
    {
        ret = ERR_BAD_PARAMETERS;
    }
    filterReloadValue = tempFiltVal; // relaod filter val
    return ret;
}
//=========================================================================
//  Description:
//  Parameters:    none
//  Returns:           non-zero for an error
//=======================================================================
static int setRBRegion(char *s)
{
    int ret = -1;
    s = s;

    SetReaderRegion((uint8_t *)dataBuf, dataSize);

    return ret;
}
//=========================================================================
//  Description:
//  Parameters:    none
//  Returns:           non-zero for an error
//=======================================================================
static int getRBPower(char *s)
{
    int ret = -1;
    s = s;

    GetReaderTxPower((uint8_t *)dataBuf, dataSize);

    return ret;
}
//=========================================================================
//  Description:
//  Parameters:    none
//  Returns:           non-zero for an error
//=======================================================================
static int setRBPower(char *s)
{
    int ret = -1;
    char *p;
    s = s;
    p = strtok(s, "=");
    if (p)
    {
        if ((atoi(p) >= 10) && (atoi(p) <= 33))
        {
            SetReaderTxPower((uint8_t *)dataBuf, dataSize, (uint8_t)atoi(p));
        }
        else
        {
            ret = ERR_BAD_PARAMETERS;
        }
    }
    else
    {
        ret = ERR_BAD_PARAMETERS;
    }
    return ret;
}

//=========================================================================
//  Description:
//  Parameters:    none
//  Returns:           non-zero for an error
//=======================================================================
static int getRBTemperature(char *s)
{
    int ret = -1;
    s = s;
    GetReaderTemperature((uint8_t *)dataBuf, dataSize);
    return ret;
}
//=========================================================================
//  Description:
//  Parameters:    none
//  Returns:           non-zero for an error
//=======================================================================
static int getRBVersion(char *s)
{
    int ret = -1;
    s = s;
    GetFirmwareVersion((uint8_t *)dataBuf, dataSize); // send buffer and lenght to function
    return ret;
}
//=========================================================================
//  Description:
//  Parameters:    none
//  Returns:           non-zero for an error
//=======================================================================
static int writeSGTINTag(char *s)
{
    int ret = -1;
    char *p;
    s = s;
    p = strtok(s, "=");
    if (p)
    {
        writeSgtinTag();

        /*if ((atoi(p) >= 10) && (atoi(p) <= 33))
        {
            SetReaderTxPower((uint8_t *)dataBuf, dataSize, (uint8_t)atoi(p));
        }
        else
        {
            ret = ERR_BAD_PARAMETERS;
        }*/
    }
    else
    {
        ret = ERR_BAD_PARAMETERS;
    }
    return ret;
}
//=========================================================================
//  Description:
//  Parameters:    none
//  Returns:           non-zero for an error
//=========================================================================
//============================================================================
static char *runHelp[] = {
    "?? - Get help",
    "GV? - Get RBELL FW version",
    "GT? - Get RBELL Temperature",
    "SPWR=xx - Set RBELL TX power",
    "GPWR? - Get RBELL TX power",
    "SREGION!",
    "SANT= - Set antenna, (do not us in 1 port modules)",
    "R= - Read nr of times, upto 255, no filter on tags, (reads 10 sessions at a time so tags can respond back) ",
    "R? - Read tag 10 times",
    "RS! - Read tag session, still tbd",
    "R! - Read tag one session only",
    "RR= - Read SGTIN continues, 10 to 1000mS, 'q' quit, 'c' clear filter on the fly",
    "RB! - do not use for now",
    "SF= - set filter in ms, 0 disable filter, 0 by deafult",
    "REPC! - Read EPC once; raw HEX",
    "REPCC= - Read EPC continues, 10 to 1000mS, 'q' quit, 'c' clear filter on the fly",
    "W=xxx - Write new sgtin nr",
    0};

//=============================================================================
static int getHELP(char *s)
{
    int ret = -1;
    char **text = runHelp;
    s = s;
    for (; *text; ++text)
    {
        cPrintf("%s\r\n", *text);
    }
    // dprintf("??{GRBV?{SRBA={GRBT?{GRBPWR?{SRBPWR={RRP={R={RB={R?{LF?{LFON{LFOFF{LFFLUSH{LFA=\r\n");
    return ret;
}

//=============================================================================
static const COMMANDS cmdTable[] =
    {
        {"??", 0, getHELP},
        {"GV?", 0, getRBVersion},
        {"GT?", 0, getRBTemperature},
        {"SPWR=", 5, setRBPower},
        {"GPWR?", 0, getRBPower},
        {"SREGION!", 0, setRBRegion},
        {"R=", 5, readTagInv},
        {"SANT=", 1, setWorkAnt},
        {"R?", 0, readTagOnce},
        {"RS=", 4, readTagSession},
        {"R!", 0, readTagOnceOnly},
        {"RR=", 10, readContinues},
        {"RB!", 0, readTagBufferInventory},
        {"SF=", 8, setTagFilter},
        {"STC!", 0, setTagCount},
        {"CTC!", 0, clearTagCount},
        {"REPC!", 0, readRawEPC},
        {"REPCC=", 10, readRawEPCCont},
        {"W=", 30, writeSGTINTag},
        {"RK!", 0, readKraalTag},
        {"RK?", 0, readKraalTag},
        {0, 0, NULL},
};

//=========================================================================
//  Description:    Process command
//  Parameters:     none
//  Returns:        non-zero for an error
//=========================================================================
int processCmd(char *s)
{
    int ret = 0;
    const COMMANDS *cmd = cmdTable;
    uint8_t found = 0;
    uint8_t i;
    const char *p;
    char *q;
    char c = 0;

    // cPrintf("Process CMD: %s",s);
    if (strlen(s))
    {
        ++stats.commands;

        // look for command
        for (; !found && cmd->command; ++cmd)
        {

            // find a matching entry in table
            p = cmd->command;
            q = s;
            for (; *p; ++p, ++q)
            {
                // dbgprintf("[%c %c]",*p, *q);
                if (*p != toupper(*q))
                {
                    break;
                }
            }
            if (*p)
            {
                continue;
            }

            found = 1;
            // nothing following
            if (cmd->flag & P_NONE)
            {
                if (*q)
                {
                    found = 0;
                }
            }
            // nothing or ?
            else if (cmd->flag & P_NONE_Q)
            {
                if (((*q == 0) || (*q == '?')))
                { //&& (*(q+1) == 0)) {
                    found = 1;
                }
                else
                {
                    found = 0;
                }
            }
            // process P_N_Q and P_N_E
            else if ((cmd->flag) & (P_N_Q | P_N_E))
            {
                for (i = 0;; ++i)
                {
                    if (q[i] == '?')
                    {
                        if ((cmd->flag & P_N_Q) == 0)
                        {
                            found = 0;
                        }
                        break;
                    }
                    else if (q[i] == '=')
                    {
                        if ((cmd->flag & P_N_E) == 0)
                        {
                            found = 0;
                        }
                        break;
                    }
                    else
                    {
                        c = q[i];
                        if ((q[i] == 0) || (i > 3) || (!isdigit(c)))
                        {
                            ret = ERR_BAD_PARAMETERS;
                            break;
                        }
                    }
                }
            }
            // check numerics
            else if (cmd->flag & P_NUM)
            {
                for (i = 0; q[i]; ++i)
                {
                    c = q[i];
                    if (!isdigit(c))
                    {
                        ret = ERR_BAD_PARAMETERS;
                        break;
                    }
                }
            }
            // check hex
            else if (cmd->flag & P_HX)
            {
                for (i = 0; q[i]; ++i)
                {
                    c = q[i];
                    if (!isxdigit(c))
                    {
                        ret = ERR_BAD_PARAMETERS;
                        break;
                    }
                }
            }
            // check upper
            else if (cmd->flag & P_UP)
            {
                for (i = 0; q[i]; ++i)
                {
                    q[i] = toupper(q[i]);
                }
            }
            if (!found)
            {
                continue;
            }

            //& with 0x3f max parameters will be 50
            if ((cmd->flag & 0x3f) && ((int)strlen(q)) > (cmd->flag & 0x3f))
            {
                ret = ERR_BAD_PARAMETERS;
            }
            break;
        }
        if ((ret == 0) && (!found))
        {
            ret = ERR_BAD_COMMAND;
        }
        if ((ret == 0) && (cmd->func))
        {
            ret = cmd->func(q); // execute the function in the table
            //	dprintf("%s CX ",cmd->command);
        }
    }

    // process erros
    if (ret)
    {
        ++stats.errors;
    }

    switch (ret)
    {
    case ERR_BAD_COMMAND:
        q = "BAD CMD";
        break;
    case ERR_BAD_PARAMETERS:
        q = "BAD PAR";
        break;
    case ERR_NOT_SUPPORTED:
        q = "NOT SUPPORTED";
        break;
    case NO_ANSWER:
        q = "NO ANSWER";
        break;
    default:
        q = NULL;
        cPrintf("OK>\r\n");
        break;
    }
    if (q != NULL)
        cPrintf("ERROR %u %s\r\n", ret, q);

    return ret;
}