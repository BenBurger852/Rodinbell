/* 
 * File:   checkSum.h
 * Author: Ben
 *
 * Created on March 15, 2020, 11:12 AM
 */


#ifndef CHECKSUM_H
#define	CHECKSUM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <xc.h>
#include <sys/attribs.h>

#ifdef	__cplusplus
extern "C" {
#endif
unsigned char CheckSum(unsigned char *uBuff, unsigned char uBuffLen);

#ifdef	__cplusplus
}
#endif

#endif	/* CHECKSUM_H */

