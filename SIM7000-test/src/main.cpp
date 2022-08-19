/*
Battery test T-SIM7000 & Thingsboard
*/

#define TINY_GSM_MODEM_SIM7000

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
#include <esp_task_wdt.h>
#include "mics.h"

// use OLED 128*64
#include "Wire.h"
#include "SSD1306.h" 
#define OLED_SDA 21
#define OLED_SCL 22
SSD1306 display(0x3c, OLED_SDA, OLED_SCL);

#define WAIT_FOR_GPS_S 1
#define WDT_TIMEOUT 30 // seconds
#define WAIT_FOR_HEATER_MS 60*1000 // miliseconds

#define ADC_BAT ADC1_GPIO35_CHANNEL
#define ADC_SOLAR ADC1_GPIO36_CHANNEL

#define PIN_LED_NUM 12
#define GPIO_PIN_LED (gpio_num_t)PIN_LED_NUM


#define Y2K1 1000000000
#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
#define US_IN_S 1000000
#define TIME_TO_SLEEP 30       /* ESP32 should sleep more seconds  (note SIM7000 needs ~20sec to turn off if sleep is activated) */
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

#define LED_PIN 12

#define CO_PIN unused_pins.pin32
#define NOx_PIN unused_pins.pin33
#define VOC_PIN unused_pins.pin34

#define ADC_CO ADC1_GPIO32_CHANNEL
#define ADC_NOx ADC1_GPIO33_CHANNEL
#define ADC_VOC ADC1_GPIO34_CHANNEL

#define PIN_ADC_BAT 35
#define PIN_ADC_SOLAR 36
#define ADC_SAMPLES 128

// define preffered connection mode
// 2 Auto // 13 GSM only // 38 LTE only
#define CONNECTION_MODE 2  // Auto

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

void displayPrint(String text, int x=0, int y=0, bool clear=true)
{
    if(clear)
    {
        display.setColor(BLACK);
        display.fillRect(x, y, 128, 16);
        display.setColor(WHITE);
    }

    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawStringMaxWidth(x, y, 128, text);
    display.display();
}


void S(String text)
{
    displayPrint(text, 0, 54, true);
    delay(500);
}

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
    // modemio.off();

    Serial.println("Going to sleep now");
    S("Sleeping...");
    display.setBrightness(5);
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


void setup()
{
    pinMode(PIN_LED_NUM, OUTPUT);

    setCpuFrequencyMhz(40);    
    Serial.begin(SERIAL_DEBUG_BAUD);
    // Set GSM module baud rate and UART pins
    SerialAT.begin(115200, SERIAL_8N1, MODEM_TX, MODEM_RX); 

    display.init();
    display.displayOn();
    ledOff();
}


