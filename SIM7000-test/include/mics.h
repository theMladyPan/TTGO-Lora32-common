 #ifndef MICS_H
 #define MICS_H

#include <esp32-hal-gpio.h>
#include "utils.h"


typedef enum{
    CO,
    NOx,
    VOC,
} gas_group_t;


class MicsSensor
{
private:
    void heaterOff();
    void heaterOn();
    void timeStamp();
    gpio_num_t gpio_heater_en;
    adc1_channel_t co_pin;
    adc1_channel_t nox_pin;
    adc1_channel_t voc_pin;
    //timeval *heater_start;
    uint64_t usec_heater_start;
    uint64_t time_to_ready_us;
    float Vin;
    uint RlCO = 100000; // 100kOhm
    uint RlNOx = 10000; // 10kOhm
    uint RlVOC = 100000; // 100kOhm
    uint n_samples;

    float R0CO = 4e5;
    float R0NOx = 1e4;
    float R0VOC = 1e5;

    double Vo2Rs(float Vo, gas_group_t gas);

public:
    MicsSensor(adc1_channel_t co_pin, adc1_channel_t nox_pin, adc1_channel_t voc_pin, gpio_num_t en_pin);
    MicsSensor(adc1_channel_t co_pin, adc1_channel_t nox_pin, adc1_channel_t voc_pin, int en_pin) : MicsSensor(co_pin, nox_pin, voc_pin, (gpio_num_t)en_pin){};
    ~MicsSensor();
    bool ready();
    void init(uint t_ready_ms, uint n_samples, float Vin=3.3);
    void idle();
    void setLoadR(uint RlCO, uint RlNOx, uint RlVOC);
    void setCalibrationR0(uint R0CO, uint R0NOx, uint R0VOC);

    double getCOVal();
    double getNOxVal();
    double getVOCVal();   
    double getCORaw();
    double getNOxRaw();
    double getVOCRaw();    
};

MicsSensor::MicsSensor(adc1_channel_t co_pin, adc1_channel_t nox_pin, adc1_channel_t voc_pin, gpio_num_t en_pin)
{
    this->gpio_heater_en = en_pin;
    this->co_pin = co_pin;
    this->nox_pin = nox_pin;
    this->voc_pin = voc_pin;
}


MicsSensor::~MicsSensor()
{
}

void MicsSensor::idle()
{
    heaterOff();
}

void MicsSensor::setLoadR(uint RlCO, uint RlNOx, uint RlVOC)
{
    this->RlCO = RlCO;
    this->RlNOx = RlNOx;
    this-> RlVOC = RlVOC;
}

/*
    def Vo_to_Rs(self, Vo:float) -> float:
        Rs = self._Rl / (self._v_in * ((1/Vo) - (1/self._v_in)))
        return Rs
*/
double MicsSensor::Vo2Rs(float Vo, gas_group_t gas)
{
    double Rs, Rl=0;
    if(gas == CO)
    {
        Rl = this->RlCO;
    }else if(gas == NOx)
    {
        Rl = this->RlNOx;
    }else if(gas == VOC)
    {
        Rl = this->RlVOC;
    }

    Rs = Rl / ( this->Vin * ( (1/Vo) - (1/this->Vin)) );
    return Rs;
}


/*
    def measure(self, n_samples: int) -> float:
        Vo = self.sample(n_samples)
        Rs = self.Vo_to_Rs(Vo)
        return self._tf(Rs/self._Ro)
        # ppm = 0,162*x + -9,75E-03; where x=Rs/Ro
        */

double MicsSensor::getCORaw()
{
    float Vo = adc1_read_auto(this->co_pin, this->n_samples);
    return Vo2Rs(Vo, CO);   
}


double MicsSensor::getCOVal()
{
    double Rs = getCORaw();
    double x = Rs/this->R0CO;
    // (4.17*x)**(-1.18)
    double val = pow(4.17*x, -1.18);
    return val;
}


double MicsSensor::getNOxRaw()
{
    float Vo = adc1_read_auto(this->nox_pin, this->n_samples);
    return Vo2Rs(Vo, NOx);
}


double MicsSensor::getNOxVal()
{
    double Rs = getNOxRaw();
    double x = Rs/this->R0NOx;
    //(.162*x)-9.75e-3)
    double val = 0.162*x -9.75e-3;
    return val;
}


double MicsSensor::getVOCRaw()
{
    float Vo = adc1_read_auto(this->voc_pin, this->n_samples);
    return Vo2Rs(Vo, VOC);
}


double MicsSensor::getVOCVal()
{
    double Rs = getVOCRaw();
    double x = Rs/this->R0VOC;
    // (.277*x)**(-2.59)
    double val = pow(0.277*x, -2.59);
    return val;
}


void MicsSensor::timeStamp()
{
    this->usec_heater_start = esp_timer_get_time();
    //gettimeofday(this->heater_start, NULL);
}


void MicsSensor::init(uint t_ready_ms, uint n_samples, float Vin)
{
    heaterOn();
    timeStamp();
    gpio_set_direction(this->gpio_heater_en, GPIO_MODE_OUTPUT);
    this->time_to_ready_us = (uint64_t)t_ready_ms*1000;
    this->Vin = Vin;
    this->n_samples = n_samples;
}


bool MicsSensor::ready()
{
    // timeval now;
    // gettimeofday(&now, NULL);
    // int64_t usec_now = (int64_t)now.tv_sec*1000000L + (int64_t)now.tv_usec;
    // int64_t usec_start = (int64_t)this->heater_start->tv_sec*1000000L + (int64_t)this->heater_start->tv_usec;
    uint64_t usec_now = esp_timer_get_time();
    return (usec_now - this->usec_heater_start) >= (this->time_to_ready_us);
}
    

void MicsSensor::heaterOn()
{   
    gpio_set_level(this->gpio_heater_en, 1);
}


void MicsSensor::heaterOff()
{
    gpio_set_level(this->gpio_heater_en, 0);
}

#endif // MICS_H