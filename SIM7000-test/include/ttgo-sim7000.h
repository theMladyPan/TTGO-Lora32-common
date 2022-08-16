#include <esp32-hal-gpio.h>


/**
 * @brief used pins
 * 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 
 * 26, 27 - SIM TX/RX
 * 
 */

struct used_pins_t{
    gpio_num_t pin2 = GPIO_NUM_2;
    gpio_num_t pin12 = GPIO_NUM_12;
    gpio_num_t pin13 = GPIO_NUM_13;
    gpio_num_t pin14 = GPIO_NUM_14;
    gpio_num_t pin15 = GPIO_NUM_15;
}

/**
 * @brief not connected pins
 * 16, 17, 20
 * 
 */
struct nc_pins_t{
    gpio_num_t pin16 = GPIO_NUM_16;
    gpio_num_t pin17 = GPIO_NUM_17;
    gpio_num_t pin20 = GPIO_NUM_20;
}

struct unused_pins_t
{
    gpio_num_t pin18 = GPIO_NUM_18;
    gpio_num_t pin19 = GPIO_NUM_19;
}