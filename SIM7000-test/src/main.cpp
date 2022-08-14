/*
Battery test T-SIM7000 & Thingsboard
*/

#define TINY_GSM_MODEM_SIM7000
//#define DEBUG

#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb
#undef DUMP_AT_COMMANDS

#include "TinyGsmClient.h"
#include <ArduinoHttpClient.h>
//#include <AESLib.h>
#include <xbase64.h>
#include <ArduinoJson.h>
//#include "BigInt.h"
#include "modem.h"
//#include "mbedtls/rsa.h"
#include "ESP32Time.h"
#include <iostream>
#include "utils.h"
#include <string>

#include <vector>

#define ADC_BAT ADC1_CHANNEL_7
#define ADC_SOLAR ADC1_CHANNEL_3
#define ADC_ATTN ADC_ATTEN_DB_11	

#define PIN_LED_NUM 12
#define GPIO_PIN_LED (gpio_num_t)PIN_LED_NUM


extern "C" {
#include "bootloader_random.h"
}

#define VERBOSE 1
//AESLib aesLib;


#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
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

// Initialize GSM client
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

// Set to true, if modem is connected
bool modemConnected = false;

void shutdown();
void wait_till_ready();
void modem_off();

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
    delay(100);
    Serial.flush();
    esp_deep_sleep_start();
}


void getAesKey(byte *aes_key)
{
    bootloader_random_enable();
    esp_fill_random(aes_key, 16);
    bootloader_random_disable();
}

/*
// Generate IV (once)
void aes_init() {
    aesLib.set_paddingmode((paddingMode)0);
}
*/

unsigned char ciphertext[2*16] = {0}; // THIS IS OUTPUT BUFFER (FOR BASE64-ENCODED ENCRYPTED DATA)

/*
uint16_t encrypt_to_ciphertext(char * msg, uint16_t msgLen, byte iv[]) {
    byte aes_iv[16] = {0};
    aesLib.gen_iv(aes_iv);

    Serial.println("Calling encrypt (string)...");
    // aesLib.get_cipher64_length(msgLen);
    int cipherlength = aesLib.encrypt((byte*)msg, msgLen, (char*)ciphertext, aes_key, sizeof(aes_key), iv);
                    // uint16_t encrypt(byte input[], uint16_t input_length, char * output, byte key[],int bits, byte my_iv[]);
    return cipherlength;
}
*/

uint16_t connectModem()
{
    Serial.println("configuring GSM mode"); // AUTO or GSM ONLY

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

    modemConnected = true;
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

// TODO: Toto treba spraviť
/*
void cipherTest(){
    String *msgBody = new String();
    httpGet(msgBody, url_handshake, true);
    
    DynamicJsonDocument doc1(3072);

    // You can use a Flash String as your JSON input.
    // WARNING: the strings in the input will be duplicated in the JsonDocument.
    DeserializationError errJ = deserializeJson(doc1, *msgBody);
    if(errJ)
    {
        cout << "Error: " << errJ << endl;
        return;
    }

    JsonObject obj = doc1.as<JsonObject>();
    JsonArray n = obj["n_array"];

    uint8_t un[n.size()];

    copyArray(n, un, n.size());
    Serial.println("Memcpy OK.");

    BigInt e = 65537, mod, msg;

    mod.importData(un, sizeof(un));
    cout << "Import N OK!" << endl;

    mod.print();

    byte aes_key[16] = {0};
    getAesKey(aes_key);

    Serial.print("aes: ");
    for(auto i=0; i<16; i++)
    {
        Serial.print( aes_key[i], HEX);
    }
    cout << endl << endl;

    // tested OK
    msg.importData(aes_key, 16);

    cout << "aes_int: " << endl;
    msg.print();
    
    // FIXME: test failed
    BigInt cipher = modPow(msg,e,mod);
    cout << "Ciphered:" << endl;
    cipher.print();

    // tested OK
    uint8_t cipher_array[128] = {0};
    bool sign;
    cipher.exportData(cipher_array, sizeof(cipher_array), sign);
    Serial.print("Cipher array: ");
    for(int i=0; i<128; i++)
    {
        Serial.print(cipher_array[i], HEX);
    }
    cout << endl;
        
    // tested OK
    char encoded[256] = {0};
    base64_encode(encoded, (const char*)cipher_array, 256);

    cout << "B64 Encoded: " << encoded << endl;

    String *senc = new String(encoded);
    Serial.println(String("Posting: ") + *senc);
    //int err = http.post(url_register, F("text/plain"), senc);
    httpPost(msgBody, url_register, senc, true);

    *msgBody = http.responseBody();
    Serial.println(F("Response: "));
    Serial.println(*msgBody);
}
*/


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
        delay(200);
        ledOff();
        delay(200);
    }
}


