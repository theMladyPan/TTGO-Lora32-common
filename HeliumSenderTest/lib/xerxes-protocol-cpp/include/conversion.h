/* 
 * File:   conversion.h
 * Author: srubi
 *
 * Created on March 6, 2022, 11:44 PM
 */

#ifndef CONVERSION_H
#define	CONVERSION_H

#ifdef	__cplusplus
extern "C" {
#endif

uint32_t to_millikelvin(double celsius)
{
    double kelvin = celsius + 273.15;
    double millikelvin = kelvin * 1000;
    return (uint32_t)millikelvin;
}

#ifdef	__cplusplus
}
#endif

#endif	/* CONVERSION_H */

