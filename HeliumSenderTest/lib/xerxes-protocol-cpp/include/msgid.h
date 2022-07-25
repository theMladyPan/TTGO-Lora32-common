/* 
 * File:   msgid.h
 * Author: srubi
 *
 * Created on March 7, 2022, 12:08 AM
 */

#ifndef MSGID_H
#define	MSGID_H

#include <stdint.h>

#ifdef	__cplusplus
extern "C" {
#endif


// msgids definitions
    
/** Ping packet ID */
#define MSGID_PING                          0x0000
    
/** Reply to ping packet */    
#define MSGID_PING_REPLY                    0x0001
    
/** Acknowledge OK packet */    
#define MSGID_ACK_OK                        0x0002
    
/** Acknowledge NOK packet */
#define MSGID_ACK_NOK                       0x0003
    
    
/**
 * Request to send measurements
 */
#define MSGID_FETCH_MEASUREMENT             0x0100
    
/** Synchronisaton message */
#define MSGID_SYNC                          0x0101
    
    
/** Set register to a value 
 * The message prototype is <MSGID_SET> <REG_ID> <LEN> <BYTE_1> ... <BYTE_N>
 */    
#define MSGID_SET                           0x0200
    
/** Read  up to <LEN> bytes from device register, starting at <REG_ID>
 * The request prototype is <MSGID_READ> <REG_ID> <LEN>
 */            
#define MSGID_READ                          0x0201
#define MSGID_READ_VALUE                    0x0202
    
    
/** Pressure value + 2 temperatures */    
#define MSGID_PRESSURE_mPa_111TEMP          0x0403    
/** Pressure value + second temperature only*/    
#define MSGID_PRESSURE_mPa_110TEMP          0x0402        
/** Pressure value + first temperature only*/    
#define MSGID_PRESSURE_mPa_101TEMP          0x0401
/** Pressure value w/o temperature*/    
#define MSGID_PRESSURE_mPa_100TEMP          0x0400
    
    
/** Strain value + 2 temperatures */    
#define MSGID_STRAIN_24BIT_11TEMP           0x1103    
/** Strain value + second temperature only */    
#define MSGID_STRAIN_24BIT_10TEMP           0x1102        
/** Strain value + first temperature only */    
#define MSGID_STRAIN_24BIT_01TEMP           0x1101
/** Strain value w/o temperature */    
#define MSGID_STRAIN_24BIT_00TEMP           0x1100
    

/** Cutter 1000P/R, 63mm wheel */
#define MSGID_PULSES                        0x2A01

    
/** 2 distance values, 0-22000um, no temp */
#define MSGID_DISTANCE_22MM                 0x4000
/** 2 distance values, 0-225000um, no temp */
#define MSGID_DISTANCE_225MM                0x4100
    

/** 2 angle values, X, Y (-90°, 90°)*/
#define MSGID_ANGLE_DEG_XY                  0x3000
    
    
//msgid structure:    
struct msg_id_8_s
{
    uint8_t msg_id_l;
    uint8_t msg_id_h;
};

typedef uint16_t msg_id_16_t;

union msg_id_u
{
    struct msg_id_8_s msgid_8 ;
    msg_id_16_t msgid_16;
};


#ifdef	__cplusplus
}
#endif

#endif	/* MSGID_H */