/* 
 * File:   hx711.h
 * Author: srubi
 *
 * Created on March 6, 2022, 9:33 PM
 * 
 * How to use this lib. Define data and clock pins, e.g. in main.c: 
 * #define ADSK HX_SCK_PORT
 * #define ADDO HX_DT_PORT
 * 
 * or 
 * 
 * #define ADSK PORTCbits.RC3
 * #define ADDO PORTCbits.RC4
 * 
 * then import library:
 * #include "../lib/xerxes/hx711.h"
 * 
 * then use functions.
 */

#ifndef HX711_H
#define	HX711_H

#ifdef	__cplusplus
extern "C" {
#endif
 

/**
 * initialize HX711 chip
 */    
void HX711_Init(void)
{
    for (char i=0; i<25; i++)
    {
        ADSK = 1;
        __delay_us(1);
        ADSK = 0;
        __delay_us(1);
    }    
}

/**
 * test if HX711 IC is busy (conversion not ready)
 * @return 
 */
bool HX711_Busy(void)
{
    return ADDO;
}

/**
 * Read channel A from HX711 IC w/ gain=128 (25 clock pulses). Speed limited to 50kHz
 * @return 
 */
uint32_t HX711_ReadSlow(void)
{
    uint32_t Count;
    uint8_t i;
    ADDO = 1;
    ADSK = 0;
    Count = 0;
    while(ADDO);
    for (i=0; i<24; i++)
    {
        ADSK = 1;
        __delay_us(10);
        Count = Count<<1;
        ADSK = 0;
        __delay_us(10);
        if(ADDO) Count++;
    }
    ADSK = 1;
    __delay_us(10);
    Count = Count^0x800000;
    ADSK = 0;
    return(Count);
} 

/**
 * Read channel A from HX711 IC w/ gain=128 (25 clock pulses)
 * @return 
 */
uint32_t HX711_Read(void)
{
    uint32_t Count;
    uint8_t i;
    ADDO = 1;
    ADSK = 0;
    Count = 0;
    while(ADDO);
    for (i=0; i<24; i++)
    {
        ADSK = 1;
        Count = Count<<1;
        ADSK = 0;
        if(ADDO) Count++;
    }
    ADSK = 1;
    Count = Count^0x800000;
    ADSK = 0;
    
    
    return Count;
} 


#ifdef	__cplusplus
}
#endif

#endif	/* HX711_H */

