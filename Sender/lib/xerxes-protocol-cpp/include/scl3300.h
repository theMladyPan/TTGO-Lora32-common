/* 
 * File:   scl3300.h
 * Author: srubi
 *
 * Created on 08 May 2022, 20:13
 */

#ifndef SCL3300_H
#define	SCL3300_H

#define Read_ACC_X 0x040000F7
#define Read_ACC_Y 0x080000FD
#define Read_ACC_Z 0x0C0000FB
#define Read_STO 0x100000E9
#define Enable_ANGLE 0xB0001F6F
#define Read_ANG_X 0x240000C7
#define Read_ANG_Y 0x280000CD
#define Read_ANG_Z 0x2C0000CB
#define Read_Temperature 0x140000EF
#define Read_Status_Summary 0x180000E5
#define Read_ERR_FLAG1 0x_1C0000E
#define Read_ERR_FLAG2 0x200000C1
#define Read_CMD 0x340000DF
#define Change_to_mode_1 0xB400001F
#define Change_to_mode_2 0xB4000102
#define Change_to_mode_3 0xB4000225
#define Change_to_mode_4 0xB4000338
#define Set_power_down_mode 0xB400046B
#define Wake_up_from_power_down_mode 0xB400001F
#define SW_Reset 0xB4002098
#define Read_WHOAMI 0x40000091
#define Read_SERIAL1 0x640000A7
#define Read_SERIAL2 0x680000AD
#define Read_current_bank 0x7C0000B3
#define Switch_to_bank_0 0xFC000073
#define Switch_to_bank_1 0xFC00016E

#ifdef	__cplusplus
extern "C" {
#endif

    
/**
 * based on python code
 * 
    self.raw = intrep
    self.RW = (intrep & 2**31) >> 31
    self.ADDR = (intrep & (0b01111100 << 16)) >> 18
    self.RS = (intrep & (0b11 << 16)) >> 16
    self.DATA = (intrep & (0xFFFF << 8)) >> 8
    self.CRC = intrep & (0xFF)
    self.ERR = self.RS == 0b11
 */    
typedef struct {
        unsigned RW         :1;
        unsigned ADDR       :5;
        unsigned RS         :2;
        unsigned DATA_H     :8;
        unsigned DATA_L     :8;
        unsigned CRC        :8;
} SclPacket_t;

    
uint32_t ExchangeBlock(uint32_t block){
    uint32_t rcvd;
    SPI_CS_Enable();
    rcvd  = (uint32_t)SPI_Exchange( (block & 0xff000000) >> 24 ) << 24;
    rcvd += (uint32_t)SPI_Exchange( (block & 0x00ff0000) >> 16 ) << 16;
    rcvd += (uint32_t)SPI_Exchange( (block & 0x0000ff00) >> 8  ) << 8;
    rcvd += (uint32_t)SPI_Exchange( (block & 0x000000ff)       );
    SPI_CS_Disable();
    __delay_us(10);
    
    return rcvd;
}


SclPacket_t ConvToPacket(uint32_t rcvd)
{
    SclPacket_t to_return;
    to_return.RW        = rcvd >> 31;
    to_return.ADDR      = rcvd >> 26;
    to_return.RS        = rcvd >> 24;
    to_return.DATA_H    = rcvd >> 16;
    to_return.DATA_L    = rcvd >> 8;
    to_return.CRC       = rcvd;
    
    return to_return;
}
    

double getDeg(SclPacket_t *packet)
{
    uint16_t raw = (uint16_t)(packet->DATA_H << 8) + packet->DATA_L;
    raw = raw ^ 0x8000;
    double degrees = (double)raw * 180 / (1 << 15);
    return 180 + degrees;
}

void Scl3300Init(){
    SPI_OpenDefault();
    
    uint32_t received_packet;
    
    __delay_ms(1);
    ExchangeBlock(Switch_to_bank_0);
    __delay_ms(1);

    received_packet = ExchangeBlock(SW_Reset);
    __delay_ms(1);

    received_packet = ExchangeBlock(Change_to_mode_4);
    __delay_ms(1);
    received_packet = ExchangeBlock(Enable_ANGLE);
    __delay_ms(100);

    received_packet = ExchangeBlock(Read_Status_Summary);
    received_packet = ExchangeBlock(Read_Status_Summary);
    received_packet = ExchangeBlock(Read_Status_Summary);
    __delay_ms(1);
    received_packet = ExchangeBlock(Read_WHOAMI);
    received_packet = ExchangeBlock(Read_WHOAMI);
    
}


void SclReadXY(SclPacket_t *packetX, SclPacket_t *packetY)
{
    SPI_CS_Enable();
    
    ExchangeBlock(Read_ANG_X);
    *packetX = ConvToPacket(ExchangeBlock(Read_ANG_Y));
    *packetY = ConvToPacket(ExchangeBlock(0x00000000));
    
    SPI_CS_Disable();    
}


#ifdef	__cplusplus
}
#endif

#endif	/* SCL3300_H */

