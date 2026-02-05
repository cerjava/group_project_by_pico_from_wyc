#include "RCCar.h"

// 引脚常量定义（请根据实际接线在此处修改引脚号）
const uint8_t Buzzer = 0;
const uint8_t PowerMotor_1 = 1; // 对应DRV8833 AIN1
const uint8_t PowerMotor_2 = 2; // 对应DRV8833 AIN2
const uint8_t SteeringMotor_1 = 3; // 对应DRV8833 BIN1
const uint8_t SteeringMotor_2 = 4; // 对应DRV8833 BIN2

// PWM参数定义
const uint32_t PWM_FREQ = 20000; // 20kHz，听不见噪音且开关损耗适中
const uint16_t PWM_WRAP = 255;   // 8位分辨率，与speed参数范围匹配

void initMotorDriver() {
    // 将四个控制引脚设置为PWM功能
    gpio_set_function(PowerMotor_1, GPIO_FUNC_PWM);
    gpio_set_function(PowerMotor_2, GPIO_FUNC_PWM);
    gpio_set_function(SteeringMotor_1, GPIO_FUNC_PWM);
    gpio_set_function(SteeringMotor_2, GPIO_FUNC_PWM);

    // 获取每个引脚对应的PWM切片号（用于独立配置）
    uint slice_num_p1 = pwm_gpio_to_slice_num(PowerMotor_1);
    uint slice_num_p2 = pwm_gpio_to_slice_num(PowerMotor_2);
    uint slice_num_s1 = pwm_gpio_to_slice_num(SteeringMotor_1);
    uint slice_num_s2 = pwm_gpio_to_slice_num(SteeringMotor_2);

    // 配置PWM：统一频率和分辨率
    pwm_config config = pwm_get_default_config();
    pwm_config_set_wrap(&config, PWM_WRAP);
    
    // 初始化四个PWM切片
    pwm_init(slice_num_p1, &config, true);
    pwm_init(slice_num_p2, &config, true);
    pwm_init(slice_num_s1, &config, true);
    pwm_init(slice_num_s2, &config, true);
}

void setPowerMotor(uint8_t speed, bool ahead) {
    if (speed == 0) {
        // 停止模式：滑行停止 (AIN1=0, AIN2=0)
        pwm_set_gpio_level(PowerMotor_1, 0);
        pwm_set_gpio_level(PowerMotor_2, 0);
        return;
    }
    
    if (ahead) {
        // 前进模式：AIN1=PWM, AIN2=0
        pwm_set_gpio_level(PowerMotor_1, speed);
        pwm_set_gpio_level(PowerMotor_2, 0);
    } else {
        // 后退模式：AIN1=0, AIN2=PWM
        pwm_set_gpio_level(PowerMotor_1, 0);
        pwm_set_gpio_level(PowerMotor_2, speed);
    }
}

void setSteeringMotor(uint8_t speed, bool left) {
    if (speed == 0) {
        // 停止转向
        pwm_set_gpio_level(SteeringMotor_1, 0);
        pwm_set_gpio_level(SteeringMotor_2, 0);
        return;
    }
    
    if (left) {
        // 左转：BIN1=PWM, BIN2=0
        pwm_set_gpio_level(SteeringMotor_1, speed);
        pwm_set_gpio_level(SteeringMotor_2, 0);
    } else {
        // 右转：BIN1=0, BIN2=PWM
        pwm_set_gpio_level(SteeringMotor_1, 0);
        pwm_set_gpio_level(SteeringMotor_2, speed);
    }
}

void brakePowerMotor() {
    // 将动力电机的两个控制引脚均设为最高占空比（高电平）
    // 对应DRV8833逻辑：AIN1=1, AIN2=1 -> 刹车模式
    pwm_set_gpio_level(PowerMotor_1, PWM_WRAP);
    pwm_set_gpio_level(PowerMotor_2, PWM_WRAP);
}

void centerSteeringMotor() {
    // 将转向电机的两个控制引脚均设为最高占空比（高电平）
    // 对应DRV8833逻辑：BIN1=1, BIN2=1 -> 刹车模式
    pwm_set_gpio_level(SteeringMotor_1, PWM_WRAP);
    pwm_set_gpio_level(SteeringMotor_2, PWM_WRAP);
}