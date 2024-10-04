/* 
 * File:   errors.h
 * Author: Ben
 *
 * Created on March 15, 2020, 3:38 PM
 */

#ifndef ERRORS_H
#define	ERRORS_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#define errno errorno
//=======================================================================
// 	New list of errors
//      0 - No error
//		1 - Timeout
//		2 to 99	- Fatal system errors
//		100 to 255 (excluding 205) - Non fatal system errors
//		205	- user abort
//		256 to 300	- reserved
//		300 to 9999 - FSE errors
//=======================================================================

//#define ERR_TIMEOUT					1	// any type of timeout
//#define BUSY						2	// BUSY response
//#define ERR_UNKNOWN_COMMAND			13

#define ERR_DATA_WRITE           	20  // Error writing data
#define ERR_FAT_CORRUPT          	21	// FAT is corrupt
#define ERR_FILE_ALLOCATION      	22	// File has incorrect allocation
#define ERR_EEPROM_READ          	23
#define ERR_EEPROM_WRITE         	24
#define ERR_ID_COMPARE           	25	// ID mismatch for data
#define ERR_NO_MORE_MEMORY       	27	// Not enough memory for allocation
#define ERR_DATA_VERSION         	31  // Wrong format of data
#define ERR_DATA_CARTRIDGE       	32  // Data cartridge error
#define ERR_NEW_DATA_CARTRIDGE   	28	// Data module has been swopped
#define ERR_BAD_FILE_HANDLE      	29	// File handle out of range
#define ERR_BAD_RECORD_NO        	30	// Incorrect record number
#define ERR_BAD_SIZE            	36	// Incorrect size of file
#define ERR_CART_NOT_ENABLED    	37  // Write protection error
#define ERR_EMS_BAD_OFFSET      	38	// Offset out of range
#define ERR_EMS_PUT_ERROR       	39	// Pointer check error
#define ERR_EEPROM_RANGE_ERROR  	40
#define ERR_I2C                     87
#define ERR_TICKER_ALLOCATION   	89
#define	ERR_FILING_SYSTEM_ALLOCATION 90	// Filing system already used
#define	ERR_WRONG_FILING_SYSTEM   	91	// Filing system number too high
#define ERR_FLASH_ERASE         	95
#define ERR_FLASH_PROGRAM       	96
           
#define ERR_SELFTEST                97

#define ERR_DATA_READ           	120 // Error reading data
#define ERR_DATA_READ_CRC       	121 // CRC error reading record
#define ERR_CLOCK           		122 // Clock read or wtite error

#define ERR_WRONG_SIZE          	129 // wrong size for file
#define S_DEALLOCATE				130

/*
#define EXPIRED_NO_USE          	200
#define LINE_BUSY               	203
#define NO_CARRIER              	204  */

#define USER_ABORT              	205
#define NO_ANSWER               	207
//#define GSM_NOT_CONNECTED           207
/*
#define NO_ANSWER               	207
#define MODEM_FAIL              	209
#define NO_CONNECTION           	224
#define ENGAGED_TONE            	228
#define NO_RING_TONE            	229

// Session layer errors
#define XXX_NO_CONNECT          	236
#define NO_ID                   	237
#define S_SESSION_WRONG_PROTOCOL 	239
#define S_NO_ALLOCATE            	240
#define S_NO_ENQ				 	240	// used by ACISTD
#define S_TRANSMIT_TIMEOUT       	242
#define S_RECEIVE_TIMEOUT        	241
#define S_LENGTH_ERROR           	243
#define S_RETRIES                	244
#define S_NO_CONNECT             	245
#define S_TERMINATED			 	245	// ACISTD - premature EOT
#define S_BAD_MESSAGE            	246
#define ERROR_NO_PORT            	249
*/

//	System errors

#define ERR_CANNOT_OPEN_FILE		403
#define ERR_CANNOT_READ_FILE		404
#define ERR_CANNOT_WRITE_FILE		405
#define ERR_CANNOT_CLOSE_FILE		406
#define ERR_CANNOT_CREATE_FILE		407
#define ERR_NO_HEAP					408
#define ERR_SEEK					409
#define ERR_CANNOT_DELETE_FILE		410
#define ERR_CANNOT_RENAME_FILE		411


#define ERR_BAD_COMMAND             500
#define ERR_BAD_PARAMETERS          501
#define ERR_NO_PARAMETERS           502
#define ERR_RANGE                   503
#define ERR_COUNTRY_CODE            504
#define ERR_ANTENNA_NOT_ENABLED     505
#define ERR_IO_EXPANDERS            506
#define ERR_NOT_SUPPORTED           507  

// Database errors

#define ERR_BIN_NO_FILE 			603
#define ERR_BIN_NO_MATCH    		604
#define ERR_BIN_UPDATE				605
#define ERR_PARMS_NO_SPACE       	610

#define ERR_SAF_LIMIT_REACHED 		623
#define ERR_HC_COMMAND				640
#define ERR_HC_FILEFULL				641
#define ERR_HC_TOO_MANY_FILES   	642
#define ERR_HC_CREATE_ERROR     	643
#define ERR_HC_ALREADY_LOADED		645
#define ERR_HC_NOT_ON_FILE      	646
#define ERR_HC_EMPTY_DELETED    	647


// Tag errors
#define ERR_TAG_MODE                700     // correct mode not set up
#define ERR_RECEIVE_TIMEOUT         701     // response not received in time
#define ERR_RECEIVE_OVERFLOW        702     // too much data received
#define ERR_RESPONSE_TOO_SHORT      703
#define ERR_READ_VERIFY             704     // read after write fail
#define ERR_TAG_ERROR               705     // tag indicates an error
#define ERR_TAG_ERASE               706     // erase error
#define ERR_TAG_WRITE               707     // tag write error
#define ERR_TOO_MANY_TAGS           709     // maore than 1 tag

// EAN number errors
#define ERR_EAN_LENGTH              710     // incorrect length for EAN number
#define ERR_EAN_CHECK_DIGIT         711     // incorrect EAN check digit

// EPC numbers
#define ERR_EPC_PARTITION_LENGTH    720     // length of prefix out of range
#define ERR_EPC_FILTER_LENGTH       721     // length of filter out of range
#define ERR_EPC_CLASS               722     // class must be 0xc0

// EPC 1.19 errors
#define ERR_EPC119_HEADER           730     // header is not EF04

// UCODE errors
#define ERR_UCODE_PREAMBLE          740     // preamble not found
#define ERR_UCODE_DELIM             741     // 741 tio 747
#define ERR_UCODE_SEQUENCE          748     // invalid bit sequence
#define ERR_UCODE_CRC               749

// EPC errors
#define ERR_EPC_PREAMBLE            750     // preamble not found
#define ERR_EPC_DELIM               751     // 751 tio 757
#define ERR_EPC_SEQUENCE            758     // invalid bit sequence
#define ERR_EPC_CRC                 759

// general tag errors
#define ERR_LBT_BUSY                800     // LBT cannot find a gap

// Gen2 error response
#define ERR_GEN2_BASE               1000     // 1000 to 1255 





#ifdef	__cplusplus
}
#endif

#endif	/* ERRORS_H */

