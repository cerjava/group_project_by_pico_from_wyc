#include "RCCar.h"

int main() {

    stdio_init_all();

    gpio_init(ON_BOARD_LED);
    gpio_set_dir(ON_BOARD_LED, GPIO_OUT);
    gpio_put(ON_BOARD_LED, 1);

    initMotorDriver();

    setPowerMotor(255, true);

    gpio_init(Buzzer);
    gpio_set_dir(Buzzer, GPIO_OUT);
    gpio_put(Buzzer, 1);

    sleep_ms(500);

    //gpio_put(Buzzer, 0);

};