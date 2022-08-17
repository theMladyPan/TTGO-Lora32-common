/*
Battery test T-SIM7000 & Thingsboard
*/

#define TINY_GSM_MODEM_SIM7000
#define VERBOSE 1

#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb
#undef DUMP_AT_COMMANDS

#include "TinyGsmClient.h"
#include <ArduinoHttpClient.h>
#include <xbase64.h>
#include <ArduinoJson.h>
#include "modem.h"
#include "ESP32Time.h"
#include <iostream>
#include "utils.h"
#include <ttgo-sim7000.h>
#include <soc/adc_channel.h>


#define ADC_BAT ADC1_GPIO35_CHANNEL
#define ADC_SOLAR ADC1_GPIO36_CHANNEL

#define PIN_LED_NUM 12
#define GPIO_PIN_LED (gpio_num_t)PIN_LED_NUM


extern "C" {
#include "bootloader_random.h"
}

//AESLib aesLib;

#define Y2K1 1000000000
#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
#define us_in_s 1000000
#define TIME_TO_SLEEP 60       /* ESP32 should sleep more seconds  (note SIM7000 needs ~20sec to turn off if sleep is activated) */
RTC_DATA_ATTR int bootCount = 0;

HardwareSerial serialGsm(1);
#define SerialAT serialGsm
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false

// O2 duh
const char apn[] = "o2internet";
const char nbiot_apn[] = "iot";
#define isNBIOT false

const char user[] = "";
const char pass[] = "";

// TTGO T-SIM pin definitions
#define MODEM_RST 5
#define MODEM_PWKEY 4
#define MODEM_DTR 25

#define MODEM_TX 26
#define MODEM_RX 27

#define I2C_SDA 21
#define I2C_SCL 22

#define LED_PIN 12

#define UNUSED_PINS


#define PIN_ADC_BAT 35
#define PIN_ADC_SOLAR 36
#define ADC_BATTERY_LEVEL_SAMPLES 100

// define preffered connection mode
// 2 Auto // 13 GSM only // 38 LTE only
#define CONNECTION_MODE 13  // Auto

#ifdef DUMP_AT_COMMANDS
#include "StreamDebugger.h"
StreamDebugger debugger(serialGsm, Serial);
TinyGsm modem(debugger);
#else
TinyGsm modem(serialGsm);
#endif

// Baud rate for debug serial
#define SERIAL_DEBUG_BAUD 115200

// Initialize GSM client+
TinyGsmClient client(modem);

ModemIO modemio(&modem, (gpio_num_t)MODEM_RST, (gpio_num_t)MODEM_PWKEY, (gpio_num_t)MODEM_DTR);

#define DEVID 66666

using namespace std;

// NOTE: Server details
const String TOKEN = "ltKh+leoyblBXyFvjnbaiw==";
const String dbname = "sim_test";
const String colname = String(DEVID);
const char url_handshake[] = "/handshake/";
const String url_welcome = "/welcome/";
const String url_register = "/register/" + String(DEVID) + "/"; 
const String url_post_plain = "/post/plain/" + dbname + "/" + colname + "/" + String(DEVID) + "/" + TOKEN + "/";

const char server[]   = "65.19.191.151";
const int  port       = 8000;
HttpClient http(client, server, port);

// create RTC instance:
ESP32Time rtc(0);


void print_wakeup_reason()
{
    esp_sleep_wakeup_cause_t wakeup_reason;

    wakeup_reason = esp_sleep_get_wakeup_cause();

    switch (wakeup_reason)
    {
    case ESP_SLEEP_WAKEUP_EXT0:
        Serial.println("Wakeup caused by external signal using RTC_IO");
        break;
    case ESP_SLEEP_WAKEUP_EXT1:
        Serial.println("Wakeup caused by external signal using RTC_CNTL");
        break;
    case ESP_SLEEP_WAKEUP_TIMER:
        Serial.println("Wakeup caused by timer");
        break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
        Serial.println("Wakeup caused by touchpad");
        break;
    case ESP_SLEEP_WAKEUP_ULP:
        Serial.println("Wakeup caused by ULP program");
        break;
    default:
        Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
        break;
    }
}


void shutdown()
{
    // modemio.sleep();
    modemio.off();

    Serial.println("Going to sleep now");
    delay(10);
    Serial.flush();
    esp_deep_sleep(TIME_TO_SLEEP * uS_TO_S_FACTOR);
}


uint16_t connectModem()
{

    modem.setPreferredMode(CONNECTION_MODE); //2 Auto // 13 GSM only // 38 LTE only

    Serial.print(F("Waiting for network..."));
    if (!modem.waitForNetwork(60000L))
    {
        Serial.println(" fail");
        modemio.reset();
        shutdown();
    }
    Serial.println(" OK");

    uint16_t quality = modem.getSignalQuality();
    Serial.print("Signal quality:");
    Serial.println(quality);

    Serial.print(F("Connecting to "));
    Serial.println(apn);
    if (!modem.gprsConnect(apn, user, pass))
    {
        Serial.println(" failed");
        modemio.reset();
        shutdown();
    }

    return quality;
}

