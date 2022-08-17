#include <esp32-hal-gpio.h>



#define I2C_SDA 21
#define I2C_SCL 22

/**
 * @brief used pins
 * 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 
 * 26, 27 - SIM TX/RX
 * 25 - modem DTR
 * 35, 36 - adc BAT, SOLAR
 */

struct used_pins_t{
    gpio_num_t pin1 = GPIO_NUM_1;
    gpio_num_t pin2 = GPIO_NUM_2;
    gpio_num_t pin12 = GPIO_NUM_12;
    gpio_num_t pin13 = GPIO_NUM_13;
    gpio_num_t pin14 = GPIO_NUM_14;
    gpio_num_t pin15 = GPIO_NUM_15;
    gpio_num_t pin25 = GPIO_NUM_25;
};

/**
 * @brief not connected pins
 * 16, 17, 20
 * 
 */
struct nc_pins_t{
    gpio_num_t pin16 = GPIO_NUM_16;
    gpio_num_t pin17 = GPIO_NUM_17;
    gpio_num_t pin20 = GPIO_NUM_20;
};

struct unused_pins_t
{
    gpio_num_t pin18 = GPIO_NUM_18;
    gpio_num_t pin19 = GPIO_NUM_19;
    gpio_num_t pin21 = GPIO_NUM_21;
    gpio_num_t pin22 = GPIO_NUM_22;
    gpio_num_t pin23 = GPIO_NUM_23;
    gpio_num_t pin32 = GPIO_NUM_32;
    gpio_num_t pin33 = GPIO_NUM_33;
    gpio_num_t pin34 = GPIO_NUM_34;
    

    gpio_num_t sda = pin21;
    gpio_num_t scl = pin22;
    gpio_num_t mosi = pin23;
    gpio_num_t miso = pin19;
    gpio_num_t sclk = pin18;

};

unused_pins_t unused_pins;