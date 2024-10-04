
// #include "filterTags.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "include/rBell.h"
// #include "c:\\Espressif\\frameworks\\esp-idf-v5.1.1\\components\\esp_hw_support\\include\\rtc_wdt.h"
#include "rtc_wdt.h"
//-----------------------------
extern volatile uint32_t globalFilterVariable;
extern uint32_t filterReloadValue;
extern void stopTimer();
extern void startTimer();
void exitFilter(void);
//-----------------------------------------------------------
struct
{
    bool showRawEPC;
    bool countTags;
    int tagKeepCount;
    bool readKraalTag;
    bool readEPCTag;
} tagReadFlags;
//=============================================================================
typedef struct tagstruct
{
    uint8_t number[12]; // Cater for EPC 96bit (496 bit not supported for now) 12bytes asdd one
    uint8_t length;
    uint8_t flag;
    uint8_t reported;
    uint8_t antenna;
    int count;
    uint8_t ticks;       // tag read
    uint8_t reportTicks; // tag reported
    struct tagstruct *next;
} TAG;
//=============================================================================
#define NOT_REPORTED 0
#define REPORTED 1
#define NO false
#define YES true
//=============================================================================
static TAG *startTag = 0;
static TAG *endTag = 0;

static uint32_t minTicks = 0;
static uint32_t filterTimeOut = 0; // if zero will not age

//=-----------------------------------------------
static TAG *findTag(uint8_t *number, uint8_t length, uint8_t antenna);
static TAG *addTag(uint8_t *number, uint8_t length, uint8_t antenna);
static uint8_t deleteTag(TAG *tag);
extern int cPrintf(const char *fmt, ...);
//==========================================================================
//  Description:    Initialise
//  Parameters:     seconds - if zero don't age
//                  minVal - ticks before reporting tag
//  Returns:        Non-zero for error
//==========================================================================
uint8_t initFilter(uint8_t mSec, uint16_t minVal)
{
    exitFilter(); // clean up

    filterReloadValue = mSec; // multiples of 100 ie 1000ms = 100mS
    filterTimeOut = mSec;     // for 1mS intervals
    minTicks = minVal;
    tagReadFlags.tagKeepCount = 0;
    globalFilterVariable = 0;
    return 0;
}
//=========================================================================
//	Description:	Routine to get number of milliseconds since system startup
//  Parameters:		none
//  Returns:		milliseconds updated at each system tick
//=========================================================================
unsigned long sys_ticks(void)
{
    // return usTicker;
    return globalFilterVariable;
}
//=========================================================================
//	Description:	Routine to get number of milliseconds since system startup
//  Parameters:		none
//  Returns:		milliseconds updated at each system tick
//=========================================================================
bool checkFilterTo(void)
{
    int ret = false;
    if (filterReloadValue) // only do this check if a reload value was set
    {

        if (globalFilterVariable >= (filterReloadValue))
        {
            // cPrintf("TO happened: %d:%d\r\n", globalFilterVariable, filterReloadValue);
            globalFilterVariable = 0; // reset variable
            filterTimeOut = filterReloadValue;
            ret = true;
        }
    }
    return ret;
}
//==========================================================================
//  Description:    Age filter
//  Parameters:     none
//  Returns:        none
//==========================================================================
void ageFilter(uint8_t readType)
{
    TAG *t;
    TAG *u;
    uint32_t time; //, tsyst;

    if (filterTimeOut)
    {
        for (t = startTag; t;)
        {

            // tsyst = sys_ticks();
            if (sys_ticks() > t->ticks)
                time = sys_ticks() - t->ticks;
            else
                time = 0;
            //        dprintf("<t->c%d t%ld st%ld t->t%ld to%ld>\r\n",t->count, time,sys_ticks(),t->ticks,filterTimeOut);
            // dprintf("[time >= timeout [%d:%d] systicks:%d t->ticks:%d minticks:%d t-flag:%d]\r\n",time, timeout, sys_ticks(),t->ticks, minTicks, t->flag);
            //&&  (t->flag == 0)

            if (((time >= filterTimeOut) || minTicks) && (time >= minTicks))
            {
                u = t;
                t = t->next;
                /*   if(tagParms.tagGenFlags1 & TAG_ARR_DEP){
                       switch(readType){
                          case READ_RR:
                               callBackFiltAdRR(u->number, u->length, 0);
                               break;
                           case READ_RA:
                           case READ_RRA:
                           case READ_R1:   //READ_R2, RA1, RA2 also included here
                           case READ_RA1:
                           case READ_RA2:
                           case READ_RRA1:
                           case READ_RRA2:
                           case READ_ETU:
                               callBackFiltAd(u->number, u->length, 0, u->antenna, 0 );
                               break;
                           default:
                               break;
                       }


                   }*/
                // dprintf("[DT]");
                // if( t->reported != REPORTED)				//only erase if not reported
                deleteTag(u);
            }
            else
            {
                // Sleep(1);
                // dprintf("[NT]");
                t = t->next;
            }
        }
    }
}
//==========================================================================
//  Description:    Filter tags
//  Parameters:     tag number
//  Returns:        false to report, true to suppress
//==========================================================================
bool filter(uint8_t *number, uint8_t length, uint8_t antenna)
{
    TAG *t;
    bool ret = false;

    // uint8_t tempNum[20];
    //  always report tags if no filter enabled
    // if (!(filterTimeOut)) {
    //     return 0;
    // }
    // memcpy(tempNum,number,length);
    if ((t = (findTag((number), (length), antenna))))
    {
        ageFilter(10);
        // cPrintf("Tag found.. ");
        // cPrintf("%d:%d\r\n", globalFilterVariable, filterReloadValue);
        if (t->reported == NOT_REPORTED) // tag was found and not reported
        {
            if (checkFilterTo() == false) // check timeout & reported state
            {
                // cPrintf("No TO\r\n");
                ret = NO; // no
            }
            else
            {
                ret = YES; // no
            }
        }
        else
        {
            t->reported = NOT_REPORTED; // report it now
            ret = NO;                   // no
        }
    }
    else
    {
        // tag was not found in list, it was added report it now
        // cPrintf("\r\nAdd tag.. ");
        t = addTag(number, length, antenna);
        t->reportTicks = sys_ticks(); // set start value
        t->ticks = sys_ticks();       // tag has been read
        t->reported = REPORTED;       // newly added tag report by default
        ret = YES;                    // report tag
        globalFilterVariable = 0;     // reset filter

        /* if (minTicks)
         {
             // dprintf("[RET1...]");
             ret = true; // suppress for now
         }
         else
         {
             cPrintf("\r\nReport tag.. ");
             t->flag = 1;
             if (t->reported == REPORTED) // already reported, YES
             {                            // set as reported)
                 // if(!(tagParms.tagGenFlags1 & FILTER_SUPRESS))
                 //	ret = 1;							//suppress
                 ret = true; // yes
             }
             else // no
             {
                 t->reported = REPORTED; // set as reported
                 ret = false;            // no report
             }
         }*/
    }

    /*   if(tagParms.tagCount){
          dprintf("-%03d-",t->count);
      } */
    // t->ticks = sys_ticks(); // tag has been read
    //  dprintf("|%ld|\r\n",t->ticks);
    // ageFilter(0);

    return ret;
}
//==========================================================================
//  Description:    Find a tag
//  Parameters:     tag number, length
//  Returns:        Pointer to tag, or zero if not found
//==========================================================================
static TAG *findTag(uint8_t *number, uint8_t length, uint8_t antenna)
{
    TAG *t;
    uint8_t x;
    for (t = startTag; t; t = t->next)
    {
        // dbgprintf("[%d]",t->antenna);
        for (x = 0; x < length; x++)
            // dbgprintf("[%x]/%x/",t->number[x], number[x]);
            // dbgprintf("\r\n");
            if (memcmp(t->number, number, length) == 0)
            { // tag was found in list
                // cPrintf("\r\nTag in list\r\n");
                return t;
            }
    } // look for next tag
    return 0; // return only a zero if all tags were checked and none found
}

