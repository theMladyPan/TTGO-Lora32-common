
#include "ESP32Time.h"
#include "ArduinoJson.h"
#include <iostream>
// ESP32 Analog library
#include "driver/adc.h"
#include "esp_adc_cal.h"

using namespace std;

bool tryAndSetTime(String *msgBody, ESP32Time *rtc)
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


float adc1_read_precise(adc1_channel_t adc_ch, uint16_t samples=1)
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
    }else if (mV < 1000)
    {
        // apply 1.33x attenuation
        atten = ADC_ATTEN_DB_2_5;
        adc1_config_channel_atten(adc_ch, atten);
        esp_adc_cal_characterize(ADC_UNIT_1, atten, ADC_WIDTH_BIT_12, 0, &adc1_chars);
    }else if (mV < 1500)
    {
        // apply 2x attenuation
        atten = ADC_ATTEN_DB_6;
        adc1_config_channel_atten(adc_ch, atten);
        esp_adc_cal_characterize(ADC_UNIT_1, atten, ADC_WIDTH_BIT_12, 0, &adc1_chars);
    }else{
        // max attenuation needed for best result. Already set up.
    }
    
    for (uint16_t j=0; j<samples; j++)
    {
        raw += (uint32_t)adc1_get_raw(adc_ch);
    }
    
    mV = esp_adc_cal_raw_to_voltage(raw/samples, &adc1_chars);    

    return (float)mV / 1000.0;
}