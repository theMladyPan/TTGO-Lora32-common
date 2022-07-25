/* 
 * File:   devids.h
 * Author: srubi
 *
 * Created on March 7, 2022, 1:15 AM
 */

#ifndef DEVIDS_H
#define	DEVIDS_H

#ifdef	__cplusplus
extern "C" {
#endif

/** Pressure sensors */
/** pressure sensor range 0-600mbar, output in Pa, 2 external temperature sensors -50/150°C output: mK */
#define DEVID_PRESSURE_600MBAR_2TEMP    0x03
/** pressure sensor range 0-60mbar, output in Pa, 2 external temperature sensors -50/150°C output: mK */
#define DEVID_PRESSURE_60MBAR_2TEMP     0x04
    
/** Strain sensors */
/** strain-gauge sensor range 0-2^24, 2 external temperature sensors -50/150°C output: mK */
#define DEVID_STRAIN_24BIT_2TEMP        0x11
    
    
/** I/O Devices */
/** I/O device, 8DI/8DO (8xDigital Input, 8xDigital 0utput) */
#define DEVID_IO_8DI_8DO                0x20

/** Encoder reader */
#define DEVID_ENC_1000PPR               0x2A
    
    
/** Inclinometers and accelerometers */
/** Inclinometer SCL3300 */
#define DEVID_ANGLE_XY_90               0x30
    
    
/** Distance sensors */
/** Distance sensor 0-22mm, resistive, linear*/
#define DEVID_DIST_22MM                 0x40
/** Distance sensor 0-225mm, resistive, linear*/
#define DEVID_DIST_225MM                0x41


#ifdef	__cplusplus
}
#endif

#endif	/* DEVIDS_H */

