#include "serialize.h"


void serialize_uint32_little(uint32_t *values, uint8_t *buffer, uint32_t len)
{
    union ul2c cu;
    for(int i=0; i<len; i++)
    {
        cu.ul = values[i];
        for(int j=0; j<4;j++)
        {
            buffer[(i*4)+j] = cu.c[j];
        }
    }
}

void serialize_uint32_big(uint32_t *values, uint8_t *buffer, uint32_t len)
{
    union ul2c cu;
    for(int i=0; i<len; i++)
    {
        cu.ul = values[i];
        for(int j=0; j<4;j++)
        {
            buffer[(i*4)+j] = cu.c[3-j];
        }
    }
}

void serialize_float_little(float *values, uint8_t *buffer, uint32_t len)
{
    union f2c cf;
    for(int i=0; i<len; i++)
    {
        cf.f = values[i];
        for(int j=0; j<4;j++)
        {
            buffer[(i*4)+j] = cf.c[j];
        }
    }
}

void serialize_float_big(float *values, uint8_t *buffer, uint32_t len)
{
    union f2c cf;
    for(int i=0; i<len; i++)
    {
        cf.f = values[i];
        for(int j=0; j<4;j++)
        {
            buffer[(i*4)+j] = cf.c[3-j];
        }
    }
}
