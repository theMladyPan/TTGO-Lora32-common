/* 
 * File:   serialize.h
 * Author: themladypan
 *
 * Created on March 7, 2022, 11:21 PM
 */

#include <stdint.h>

#ifndef SERIALIZE_H
#define	SERIALIZE_H

#ifdef	__cplusplus
extern "C" {
#endif


union ul2c
{
    uint32_t ul;
    uint8_t c[4];
};

union f2c
{
    float f;
    uint8_t c[4];
};

void serialize_uint32_little(uint32_t *values, uint8_t *buffer, uint32_t len);

void serialize_uint32_big(uint32_t *values, uint8_t *buffer, uint32_t len);

void serialize_float_little(float *values, uint8_t *buffer, uint32_t len);

void serialize_float_big(float *values, uint8_t *buffer, uint32_t len);



#ifdef	__cplusplus
}
#endif

#endif	/* SERIALIZE_H */

