#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <tusb.h>

#define PIN_SCK  2
#define PIN_MOSI 3
#define PIN_MISO 4
#define PIN_CSN  6
#define PIN_CE   5

// ---------- 底层SPI函数（必须完整）----------
static inline void cs_select() {
    gpio_put(PIN_CSN, 0);
}
static inline void cs_deselect() {
    gpio_put(PIN_CSN, 1);
}
static uint8_t spi_xfer(uint8_t tx) {
    uint8_t rx;
    spi_write_read_blocking(spi0, &tx, &rx, 1);
    return rx;
}
static void nrf_write_reg(uint8_t reg, uint8_t val) {
    uint8_t cmd = 0x20 | (reg & 0x1F);
    cs_select();
    spi_xfer(cmd);
    spi_xfer(val);
    cs_deselect();
}
static uint8_t nrf_read_reg(uint8_t reg) {
    uint8_t cmd = reg & 0x1F;
    cs_select();
    spi_xfer(cmd);
    uint8_t val = spi_xfer(0xFF);
    cs_deselect();
    return val;
}
static void nrf_clear_irq() {
    nrf_write_reg(0x07, 0x70);  // 清除 RX_DR, TX_DS, MAX_RT
}
static void nrf_write_tx_payload(uint8_t *buf, uint8_t len) {
    uint8_t cmd = 0xA0;
    cs_select();
    spi_xfer(cmd);
    for (int i = 0; i < len; i++) spi_xfer(buf[i]);
    cs_deselect();
}
// ---------------------------------------------

int main() {
    stdio_init_all();
    while (!tud_cdc_connected()) sleep_ms(10);
    printf("\n========= NRF24L01+ 发射端（增强调试）=========\n");

    // SPI 初始化
    spi_init(spi0, 5000000);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);

    gpio_init(PIN_CSN); gpio_set_dir(PIN_CSN, GPIO_OUT); cs_deselect();
    gpio_init(PIN_CE);  gpio_set_dir(PIN_CE,  GPIO_OUT); gpio_put(PIN_CE, 0);
    sleep_ms(50);

    // 配置寄存器（与接收端完全一致）
    nrf_write_reg(0x00, 0x0C);  // CONFIG: PWR_UP=1, PRIM_RX=0
    nrf_write_reg(0x03, 0x03);  // 5字节地址
    nrf_write_reg(0x05, 120);   // 信道120
    nrf_write_reg(0x06, 0x06);  // 1Mbps, 0dBm
    nrf_write_reg(0x04, 0x5A);  // 重发延迟500us, 10次
    nrf_write_reg(0x1C, 0x01);  // DYNPD 管道0
    nrf_write_reg(0x1D, 0x01);  // FEATURE: EN_DPL

    // 设置发送地址
    uint8_t tx_addr[] = {0x37,0x37,0x37,0x37,0x37};
    cs_select();
    spi_xfer(0x30);  // 写 TX_ADDR
    for (int i=0;i<5;i++) spi_xfer(tx_addr[i]);
    cs_deselect();

    // 设置接收地址（用于ACK）
    cs_select();
    spi_xfer(0x2A);  // 写 RX_ADDR_P0
    for (int i=0;i<5;i++) spi_xfer(tx_addr[i]);
    cs_deselect();

    // 使能管道0
    uint8_t en_rx = nrf_read_reg(0x02);
    en_rx |= 0x01;
    nrf_write_reg(0x02, en_rx);

    nrf_clear_irq();

    // CE 拉高，进入待机模式
    gpio_put(PIN_CE, 1);
    sleep_ms(2);

    // 打印当前配置
    printf("CONFIG     = 0x%02X\n", nrf_read_reg(0x00));
    printf("EN_RXADDR  = 0x%02X\n", nrf_read_reg(0x02));
    printf("RF_CH      = 0x%02X (%d)\n", nrf_read_reg(0x05), nrf_read_reg(0x05));
    printf("TX_ADDR    = ");
    cs_select();
    spi_xfer(0x10);  // 读 TX_ADDR
    for (int i=0;i<5;i++) printf("%02X ", spi_xfer(0xFF));
    cs_deselect();
    printf("\n");
    printf("目标地址: 37 37 37 37 37 (应与上面一致)\n");

    uint8_t counter = 0;
    while (1) {
        printf("\n--- 发送尝试 #%d ---\n", counter);

        // 写负载
        nrf_write_tx_payload(&counter, 1);
        printf("已写入负载: %d\n", counter);

        // 脉冲 CE 启动发送
        gpio_put(PIN_CE, 0);
        sleep_us(10);
        gpio_put(PIN_CE, 1);
        printf("CE 脉冲完成\n");

        // 等待发送结果
        uint32_t timeout = 0;
        while (timeout < 50000) {
            uint8_t status = nrf_read_reg(0x07);
            if (status & 0x20) {
                printf("✅ 发送成功！收到ACK, STATUS=0x%02X\n", status);
                nrf_write_reg(0x07, 0x20);  // 清除 TX_DS
                break;
            }
            if (status & 0x10) {
                printf("❌ 发送失败（重发超时）, STATUS=0x%02X\n", status);
                nrf_write_reg(0x07, 0x10);  // 清除 MAX_RT
                break;
            }
            timeout++;
            sleep_us(10);
        }
        if (timeout >= 50000) {
            printf("⚠️ 发送超时（无任何中断）\n");
        }

        // 打印当前 FIFO 状态
        printf("FIFO_STATUS = 0x%02X\n", nrf_read_reg(0x17));
        counter++;
        sleep_ms(2000);
    }
}