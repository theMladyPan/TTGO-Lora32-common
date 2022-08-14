#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>  
#include "SSD1306.h"

#include <iostream>
#include <stdio.h>
#include "driver/uart.h"
#include "driver/gpio.h"

#define BAND    868E6


unsigned int counter = 0;
static uint32_t tStart = 0;

SSD1306 display(0x3c, 4, 15);

static String rcvdMsg("");
static bool rcvd = 0;


void message(String text, uint x=0, uint y=0, bool clear=false)
{
    if(clear)
    {
        display.clear();
    }
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_16);

    display.drawString(x, y, text);
    display.display();
}

void LoRa_rxMode(){
    LoRa.disableInvertIQ();               // normal mode
    LoRa.receive();                       // set receive mode
}

void LoRa_txMode(){
    LoRa.idle();                          // set standby mode
    LoRa.enableInvertIQ();                // active invert I and Q signals
}

void LoRa_sendMessage(String message) {
    LoRa_txMode();                        // set tx mode
    LoRa.beginPacket();                   // start packet
    LoRa.print(message);                  // add payload
    LoRa.endPacket(true);                 // finish packet and send it
}

void onReceive(int packetSize) {
    digitalWrite(LED_BUILTIN, HIGH);
    rcvdMsg = "";
    while (LoRa.available()) {
        rcvdMsg += (char)LoRa.read();
    }
    rcvd = 1;
    digitalWrite(LED_BUILTIN, LOW);
}

void onTxDone() {
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    LoRa_rxMode();   
    //LoRa.idle(); 
}

void setup() {
    setCpuFrequencyMhz(160); // set CPU core to 240MHz
    
    pinMode(OLED_RST,OUTPUT);
    pinMode(LED_BUILTIN,OUTPUT);
    
    digitalWrite(OLED_RST, LOW);    // set GPIO16 low to reset OLED
    delay(50); 
    digitalWrite(OLED_RST, HIGH); // while OLED is running, must set GPIO16 in high
    
    Serial.begin(115200);
    Serial.setTimeout(300);
    
    display.init();
    display.flipScreenVertically();  
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, "Device started");

    // begin(unsigned long baud, uint32_t config=SERIAL_8N1, int8_t rxPin=-1, int8_t txPin=-1, bool invert=false, unsigned long timeout_ms = 20000UL);
    //Serial1.begin(115200, SERIAL_8N1, 33, 32, false, 20);
    while (!Serial);
    Serial.println("LoRa Sender Test");
    
    SPI.begin(LORA_SCK,LORA_MISO,LORA_MOSI,LORA_CS);
    LoRa.setPins(LORA_CS,LORA_RST,LORA_IRQ);

    if (!LoRa.begin(BAND))
    {
        Serial.println("Starting LoRa failed!");
        while (1);
    }

    LoRa.setSpreadingFactor(7); // spreading factor SF7
    LoRa.setSignalBandwidth(125e3); // frequency bandwidth 125kHz
    LoRa.setCodingRate4(5); // coding factor 4:5

    LoRa.setTxPower(20);
    LoRa.enableCrc();
    
    LoRa.onReceive(onReceive);
    LoRa.onTxDone(onTxDone);
    LoRa_rxMode();

    gpio_wakeup_enable(static_cast<gpio_num_t>(LORA_IRQ), GPIO_INTR_HIGH_LEVEL); 
    esp_sleep_enable_timer_wakeup(2e6); // sleep for 1s max
    esp_sleep_enable_gpio_wakeup();
    
    Serial.println("init ok");
}


void loop() {

    display.displayOn();

    //char buf[64];
    //esp_fill_random(&buf, sizeof(buf));
    auto rcvd = String(counter++) + " - " + String(millis());
    if(Serial.available())
    {
        rcvd = String(Serial.readString());
    }

    message(String("Sending packet: "), 0, 0, true);
    message(String(rcvd), 0, 16, false); 

    // send packet
    tStart = millis();
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    LoRa.beginPacket();
    for(auto c:rcvd)
    {
        LoRa.write(c);
    }
    //LoRa.print(rcvd);
    LoRa.endPacket(true);
    
    Serial.print(rcvd+", len: "+String(rcvd.length()));

    esp_light_sleep_start();

    //after wake up from txDone evt:
    Serial.print(", sent in "+String(millis()-tStart)+"ms\n");
    message("Done!", 0, 32, false); 
    esp_light_sleep_start(); // sleep for another second

    
    //esp_deep_sleep(3e6);
}