#include "RCCar.h"


int main() {

    stdio_init_all();
    

    gpio_init(ON_BOARD_LED);
    gpio_set_dir(ON_BOARD_LED, GPIO_OUT);
    gpio_put(ON_BOARD_LED, 1);
};