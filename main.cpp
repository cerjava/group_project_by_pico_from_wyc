#include "RCCar.h"

// 无线RF同步信号
// 有啥用？
const uint RF_SYNC_CODE = 0b1000101011000011;

int main() {

    stdio_init_all();

    // 这个25你还没背出来吗？
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);
    gpio_put(25, 1);

    initMotorDriver();

    setPowerMotor(65535, true);


    bool left = true;
    while(true)
    {
        setSteeringMotor(65535, left);
        left = !left;
        sleep_ms(2000);
    }

    gpio_init(Buzzer);
    gpio_set_dir(Buzzer, GPIO_OUT);
    gpio_put(Buzzer, 1);

    sleep_ms(500);

    //gpio_put(Buzzer, 0);

};