// rf95_mesh_server1.cpp
// -*- mode: C++ -*-
// Example sketch showing how to create a simple addressed, routed reliable messaging server
// with the RHMesh class.
// It is designed to work with the other examples rf95_mesh_*
// Hint: you can simulate other network topologies by setting the 
// RH_TEST_NETWORK define in RHRouter.h

// Mesh has much greater memory requirements, and you may need to limit the
// max message length to prevent wierd crashes
//
// Requires Pigpio GPIO library. Install by downloading and compiling from
// http://abyz.me.uk/rpi/pigpio/, or install via command line with 
// "sudo apt install pigpio". To use, run "make" at the command line in 
// the folder where this source code resides. Then execute application with
// sudo ./rf95_mesh_server1.
// Tested on Raspberry Pi Zero and Zero W with LoRaWan/TTN RPI Zero Shield 
// by ElectronicTricks. Although this application builds and executes on
// Raspberry Pi 3, there seems to be missed messages and hangs.
// Strategically adding delays does seem to help in some cases.

//(9/20/2019)   Contributed by Brody M. Based off rf22_mesh_server1.pde.
//              Raspberry Pi mods influenced by nrf24 example by Mike Poublon,
//              and Charles-Henri Hallard (https://github.com/hallard/RadioHead)

#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include <RHMesh.h>
#include <RH_RF95.h>

#include "SSD1306.h"

#define RH_MESH_MAX_MESSAGE_LEN 50

//Function Definitions
void sig_handler(int sig);

#define SCK     5    // GPIO5  -- SX1278's SCK
#define MISO    19   // GPIO19 -- SX1278's MISO
#define MOSI    27   // GPIO27 -- SX1278's MOSI
#define SS      18   // GPIO18 -- SX1278's CS
#define RST     14   // GPIO14 -- SX1278's RESET
#define DI0     26   // GPIO26 -- SX1278's IRQ(Interrupt Request)
#define BAND    868E6

//Pin Definitions
#define RFM95_CS_PIN SS
#define RFM95_IRQ_PIN DI0
#define RFM95_LED LED_BUILTIN
#define LED_PIN_NUM GPIO_NUM_2

// In this small artifical network of 4 nodes,
#define CLIENT_ADDRESS  1
#define SERVER1_ADDRESS 2
#define SERVER2_ADDRESS 3
#define SERVER3_ADDRESS 4

//RFM95 Configuration
#define RFM95_FREQUENCY  868.00
#define RFM95_TXPOWER 14

SSD1306 display(0x3c, 4, 15);


void message(String text, uint x=0, uint y=0, bool clear=false)
{
    if(clear)
    {
        display.clear();
    }
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);

    display.drawString(x, y, text);
    display.display();
}

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS_PIN, RFM95_IRQ_PIN);

// Class to manage message delivery and receipt, using the driver declared above
RHMesh manager(rf95, SERVER1_ADDRESS);

//Flag for Ctrl-C
int flag = 0;

void setup()
{
    Serial.begin(115200);

    pinMode(OLED_RST,OUTPUT);
    digitalWrite(OLED_RST, LOW);    // set GPIO16 low to reset OLED
    delay(50); 
    digitalWrite(OLED_RST, HIGH); // while OLED is running, must set GPIO16 in 
    printf("Display Init...\n\n");

    display.init();
    display.flipScreenVertically();  
    message("Mesh Server OK");
    message("Addr: ", 0, 12);
    message(String(SERVER1_ADDRESS), 60, 12);
}

int main();

void loop()
{
    main();
    ESP.restart();
}

//Main Function
int main ()
{

  gpio_set_direction(LED_PIN_NUM, GPIO_MODE_OUTPUT);
  printf("\nINFO: LED on GPIO %d\n", (uint8_t) RFM95_LED);
  gpio_set_level(LED_PIN_NUM, HIGH);
  delay(100);
  gpio_set_level(LED_PIN_NUM, LOW);


  if (!manager.init())
  {
    Serial.print( "\n\nMesh Manager Failed to initialize.\n\n" );
    return 1;
  }

  /* Begin Manager/Driver settings code */
  printf("\nRFM 95 Settings:\n");
  printf("Frequency= %d MHz\n", (uint16_t) RFM95_FREQUENCY);
  printf("Power= %d\n", (uint8_t) RFM95_TXPOWER);
  printf("Client Address= %d\n", CLIENT_ADDRESS);
  printf("Server(This) Address 1= %d\n", SERVER1_ADDRESS);
  printf("Server Address 2= %d\n", SERVER2_ADDRESS);
  printf("Server Address 3= %d\n", SERVER3_ADDRESS);
  printf("Route: Client->Server 3 is automatic in MESH.\n");
  rf95.setTxPower(RFM95_TXPOWER, false);
  rf95.setFrequency(RFM95_FREQUENCY);
  /* End Manager/Driver settings code */

  uint8_t data[] = "And hello back to you from server1";
  // Dont put this on the stack:
  uint8_t buf[RH_MESH_MAX_MESSAGE_LEN];

  while(!flag)
  {
    uint8_t len = sizeof(buf);
    uint8_t from;
    if (manager.recvfromAck(buf, &len, &from))
    {
      gpio_set_level(LED_PIN_NUM, HIGH);
      Serial.print("got request from : 0x");
      message(String("Req from: "), 0, 24);
      message(String(from) + "; " + String((char *)buf));
      Serial.print(from, HEX);
      Serial.print(": ");
      Serial.println((char*)buf);

      // Send a reply back to the originator client
      if (manager.sendtoWait(data, sizeof(data), from) != RH_ROUTER_ERROR_NONE)
        Serial.println("sendtoWait failed");

      gpio_set_level(LED_PIN_NUM, LOW);

    }
  }

  Serial.print( "\nrf95_mesh_server1 Tester Ending\n" );
  
  return 0;
}

void sig_handler(int sig)
{
  flag=1;
}