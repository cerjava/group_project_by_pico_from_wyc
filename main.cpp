#include "pico/stdlib.h"
#include "dataLib.h"
#include "lib\pico_nrf24\lib\nrf24l01\nrf24_driver.h"
 

int main() {
    gpio_init(ON_BOARD_LED);
    gpio_set_dir(ON_BOARD_LED, GPIO_OUT);
    gpio_put(ON_BOARD_LED, 1);
}