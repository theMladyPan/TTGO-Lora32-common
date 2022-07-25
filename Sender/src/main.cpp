#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>  
#include "SSD1306.h"
#include "protocol.h"
#include "msgid.h"
#include "devids.h"

#include <iostream>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#define ECHO_TEST_TXD  (GPIO_NUM_32)
#define ECHO_TEST_RXD  (GPIO_NUM_33)
#define ECHO_TEST_RTS  (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS  (UART_PIN_NO_CHANGE)

#define BUF_SIZE (1024)

#define SCK     5    // GPIO5  -- SX1278's SCK
#define MISO    19   // GPIO19 -- SX1278's MISO
#define MOSI    27   // GPIO27 -- SX1278's MOSI
#define SS      18   // GPIO18 -- SX1278's CS
#define RST     14   // GPIO14 -- SX1278's RESET
#define DI0     26   // GPIO26 -- SX1278's IRQ(Interrupt Request)
#define BAND    868E6

unsigned int counter = 0;

SSD1306 display(0x3c, 4, 15);

void ledOff()
{
    Serial.print("sent");
}


void setup() {
    pinMode(16,OUTPUT);
    pinMode(2,OUTPUT);
    
    digitalWrite(16, LOW);    // set GPIO16 low to reset OLED
    delay(50); 
    digitalWrite(16, HIGH); // while OLED is running, must set GPIO16 in high
    
    Serial.begin(115200);
    Serial.setTimeout(300);

    // begin(unsigned long baud, uint32_t config=SERIAL_8N1, int8_t rxPin=-1, int8_t txPin=-1, bool invert=false, unsigned long timeout_ms = 20000UL);
    //Serial1.begin(115200, SERIAL_8N1, 33, 32, false, 20);
    while (!Serial);
    Serial.println("LoRa Sender Test");
    
    SPI.begin(SCK,MISO,MOSI,SS);
    LoRa.setPins(SS,RST,DI0);
    if (!LoRa.begin(868E6)) {
        Serial.println("Starting LoRa failed!");
        while (1);
    }
    LoRa.onTxDone(ledOff);
    //LoRa.setTxPower();
    LoRa.enableCrc();
  //LoRa.onReceive(cbk);
//  LoRa.receive();
    Serial.println("init ok");
    display.init();
    display.flipScreenVertically();  
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, "Device started");

    setCpuFrequencyMhz(80); // set CPU core to 240MHz

    //bootloader_random_enable();
}


void loop() {
    delay(1000);
    char buf[10];
    esp_fill_random(&buf, sizeof(buf));
    auto rcvd = String(buf);
    rcvd = String(counter++);
    if(Serial.available())
    {
        rcvd = String(Serial.readString());
    }

    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);

    display.drawString(0, 0, "Sending packet: ");
    display.drawString(0, 20, String(rcvd));
    display.display();

    // send packet
    uint32_t tStart = millis();
    digitalWrite(2, HIGH);   // turn the LED on (HIGH is the voltage level)
    LoRa.beginPacket();
    LoRa.print(rcvd);
    LoRa.endPacket();
    digitalWrite(2, LOW);    // turn the LED off by making the voltage LOW
    
    Serial.print(rcvd);
    Serial.print(" - sent in ");
    Serial.print(String(millis()-tStart));
    Serial.print("ms\n");
    
    delay(2000);
    //esp_deep_sleep(3e6);
}