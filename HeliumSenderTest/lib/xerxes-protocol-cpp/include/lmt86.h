/* 
 * File:   lmt86.h
 * Author: srubi
 *
 * Created on March 6, 2022, 11:43 PM
 */

#ifndef LMT86_H
#define	LMT86_H

#include <math.h>

#ifdef	__cplusplus
extern "C" {
#endif


double LMT86_Read(adcc_channel_t channel)
{
    adc_result_t cur_val = ADCC_GetSingleConversion(channel); // read channel
    double mV = ((double)cur_val * 2048 ) / 4096; // convert reading to millivolts, FVR = 2.048mV
    double temp = ((10.888 - sqrt(118.548544 + (0.01388 * (1777.3 - mV) ))) / -0.00694) + 30; // calculate temperature for LMT86 sensor in celsius
    return temp; // fairly obvious
}

#ifdef	__cplusplus
}
#endif

#endif	/* LMT86_H */

