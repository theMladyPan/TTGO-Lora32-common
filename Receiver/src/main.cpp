#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>  
#include "SSD1306.h" 
//#include "images.h"

#define SCK     5    // GPIO5  -- SX1278's SCK
#define MISO    19   // GPIO19 -- SX1278's MISO
#define MOSI    27   // GPIO27 -- SX1278's MOSI
#define SS      18   // GPIO18 -- SX1278's CS
#define RST     14   // GPIO14 -- SX1278's RESET
#define DI0     26   // GPIO26 -- SX1278's IRQ(Interrupt Request)
#define BAND    868E6

SSD1306 display(0x3c, 4, 15);
String rssi = "RSSI --";
String packSize = "--";

static uint rcvd = 0;


void onRxInt(int packSize)
{
    rcvd = packSize;
}

int receive(int packetSize) {
    String packet("");
    int rssi = LoRa.packetRssi();
    packSize = String(packetSize,DEC);
    for (int i = 0; i < packetSize; i++) 
    { 
        packet += (char) LoRa.read(); 
    }
    
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 0, "RSSI " + String(rssi, DEC) + "dBm");  
    display.drawString(0 , 16 , "Rcvd: "+ packSize + " bytes");
    display.drawStringMaxWidth(0 , 32 , 128, packet);
    display.display();
    Serial.println(packet);

    return rssi;
}

  void setup() {
    pinMode(OLED_RST,OUTPUT);
    digitalWrite(OLED_RST, LOW);    // set GPIO16 low to reset OLED
    delay(50); 
    digitalWrite(OLED_RST, HIGH); // while OLED is running, must set GPIO16 in high、
    
    Serial.begin(115200);
    while (!Serial);
    Serial.println();
    Serial.println("LoRa Receiver Callback");
    SPI.begin(SCK,MISO,MOSI,SS);
    LoRa.setPins(SS,RST,DI0);  
    if (!LoRa.begin(868E6)) {
      Serial.println("Starting LoRa failed!");
      while (1);
    }
    LoRa.onReceive(onRxInt);
    LoRa.disableInvertIQ(); 
    LoRa.receive();    
    LoRa.setSpreadingFactor(7); // spreading factor SF7
    LoRa.setSignalBandwidth(125e3); // frequency bandwidth 125kHz
    LoRa.setCodingRate4(5); // coding factor 4:5

    LoRa.setTxPower(20);
    LoRa.enableCrc();

    Serial.println("init ok");
    display.init();
    display.flipScreenVertically();  
    display.setFont(ArialMT_Plain_16);

    display.drawString(0 , 0 , "Listening");
    display.display();

    gpio_wakeup_enable(static_cast<gpio_num_t>(LORA_IRQ), GPIO_INTR_HIGH_LEVEL);
    esp_sleep_enable_gpio_wakeup();
}

void sleep_ms(uint ms)
{
    esp_sleep_enable_timer_wakeup(ms*1000);
    esp_light_sleep_start();
}

void loop() {
    //int packetSize = LoRa.parsePacket();
    //if (packetSize) { cbk(packetSize);  }
    if(rcvd)
    {
        receive(rcvd);
        rcvd = 0;
    }
    sleep_ms(10);
}