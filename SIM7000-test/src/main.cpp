/*
Battery test T-SIM7000 & Thingsboard
*/

#define TINY_GSM_MODEM_SIM7000
//#define DEBUG

#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb
#undef DUMP_AT_COMMANDS

#include "TinyGsmClient.h"
#include <ArduinoHttpClient.h>
#include <AESLib.h>
#include <xbase64.h>
#include <ArduinoJson.h>
#include "BigInt.h"
#include "modem.h"
#include "mbedtls/rsa.h"

extern "C" {
#include "bootloader_random.h"
}


AESLib aesLib;


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


// Server details
const char server[]   = "65.19.191.151";
const char url_resource[] = "/xerxes";
const char url_handshake[] = "/handshake/";
String url_register = "/register/" + String(DEVID) + "/"; 
const int  port       = 8000;
HttpClient http(client, server, port);

// Set to true, if modem is connected
bool modemConnected = false;

void read_adc_bat(uint16_t *voltage);
void read_adc_solar(uint16_t *voltage);
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
    //modem_sleep();
    modemio.off();

    delay(100);
    Serial.flush();
    esp_deep_sleep_start();
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

// Generate IV (once)
void aes_init() {
    aesLib.set_paddingmode((paddingMode)0);
}

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

void setup()
{
    // Set console baud rate
    Serial.begin(SERIAL_DEBUG_BAUD);
    delay(10);
    Serial.println(F("Started"));
    delay(3000);
    

    /*
    pinMode(PIN_ADC_BAT, ANALOG);
    pinMode(PIN_ADC_SOLAR, ANALOG);

    uint16_t v_bat = 0;
    uint16_t v_solar = 0;

    while (1)
    {
    read_adc_bat(&v_bat);
    Serial.print("BAT: ");
    Serial.print(v_bat);

    read_adc_solar(&v_solar);
    Serial.print(" SOLAR: ");
    Serial.println(v_solar);

    delay(1000);
    }*/

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

    // Set GSM module baud rate and UART pins
    SerialAT.begin(9600, SERIAL_8N1, MODEM_TX, MODEM_RX); //reversing them
    String modemInfo = modemio.info();
    Serial.print(F("Modem: "));
    Serial.println(modemInfo);

    if (!modemConnected)   
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

        Serial.print("Signal quality:");
        Serial.println(modem.getSignalQuality());

        Serial.print(F("Connecting to "));
        Serial.println(apn);
        if (!modem.gprsConnect(apn, user, pass))
        {
            Serial.println(" failed");
            modemio.reset();
            shutdown();
        }

        modemConnected = true;
        Serial.println("#-#-# GSM OK #-#-#");
    }


    Serial.print(F("Performing HTTP GET request... "));
    int err = http.get(url_handshake);
    if (err != 0) {
        Serial.println(F("failed to connect"));
        Serial.print("ErrNrr: ");
        Serial.println(String(err));
        delay(1000);
        return;
    }

    int status = http.responseStatusCode();
    Serial.println(F("Response status code: "));
    Serial.println(status);
    if (!status) {
        delay(10000);
        return;
    }

    Serial.println(F("Response Headers:"));
    while (http.headerAvailable()) {
        String headerName  = http.readHeaderName();
        String headerValue = http.readHeaderValue();
        Serial.println("    " + headerName + " : " + headerValue);
    }

    int length = http.contentLength();
    if (length >= 0) {
        Serial.print(F("Content length is: "));
        Serial.println(length);
    }
    if (http.isResponseChunked()) {
        Serial.println(F("The response is chunked"));
    }

    String body = http.responseBody();
    Serial.println(F("Response:"));
    Serial.println(body);


    DynamicJsonDocument doc1(3072);

    // You can use a Flash String as your JSON input.
    // WARNING: the strings in the input will be duplicated in the JsonDocument.
    DeserializationError errJ = deserializeJson(doc1, body);
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
    
    // TODO: test failed
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

    String senc = String(encoded);
    Serial.println(String("Posting: ") + senc);
    err = http.post(url_register, F("text/plain"), senc);

    body = http.responseBody();
    Serial.println(F("Response: "));
    Serial.println(body);

    http.stop();
    Serial.println(F("Server disconnected"));

    delay(100); // required - else will shutdown too early and miss last value
    shutdown();
}

void loop()
{
}

void read_adc_bat(uint16_t *voltage)
{
    uint32_t in = 0;
    for (int i = 0; i < ADC_BATTERY_LEVEL_SAMPLES; i++)
    {
        in += (uint32_t)analogRead(PIN_ADC_BAT);
    }
    in = (int)in / ADC_BATTERY_LEVEL_SAMPLES;

    uint16_t bat_mv = ((float)in / 4096) * 3600 * 2;

    *voltage = bat_mv;
}

void read_adc_solar(uint16_t *voltage)
{
    uint32_t in = 0;
    for (int i = 0; i < ADC_BATTERY_LEVEL_SAMPLES; i++)
    {
        in += (uint32_t)analogRead(PIN_ADC_SOLAR);
    }
    in = (int)in / ADC_BATTERY_LEVEL_SAMPLES;

    uint16_t bat_mv = ((float)in / 4096) * 3600 * 2;

    *voltage = bat_mv;
}