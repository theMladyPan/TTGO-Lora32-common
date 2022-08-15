
#include "ESP32Time.h"
#include "ArduinoJson.h"
#include <iostream>
// ESP32 Analog library
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include <string>

using namespace std;

struct gps_info_t {
    float lat=0;
    float lon=0;
    float spd=0;
    int alt = 0;
    int vsat = 0;
    int usat = 0;
};


bool tryAndSetTimeHttp(String *msgBody, ESP32Time *rtc)
{
    DynamicJsonDocument doc1(3072);    
    // You can use a Flash String as your JSON input.
    // WARNING: the strings in the input will be duplicated in the JsonDocument.
    DeserializationError errJ = deserializeJson(doc1, *msgBody);
    if(errJ)
    {
        cout << "Error: " << errJ << endl;
        return false;
    }
    JsonObject obj = doc1.as<JsonObject>();
    double time_s = obj["time"];

    rtc->setTime(time_s);
    return true;
}


bool tryAndSetTimeGSM(String *gsmTime, ESP32Time *rtc)
{
    // vyparsuj čas z 22/08/15,14:57:57+08
    tm ts;
    string gt(gsmTime->c_str());
    ts.tm_year = atoi(gt.substr(0, 2).c_str()) + 100;
    ts.tm_mon= atoi(gt.substr(3, 2).c_str()) - 1;
    ts.tm_mday = atoi(gt.substr(6, 2).c_str());
    ts.tm_hour = atoi(gt.substr(9, 2).c_str());
    ts.tm_min = atoi(gt.substr(12, 2).c_str());
    ts.tm_sec = atoi(gt.substr(15, 2).c_str());

    rtc->setTimeStruct(ts);

    return true;
}


float adc1_read_auto(adc1_channel_t adc_ch, uint16_t samples=1)
{
    adc_atten_t atten;
    uint64_t raw = 0;
    esp_adc_cal_characteristics_t adc1_chars;

    adc1_config_channel_atten(adc_ch, atten);
    esp_adc_cal_characterize(ADC_UNIT_1, atten, ADC_WIDTH_BIT_12, 0, &adc1_chars);

    // read approximate adc input in mV
    uint32_t mV = esp_adc_cal_raw_to_voltage(adc1_get_raw(adc_ch), &adc1_chars);    

    if(mV < 750)
    {
        // output below 750mv, no attenuation needed
        atten = ADC_ATTEN_DB_0;
        adc1_config_channel_atten(adc_ch, atten);
        esp_adc_cal_characterize(ADC_UNIT_1, atten, ADC_WIDTH_BIT_12, 0, &adc1_chars);
        #ifdef VERBOSE
        Serial.println("Atten: 0DB");
        #endif
    }else if (mV < 1000)
    {
        // apply 1.33x attenuation
        atten = ADC_ATTEN_DB_2_5;
        adc1_config_channel_atten(adc_ch, atten);
        esp_adc_cal_characterize(ADC_UNIT_1, atten, ADC_WIDTH_BIT_12, 0, &adc1_chars);
        #ifdef VERBOSE
        Serial.println("Atten: 2_5DB");
        #endif
    }else if (mV < 1500)
    {
        // apply 2x attenuation
        atten = ADC_ATTEN_DB_6;
        adc1_config_channel_atten(adc_ch, atten);
        esp_adc_cal_characterize(ADC_UNIT_1, atten, ADC_WIDTH_BIT_12, 0, &adc1_chars);
        #ifdef VERBOSE
        Serial.println("Atten: 6DB");
        #endif
    }else{
        // max attenuation needed for best result. Already set up.
        #ifdef VERBOSE
        Serial.println("Atten: 11DB");
        #endif
    }
    
    for (uint16_t j=0; j<samples; j++)
    {
        raw += (uint32_t)adc1_get_raw(adc_ch);
    }
    
    mV = esp_adc_cal_raw_to_voltage(raw/samples, &adc1_chars);    

    return (float)mV / 1000.0;
}

float adc1_read(adc1_channel_t adc_ch, uint16_t samples=1)
{
    esp_adc_cal_characteristics_t adc1_chars;
    adc1_config_channel_atten(adc_ch, ADC_ATTEN_DB_11);
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 0, &adc1_chars);

    uint64_t raw;

    for (uint16_t j=0; j<samples; j++)
    {
        raw += (uint32_t)adc1_get_raw(adc_ch);
    }
    
    uint32_t mV = esp_adc_cal_raw_to_voltage(raw/samples, &adc1_chars);    

    return (float)mV / 1000.0;
}


/**
 * @brief Get the Gps Info object
 * 
 * @param modem - e.g. SIM7000
 * @param gpsInfo - struct to be filled with
 * @param usec 0-2^32 = 0-4294 sec to wait for gps data
 * @return true 
 * @return false 
 */
bool getGpsInfo(TinyGsm *modem, gps_info_t *gpsInfo, uint32_t usec)
{
    timeval start, now;
    gettimeofday(&start, NULL);
    uint32_t elapsed;
    float lat, lon, spd;
    int alt, vsat, usat;

    do{
        gettimeofday(&now, NULL);
        modem->getGPS(&lat, &lon, &spd, &alt, &vsat, &usat);
        elapsed = (now.tv_sec - start.tv_sec)*1000000 + (now.tv_usec - start.tv_usec) ;
    }while ((lat == 0 || lon == 0 || alt == 0) && elapsed < usec);

    gpsInfo->lat = lat;
    gpsInfo->lon = lon;
    gpsInfo->alt = alt;
    gpsInfo->spd = spd;
    gpsInfo->usat = usat;
    gpsInfo->vsat = vsat;

    return (lat!=0 && lon!=0 && alt != 0);
}