//==========================================================================
//  Description:    Add tag
//  Parameters:     tag number, length
//  Returns:        Pointer to tag
//==========================================================================
static TAG *addTag(uint8_t *number, uint8_t length, uint8_t antenna)
{
    TAG *addedTag = malloc(sizeof(TAG));
    tagReadFlags.tagKeepCount++;
    if (length > sizeof(addedTag->number))
    {
        length = sizeof(addedTag->number);
    }
    memset(addedTag, 0, sizeof(TAG));
    memmove(addedTag->number, number, length);
    // for(int x=0;x<length;x++)
    //     cPrintf("%02x:",addedTag->number[x]);
    addedTag->length = length;
    addedTag->count = tagReadFlags.tagKeepCount;
    addedTag->antenna = antenna;
    if (startTag == 0)
    {
        startTag = endTag = addedTag;
    }
    else
    {
        endTag->next = addedTag; // point to next tag address
        endTag = addedTag;       // fill endtag with new tag malloced
    }
    return addedTag;
}
//==========================================================================
//  Description:    Delete tag
//  Parameters:     pointer to tag
//  Returns:        Non-zero for error
//==========================================================================
static uint8_t deleteTag(TAG *tag)
{
    TAG *t, *u;

    if (tag == 0)
    {
        for (t = startTag; t;)
        {
            u = t;
            t = t->next;
            free(u);
        }
        startTag = endTag = 0;
        return 0;
    }

    if (startTag && endTag)
    {
        if (tag == startTag)
        {
            if (tag->next == 0)
            {
                startTag = endTag = 0;
            }
            else
            {
                startTag = tag->next;
            }
            free(tag->number);
            return 0;
        }
        else
        {
            for (t = startTag; t; t = t->next)
            {
                if (t->next == tag)
                {
                    t->next = tag->next;
                    if (tag == endTag)
                    {
                        endTag = t;
                    }
                    free(tag->number);
                    return 0;
                }
            }
        }
    }
    free(tag->number);
    return 1;
}
//==========================================================================
//  Description:    Clean up
//  Parameters:     None
//  Returns:        None
//==========================================================================
void exitFilter(void)
{
    TAG *t, *u;

    for (t = startTag; t;)
    {
        u = t;
        t = t->next;
        deleteTag(u);
        // memset(u,0,sizeof(TAG));
        // free(u);
    }
    startTag = endTag = 0;
    globalFilterVariable = 0;
    tagReadFlags.tagKeepCount = 0;
}
//=================================================================================
//=================================================================================