void setup()
{
    pinMode(PIN_LED_NUM, OUTPUT);
}

void loop()
{
    // Set console baud rate
    Serial.begin(SERIAL_DEBUG_BAUD);
    delay(10);
    Serial.println(F("Started"));
    Serial.print("Current time is: ");
    Serial.println(rtc.getDateTime(true));
    
    float vBat = adc1_read_precise(ADC_BAT, 1024)*2;

    cout << "Vbat: " << vBat << "mV." << endl;

    delay(100);
    flash(1);
    
    //Increment boot number and print it every reboot
    ++bootCount;
    Serial.println("Boot number: " + String(bootCount));

    //Print the wakeup reason for ESP32
    print_wakeup_reason();

    /*
    First we configure the wake up source
    We set our ESP32 to wake up every 5 seconds
    */
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
                    " Seconds");

    modemio.on();
    modem.enableGPS();
    // Set GSM module baud rate and UART pins
    SerialAT.begin(9600, SERIAL_8N1, MODEM_TX, MODEM_RX); //reversing them
    String modemInfo = modemio.info();
    Serial.print(F("Modem: "));
    Serial.println(modemInfo);

    if (!modemConnected)   
    {
        connectModem();
    }

    String *msgBody = new String();

    if(rtc.getEpoch() < 1000000000)
    {
        httpGet(msgBody, url_welcome, VERBOSE);
        tryAndSetTime(msgBody, &rtc);
    }
    
    Serial.print("Web DateTime: ");
    Serial.println(rtc.getDateTime());
    
    
    float lat, lon, spd;
    int alt, vsat, usat;
    modem.getGPS(&lat, &lon, &spd, &alt, &vsat, &usat);

    DynamicJsonDocument doc1(2048);

    doc1["time"]["epoch"] = rtc.getEpoch();
    doc1["time"]["gm_time"] = rtc.getDateTime();
    doc1["time"]["gsm"] = modem.getGSMDateTime(DATE_FULL);

    doc1["vbat"] = vBat;
    doc1["info"]["gsm"]["signal"] = modem.getSignalQuality();
    doc1["info"]["gsm"]["operator"] = modem.getOperator();
    doc1["info"]["gsm"]["ip"] = modem.getLocalIP();
    String GSMlocation(modem.getGsmLocation());
    if (GSMlocation != "")
    {
        doc1["info"]["gsm"]["location"] = GSMlocation;
    }

    if(vsat > 0)
    {
        #ifdef VERBOSE
        Serial.println("GPS ON");
        #endif //VERBOSE
        doc1["info"]["gps"]["lat"] = lat;
        doc1["info"]["gps"]["lon"] = lon;
        doc1["info"]["gps"]["alt"] = alt;
        doc1["info"]["gps"]["sat"] = vsat;      
        flash(5);
    }

    String jsonString;
    serializeJson(doc1, jsonString);

    httpPostJson(msgBody, url_post_plain, &jsonString, true);

    http.stop();
    Serial.println(F("Server disconnected"));

    delay(100); // required - else will shutdown too early and miss last value

    shutdown();

    //esp_restart();
}