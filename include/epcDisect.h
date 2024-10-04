/* 
 * File:   epcDisect.h
 * Author: Ben
 *
 * Created on 23 March 2020, 2:35 PM
 */
#ifndef EPCDISECT_H
#define	EPCDISECT_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
    
uint8_t disectEpcTagNumber(uint8_t *tagBuffer);
uint8_t disectEpcTagToBuf(uint8_t *tagBuffer, uint64_t *vizNum);

extern int cPrintf(const char *fmt, ...);

#ifdef	__cplusplus
}
#endif

#endif	/* EPCDISECT_H */