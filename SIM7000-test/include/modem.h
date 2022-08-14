
#include "TinyGsmClient.h"
#include "Arduino.h"

class ModemIO
{
private:
    TinyGsm *modem;
    gpio_num_t rst, pwkey, dtr;
public:
    ModemIO(bool debug=false);
    ModemIO();
    ModemIO(TinyGsm *modem, gpio_num_t rst, gpio_num_t pwkey, gpio_num_t dtr);
    
    void reset();
    void on();
    void off();
    void sleep();
    void wake();
    void shutdown();
    String info();

    ~ModemIO();
};

ModemIO::ModemIO(TinyGsm *modem, gpio_num_t rst, gpio_num_t pwkey, gpio_num_t dtr)
{
    this->modem = modem;
    this->rst = rst;
    this->pwkey = pwkey;
    this->dtr = dtr;

    gpio_set_direction(this->dtr, GPIO_MODE_OUTPUT);
    gpio_set_direction(this->pwkey, GPIO_MODE_OUTPUT);
    gpio_set_direction(this->rst, GPIO_MODE_OUTPUT);
}

ModemIO::~ModemIO()
{
}

void ModemIO::reset()
{
    #ifdef DEBUG
    Serial.println("Modem hardware reset");
    #endif //DEBUG
    gpio_set_level(this->rst, 0);
    delay(260); //Treset 252ms
    gpio_set_level(this->rst, 0);
    delay(4000); //Modem takes longer to get ready and reply after this kind of reset vs power on

    //modem.factoryDefault();
    //modem.restart(); //this results in +CGREG: 0,0
}


void ModemIO::on()
{
    // Set-up modem  power pin
    gpio_set_level(this->pwkey, 1);
    delay(10);
    gpio_set_level(this->pwkey, 0);
    delay(1010); //Ton 1sec
    gpio_set_level(this->pwkey, 1);

    //wait_till_ready();
    Serial.println("Waiting till modem ready...");
    delay(4510); //Ton uart 4.5sec but seems to need ~7sec after hard (button) reset
                //On soft-reset serial replies immediately.
}


void ModemIO::off()
{
    //if you turn modem off while activating the fancy sleep modes it takes ~20sec, else its immediate
    Serial.println("Going to sleep now with modem turned off");
    //modem.gprsDisconnect();
    //modem.radioOff();
    this->modem->sleepEnable(false); // required in case sleep was activated and will apply after reboot
    this->modem->poweroff();
}


// fancy low power mode - while connected
void ModemIO::sleep() // will have an effect after reboot and will replace normal power down
{
    Serial.println("Going to sleep now with modem in power save mode");
    // needs reboot to activa and takes ~20sec to sleep
    this->modem->PSM_mode();    //if network supports will enter a low power sleep PCM (9uA)
    this->modem->eDRX_mode14(); // https://github.com/botletics/SIM7000-LTE-Shield/wiki/Current-Consumption#e-drx-mode
    this->modem->sleepEnable(); //will sleep (1.7mA), needs DTR or PWRKEY to wake
    gpio_set_level(this->dtr, 1);
}


void ModemIO::wake()
{
    Serial.println("Wake up modem from sleep");
    // DTR low to wake serial
    gpio_set_level(this->dtr, 0);
    delay(50);
    //wait_till_ready();
}


String ModemIO::info()
{
    return this->modem->getModemInfo();
}