void loop()
{   
    esp_task_wdt_init(WDT_TIMEOUT, true); //enable panic so ESP32 restarts
    esp_task_wdt_add(NULL); //add current thread to WDT watch

    MicsSensor *airpol = new MicsSensor(ADC_CO, ADC_NOx, ADC_VOC, unused_pins.pin18);
    airpol->init(WAIT_FOR_HEATER_MS, ADC_SAMPLES);

    Serial.print("Started heater of sensor.");

    modemio.on(); // starting modem, Ton~1s
    
    display.displayOn();
    display.clear();
    display.setBrightness(255);

    S("Getting power ...");

    // Set console baud rate
    DynamicJsonDocument doc1(2048);

    // log the voltages
    float Vbat, Vsol;
    Vbat = adc1_read_auto(ADC_BAT, ADC_SAMPLES)*2;
    Vsol = adc1_read_auto(ADC_SOLAR, ADC_SAMPLES)*2;
    doc1["info"]["power"]["Vbat"] = Vbat;
    doc1["info"]["power"]["Vsolar"] = Vsol;
    displayPrint(String("Vbat: ") + String(Vbat), 0, 0);
    displayPrint(String("Vsol: ") + String(Vsol), 0, 10);
    

    int64_t start = esp_timer_get_time();

    
    //Increment boot number and print it every reboot
    ++bootCount;
    Serial.println("Boot number: " + String(bootCount));
    S("Boot: "+String(bootCount));

    //Print the wakeup reason for ESP32
    print_wakeup_reason();

    
    String modemInfo = modemio.info();
    Serial.print(F("Modem: "));
    Serial.println(modemInfo);
   
    esp_task_wdt_reset(); // reset watchdog
    if (!modem.isGprsConnected())   
    {
        Serial.println("Modem disconected. Reconnecting...");
        S("Connecting to GPRS...");
        connectModem();
    }
    S("Enabling GPS...");
    if(modem.enableGPS())
    {
        Serial.println("GPS Enabled!");
        S("GPS Enabled!");
    }

    esp_task_wdt_reset(); // reset watchdog

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


    esp_task_wdt_reset(); // reset watchdog
    doc1["time"]["epoch"] = rtc.getEpoch();
    doc1["time"]["local"] = rtc.getDateTime(true);

    Serial.println("GSM Time: " + gsmTime);
    S(gsmTime);

    doc1["info"]["gsm"]["signal"] = modem.getSignalQuality();
    doc1["info"]["gsm"]["operator"] = modem.getOperator();
    doc1["info"]["gsm"]["ip"] = modem.getLocalIP();
    // doc1["temp"]["modem"] = modem.getTemperature(); not available on this modem

    String GSMlocation(modem.getGsmLocation());
    if (GSMlocation != "")
    {
        doc1["info"]["gsm"]["location"] = GSMlocation;
    }
        

    // gather data from sensor
    S("Waiting for sensor.");
    Serial.print("Waiting for sensor ");
    int i=0;
    while(!airpol->ready())
    {
        Serial.print(".");
        if(i%4 == 0)S("Waiting for sensor   |");
        if(i%4 == 1)S("Waiting for sensor   /");
        if(i%4 == 2)S("Waiting for sensor   -");
        if(i++%4 == 3)S("Waiting for sensor   \\");
        esp_task_wdt_reset(); // reset watchdog
    }
    S("Reading polutants.");
    float Vco, Vnox, Vvoc;
    
    Vco = airpol->getCOVal();
    doc1["measurement"]["gas"]["CO"] = Vco;
    displayPrint("CO : " + String(Vco) , 0, 20);      
    
    Vnox = airpol->getNOxVal();
    doc1["measurement"]["gas"]["NOx"] = Vnox;
    displayPrint("NOX: " + String(Vnox), 0, 30);      
    
    Vvoc = airpol->getVOCVal();
    doc1["measurement"]["gas"]["VOC"] = Vvoc;
    displayPrint("VOC: " + String(Vvoc), 0, 40);

    airpol->idle();

    gps_info_t gpsInfo; 
    Serial.println("Gathering GPS cordinates");
    
    S("Reading GPS coord. ...");
    esp_task_wdt_init(WAIT_FOR_GPS_S + 1, true); // reconfigure wdt for upcoming delay
    getGpsInfo(&modem, &gpsInfo, WAIT_FOR_GPS_S*US_IN_S);
    esp_task_wdt_init(WDT_TIMEOUT, true); // back to normal wdt timeout
    doc1["info"]["gps"]["lat"] = gpsInfo.lat;
    doc1["info"]["gps"]["lon"] = gpsInfo.lon;
    doc1["info"]["gps"]["alt"] = gpsInfo.alt;    
    doc1["info"]["gps"]["visible_sat"] = gpsInfo.vsat;      
    doc1["info"]["gps"]["used_sat"] = gpsInfo.usat;   
       
    displayPrint("Lat: " + String(gpsInfo.lat), 64, 0);      
    displayPrint("Lon: " + String(gpsInfo.lon), 64, 10);      
    displayPrint("Alt: " + String(gpsInfo.alt), 64, 20);
    displayPrint("USat: " + String(gpsInfo.usat), 64, 30);      
    displayPrint("VSat: " + String(gpsInfo.vsat), 64, 40);
    

    String jsonString;
    serializeJson(doc1, jsonString);

    esp_task_wdt_reset(); // reset watchdog
    S("Posting data...");
    httpPostJson(msgBody, url_post_plain, &jsonString, true);

    S("Data sent!");
    http.stop();
    Serial.println(F("Server disconnected"));
    int64_t elapsed = (int64_t)(esp_timer_get_time() - start)/1000;
    cout << "Posted in: " << elapsed << "ms" << endl;
    Serial.flush(); // wait for tx to complete
    
    esp_task_wdt_reset(); // reset watchdog
    // esp_task_wdt_init(70, true); // reconfigure wdt for upcoming delay
    
    //delay(TIME_TO_SLEEP*1000);
    modemio.off();
    shutdown();
    // esp_light_sleep_start();
    // esp_restart();
}

/*
curl --location --request POST 'https://data.mongodb-api.com/app/data-ylgrg/endpoint/data/v1/action/findOne' \
--header 'Content-Type: application/json' \
--header 'Access-Control-Request-Headers: *' \
--header 'api-key: hcmcMYy4lqeo9XsWGg0BL8cvWdIEdZVUYsjI06hA5e8Rk6Z6qE6KKxVNxSw15vno' \
--data-raw '{
    "collection":"66666",
    "database":"sim_test",
    "dataSource":"xerxes",
    "projection": {"_id": 1}
}'

*/