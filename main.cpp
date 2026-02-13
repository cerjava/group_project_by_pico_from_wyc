#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <tusb.h>

// 引脚定义（与您硬件接线一致）
#define PIN_SCK  2
#define PIN_MOSI 3
#define PIN_MISO 4
#define PIN_CSN  6
#define PIN_CE   5

// ---------- 底层 SPI 读写 ----------
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
// 读寄存器
static uint8_t nrf_read_reg(uint8_t reg) {
    uint8_t cmd = reg & 0x1F;
    cs_select();
    spi_xfer(cmd);
    uint8_t val = spi_xfer(0xFF);
    cs_deselect();
    return val;
}
// 写寄存器
static void nrf_write_reg(uint8_t reg, uint8_t val) {
    uint8_t cmd = 0x20 | (reg & 0x1F);
    cs_select();
    spi_xfer(cmd);
    spi_xfer(val);
    cs_deselect();
}
// 读 RX 负载
static void nrf_read_rx_payload(uint8_t *buf, uint8_t len) {
    uint8_t cmd = 0x61;  // R_RX_PAYLOAD
    cs_select();
    spi_xfer(cmd);
    for (int i = 0; i < len; i++) {
        buf[i] = spi_xfer(0xFF);
    }
    cs_deselect();
}
// 写 TX 负载（仅用于 ACK，本测试未用）
static void nrf_write_tx_payload(uint8_t *buf, uint8_t len) {
    uint8_t cmd = 0xA0;  // W_TX_PAYLOAD
    cs_select();
    spi_xfer(cmd);
    for (int i = 0; i < len; i++) {
        spi_xfer(buf[i]);
    }
    cs_deselect();
}
// 清除中断标志
static void nrf_clear_irq() {
    nrf_write_reg(0x07, 0x70);  // 写 STATUS，清除 RX_DR, TX_DS, MAX_RT
}
// -----------------------------------------

int main() {
    stdio_init_all();
    while (!tud_cdc_connected()) sleep_ms(10);
    printf("\n========= NRF24L01+ 接收端（自主寄存器控制）=========\n");

    // 1. 初始化 SPI
    spi_init(spi0, 5000000);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);

    // 2. 初始化 CSN, CE
    gpio_init(PIN_CSN); gpio_set_dir(PIN_CSN, GPIO_OUT); cs_deselect();
    gpio_init(PIN_CE);  gpio_set_dir(PIN_CE,  GPIO_OUT); gpio_put(PIN_CE, 0);
    sleep_ms(50);

    // 3. 配置寄存器（关键步骤）
    // 3.1 使能 CRC, 16位, 上电
    nrf_write_reg(0x00, 0x0C);  // CONFIG = 0b00001100 (PRIM_RX=0 先)
    // 3.2 设置地址宽度 5 字节
    nrf_write_reg(0x03, 0x03);  // SETUP_AW = 3 (5字节)
    // 3.3 设置 RF 信道 120
    nrf_write_reg(0x05, 120);   // RF_CH = 120
    // 3.4 设置数据速率 1Mbps, 功率 0dBm
    nrf_write_reg(0x06, 0x06);  // RF_SETUP = 0b00000110 (1Mbps, 0dBm)
    // 3.5 设置自动重发延迟 500us, 重发 10次
    nrf_write_reg(0x04, 0x5A);  // SETUP_RETR = ARD=0101(500us), ARC=1010(10)
    // 3.6 使能动态负载长度（管道0）
    nrf_write_reg(0x1C, 0x01);  // DYNPD = 0x01 (仅管道0)
    // 3.7 使能管道0的自动应答和动态负载
    nrf_write_reg(0x1D, 0x01);  // FEATURE = 0x01 (EN_DPL)
    
    // 3.8 设置接收地址管道0
    uint8_t rx_addr0[] = {0x37, 0x37, 0x37, 0x37, 0x37};
    cs_select();
    spi_xfer(0x2A);  // 写 RX_ADDR_P0 命令
    for (int i = 0; i < 5; i++) spi_xfer(rx_addr0[i]);
    cs_deselect();

    // 3.9 设置发送地址（用于自动应答，设成相同地址即可）
    cs_select();
    spi_xfer(0x30);  // 写 TX_ADDR 命令
    for (int i = 0; i < 5; i++) spi_xfer(rx_addr0[i]);
    cs_deselect();

    // 3.10 使能管道0接收
    uint8_t en_rx = nrf_read_reg(0x02);
    en_rx |= 0x01;   // 使能管道0
    nrf_write_reg(0x02, en_rx);

    // 3.11 清除中断
    nrf_clear_irq();

    // 3.12 设置为接收模式
    uint8_t config = nrf_read_reg(0x00);
    config |= 0x01;  // PRIM_RX = 1
    nrf_write_reg(0x00, config);
    gpio_put(PIN_CE, 1);  // CE 拉高，进入接收模式
    sleep_ms(2);

    // 4. 打印当前寄存器状态，验证配置
    printf("CONFIG     = 0x%02X (PRIM_RX应=1)\n", nrf_read_reg(0x00));
    printf("EN_RXADDR  = 0x%02X (管道0应使能)\n", nrf_read_reg(0x02));
    printf("RF_CH      = 0x%02X (%d)\n", nrf_read_reg(0x05), nrf_read_reg(0x05));
    printf("RX_ADDR_P0 = ");
    cs_select();
    spi_xfer(0x0A);  // 读 RX_ADDR_P0
    for (int i = 0; i < 5; i++) printf("%02X ", spi_xfer(0xFF));
    cs_deselect();
    printf("\n");
    printf("监听地址: 37 37 37 37 37\n");
    printf("STATUS     = 0x%02X\n", nrf_read_reg(0x07));
    printf("FIFO_STATUS= 0x%02X\n", nrf_read_reg(0x17));
    printf("========================================\n");

    // 5. 接收循环
    uint8_t rx_buf[32];
    printf("接收端准备就绪，等待数据...\n");
    while (1) {
        uint8_t status = nrf_read_reg(0x07);
        if (status & 0x40) {  // RX_DR
            // 读取负载长度
            cs_select();
            spi_xfer(0x60);  // R_RX_PL_WID
            uint8_t len = spi_xfer(0xFF);
            cs_deselect();
            if (len > 32) len = 32;
            
            uint8_t buf[32];
            cs_select();
            spi_xfer(0x61);  // R_RX_PAYLOAD
            for (int i=0; i<len; i++) buf[i] = spi_xfer(0xFF);
            cs_deselect();
            
            // 清除 RX_DR 标志
            nrf_write_reg(0x07, 0x40);
            
            printf("📥 收到 %d 字节: ", len);
            for (int i=0; i<len; i++) printf("%02X ", buf[i]);
            if (len == 1) printf("(值=%d)", buf[0]);
            printf("\n");
        }
    sleep_ms(10);
    }
}