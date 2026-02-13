#include <stdio.h>
#include "pico/stdlib.h"
#include "RCCar.h"
#include <tusb.h>

int main(void) {
    stdio_init_all();
    while (!tud_cdc_connected()) sleep_ms(10);
    printf("NRF24L01+ 发射端启动 (驱动库: nrf24_driver)\n");

    // 1. 引脚配置（完全按照您验证通过的接线）
    pin_manager_t my_pins = {
        .copi = 3,   // MOSI = GPIO3
        .cipo = 4,   // MISO = GPIO4
        .sck  = 2,   // SCK  = GPIO2
        .csn  = 6,   // CSN  = GPIO6
        .ce   = 5    // CE   = GPIO5
    };

    // 2. 射频参数配置（与接收端完全一致）
    nrf_manager_t my_config = {
        .address_width = AW_5_BYTES,
        .dyn_payloads  = DYNPD_ENABLE,
        .retr_delay    = ARD_500US,
        .retr_count    = ARC_10RT,
        .data_rate     = RF_DR_1MBPS,
        .power         = RF_PWR_NEG_12DBM,
        .channel       = 120
    };

    uint32_t baudrate = 5000000; // 5MHz SPI
    nrf_client_t nrf;

    // 3. 初始化驱动
    nrf_driver_create_client(&nrf);
    nrf.configure(&my_pins, baudrate);
    nrf.initialise(&my_config);
    nrf.standby_mode();  // 进入待机模式（准备发送）

    // 4. 设置目标地址（接收端监听管道0的地址）
    uint8_t tx_addr[] = {0x37, 0x37, 0x37, 0x37, 0x37};
    nrf.tx_destination(tx_addr);

    uint8_t counter = 0;
    while (1) {
        printf("发送: %d\n", counter);
        fn_status_t success = nrf.send_packet(&counter, sizeof(counter));
        if (success) {
            printf("✅ 发送成功 (收到ACK)\n");
        } else {
            printf("❌ 发送失败 (无ACK)\n");
        }
        counter++;
        sleep_ms(2000);
    }
}