int httpGet(String *body, const String url, bool verbose=false)
{
    if(verbose)Serial.print(F("Performing HTTP GET request... "));
    int err = http.get(url);
    if (err != 0) {
        if(verbose){
            Serial.println(F("failed to connect"));
            Serial.print("ErrNrr: ");
            Serial.println(String(err));
            delay(100);
        }
        return 0;
    }

    int status = http.responseStatusCode();
    if(verbose){
        Serial.println(F("Response status code: "));
        Serial.println(status);
    }
    if (!status) {
        delay(1000);
        return 0;
    }

    if(verbose)Serial.println(F("Response Headers:"));
    while (http.headerAvailable()) {
        String headerName  = http.readHeaderName();
        String headerValue = http.readHeaderValue();
        if(verbose)Serial.println("    " + headerName + " : " + headerValue);
    }
        

    int length = http.contentLength();
    if (length >= 0 && verbose) {
        Serial.print(F("Content length is: "));
        Serial.println(length);
    }
    if (http.isResponseChunked()) {
        Serial.println(F("The response is chunked"));
    }

    *body = http.responseBody();
    Serial.println(F("Response:"));
    Serial.println(*body);

    return status;
}

int httpPostJson(String *body, const String url, String *data, bool verbose=false)
{
    if(verbose)Serial.println(F("Performing HTTP POST request, sending: "));
    if(verbose)Serial.println(*data);

    int err = http.post(url, F("document/json"), *data);
    if (err != 0) {
        if(verbose){
            Serial.println(F("failed to connect"));
            Serial.print("ErrNrr: ");
            Serial.println(String(err));
            delay(100);
        }
        return 0;
    }

    int status = http.responseStatusCode();
    if(verbose){
        Serial.println(F("Response status code: "));
        Serial.println(status);
    }
    if (!status) {
        delay(1000);
        return 0;
    }

    if(verbose)Serial.println(F("Response Headers:"));
    while (http.headerAvailable()) {
        String headerName  = http.readHeaderName();
        String headerValue = http.readHeaderValue();
        if(verbose)Serial.println("    " + headerName + " : " + headerValue);
    }
        

    int length = http.contentLength();
    if (length >= 0 && verbose) {
        Serial.print(F("Content length is: "));
        Serial.println(length);
    }
    if (http.isResponseChunked()) {
        Serial.println(F("The response is chunked"));
    }

    *body = http.responseBody();
    Serial.println(F("Response:"));
    Serial.println(*body);

    return status;
}


void ledOn()
{
    digitalWrite(PIN_LED_NUM, LOW);
}


void ledOff()
{
    digitalWrite(PIN_LED_NUM, HIGH);
}


void flash(int n)
{
    for(auto i=0; i<n; i++)
    {
        ledOn();
        delay(100);
        ledOff();
        delay(100);
    }
}


void setup()
{
    pinMode(PIN_LED_NUM, OUTPUT);

    setCpuFrequencyMhz(40);    
    Serial.begin(SERIAL_DEBUG_BAUD);
}

void loop()
{
    // Set console baud rate
    DynamicJsonDocument doc1(2048);

    doc1["info"]["power"]["Vbat"] = adc1_read_auto(ADC_BAT, 128)*2;
    doc1["info"]["power"]["Vsolar"] = adc1_read_auto(ADC_SOLAR, 128)*2;


    auto start = rtc.getEpoch();

    flash(2);
    
    //Increment boot number and print it every reboot
    ++bootCount;
    Serial.println("Boot number: " + String(bootCount));

    //Print the wakeup reason for ESP32
    print_wakeup_reason();

    modemio.on();
    // modem.enableGPS();
    // modem.getBattVoltage();

    // Set GSM module baud rate and UART pins
    SerialAT.begin(115200, SERIAL_8N1, MODEM_TX, MODEM_RX);

    String modemInfo = modemio.info();
    Serial.print(F("Modem: "));
    Serial.println(modemInfo);
   
    if (!modem.isGprsConnected())   
    {
        Serial.println("modem disconected. Reconnecting...");
        connectModem();
    }

    String *msgBody = new String();

    ledOn();
    String gsmTime = modem.getGSMDateTime(DATE_FULL);
    if(rtc.getEpoch() < Y2K1)
    {
        tryAndSetTimeGSM(&gsmTime, &rtc);
        Serial.print("Current time is: ");
        Serial.println(rtc.getDateTime(true));
    }    
    ledOff();


    doc1["time"]["epoch"] = rtc.getEpoch();
    doc1["time"]["local"] = rtc.getDateTime(true);

    Serial.println("GSM Time: " + gsmTime);

    doc1["info"]["power"]["%Bat"] = modem.getBattPercent();
    doc1["info"]["gsm"]["signal"] = modem.getSignalQuality();
    doc1["info"]["gsm"]["operator"] = modem.getOperator();
    doc1["info"]["gsm"]["ip"] = modem.getLocalIP();
    // doc1["temp"]["modem"] = modem.getTemperature(); not available on this modem

    String GSMlocation(modem.getGsmLocation());
    if (GSMlocation != "")
    {
        doc1["info"]["gsm"]["location"] = GSMlocation;
    }

    /* ditch the gps for now */
    gps_info_t gpsInfo;
    getGpsInfo(&modem, &gpsInfo, 1*us_in_s);

    doc1["info"]["gps"]["lat"] = gpsInfo.lat;
    doc1["info"]["gps"]["lon"] = gpsInfo.lon;
    doc1["info"]["gps"]["alt"] = gpsInfo.alt;
    doc1["info"]["gps"]["visible_sat"] = gpsInfo.vsat;      
    doc1["info"]["gps"]["used_sat"] = gpsInfo.usat;      
    

    String jsonString;
    serializeJson(doc1, jsonString);

    ledOn();
    httpPostJson(msgBody, url_post_plain, &jsonString, true);
    ledOff();

    http.stop();
    Serial.println(F("Server disconnected"));
    Serial.println("Posted in: " + String(rtc.getEpoch() - start) + "s");
    Serial.flush(); // wait for tx to complete
    
    // delay instead of sleep
    delay(60000);
    // shutdown();
    // esp_light_sleep_start();
    // esp_restart();
}