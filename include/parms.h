/* 
 * File:   parms.h
 * Author: Ben
 *
 * Created on March 29, 2020, 4:10 PM
 */

#ifndef PARMS_H
#define	PARMS_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#include "../mcc_generated_files/mcc.h"
#define SGLN 0
#define SGTIN 1
    
typedef union
 { 
    struct
    {
        byte tagRxSTX:1;
        byte tagRxEND:1;
        byte lfTagStatus:1;
        byte RBReadRepeat:1;
        byte readSGLN:1;
        byte readSGTIN:1;
        byte waterReader:1;
        UINT readRepeatTimeout; 
    };
 } TAG_RX_STAT;

  
    //======================================
 typedef union
{ 
    struct
    {    
        byte startByte;
        byte switchOffState:3;      
        byte commandFrom:2;             //can be 0 1 or 2 for both
        byte debug:1;
        byte wifiConnectState:1;        
        byte btConnectState:1;
        byte selectComms:3;             //select wifi or BT 0 = wire 1 = WIFI; 2 = BT; 
        byte reportMode:1;              //raw EPC mode = 1; GS1 mode = 1
        byte beeperMode:1;
        byte storeTags:1;  
        byte autoOff:1;
        byte lineFeed:1;
        byte version:3;
     } ;
 } SYSTEM_STAT;

  //=======================================
typedef union
 { 
    struct
    {
        byte readerStatus:1;    
        byte readRepeat:1;        
    };
 } LF_READER_STAT;
  //=============================================
typedef union
{ 
 struct 
    {
        unsigned char extButtonStat:1;       
        unsigned char extButtonLongIn:1;        //button down for 5000 ms or more
        unsigned char extButtonCount:3;         //count how many times pressed
        unsigned char buttonDebounce:1;
        
        byte lfReadPressed:1;
        byte uhfReadPressed:1;
    } ;
} BUTTON_STAT; 
 //=============================================
typedef union
{ 
    struct
    {
        byte ledState:3;
        byte buzzState:1;
        byte secValSet:1;
        byte msecValSet:1;
        byte rbToValSet:1;
        byte lfToValSet:1;
        byte eeValSet:1;        
        byte beepStat:1; 
        byte bootUp:1;
        UINT ledFlashVal;        
        UINT beepLen;        
        UINT switchOffTimer;
    };
} TIMER_STAT; 

  //=======================================
typedef union
 { 
    struct
    {
        byte readerStatus:1;   
        byte readRepeat:1;
        byte waterReader:1;
        UINT readRepeatTimeout;
        byte sNumber;
    };
 } RB_READER_STAT;
//=============================================
 typedef union
 { 
    struct
    {
        byte ssid[20];
        byte password[20];
    };
 } WIFI_PARMS;
//=============================================
 typedef union
 { 
    struct
    {
        byte apname[20];
        byte password[20];
    };
 } BT_PARMS;
 
#define PRESSED 0
#define NOT_PRESSED 1
#ifdef	__cplusplus
}
#endif

#endif	/* PARMS_H */

