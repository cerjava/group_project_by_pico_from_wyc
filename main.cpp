#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

#include <tusb.h> // TinyUSB tud_cdc_connected()

#define SPI_PORT spi0
#define PIN_SCK  2
#define PIN_MOSI 3
#define PIN_MISO 4
#define PIN_CSN  6
#define PIN_CE   5

uint8_t spi_xfer(uint8_t tx) {
    uint8_t rx;
    spi_write_read_blocking(SPI_PORT, &tx, &rx, 1);
    return rx;
}

uint8_t nrf_read_reg(uint8_t reg) {
    uint8_t cmd = reg & 0x1F;
    gpio_put(PIN_CSN, 0);
    spi_xfer(cmd);
    uint8_t val = spi_xfer(0xFF);
    gpio_put(PIN_CSN, 1);
    return val;
}

int main() {
    stdio_init_all();
    while (!tud_cdc_connected()) sleep_ms(10);
    printf("开始检测 NRF24L01+...\n");

    spi_init(SPI_PORT, 500 * 1000);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);

    gpio_init(PIN_CSN); gpio_set_dir(PIN_CSN, GPIO_OUT); gpio_put(PIN_CSN, 1);
    gpio_init(PIN_CE);  gpio_set_dir(PIN_CE,  GPIO_OUT); gpio_put(PIN_CE,  0);

    sleep_ms(100);

    uint8_t config = nrf_read_reg(0x00);
    uint8_t status = nrf_read_reg(0x07);
    uint8_t rx_addr = nrf_read_reg(0x0A);
    uint8_t retr = nrf_read_reg(0x04);
    uint8_t rf_setup = nrf_read_reg(0x06);

    printf("CONFIG     = 0x%02X\n", config);
    printf("STATUS     = 0x%02X\n", status);
    printf("RX_ADDR_P0 = 0x%02X\n", rx_addr);
    printf("SETUP_RETR = 0x%02X\n", retr);
    printf("RF_SETUP   = 0x%02X\n", rf_setup);

    if (config == 0xFF || config == 0x00) {
        printf("\n❌ 模块无响应！\n");
        printf("   请检查：\n");
        printf("   - VCC 是否真的 3.3V（万用表测模块引脚）\n");
        printf("   - CSN(6) 和 CE(5) 是否接对，杜邦线是否插紧\n");
        printf("   - 模块是否损坏（换另一个模块试）\n");
    } else {
        printf("\n✅ 模块已响应！SPI 通信正常，可以继续开发。\n");
    }

    while (1) sleep_ms(1000);
}