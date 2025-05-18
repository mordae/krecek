#pragma once

/* Access the QSPI flash using Winbond protocol. */
#define PICO_BOOT_STAGE2_CHOOSE_W25Q080 1

/* Flash access clock divider. */
#if !defined(PICO_FLASH_SPI_CLKDIV)
#define PICO_FLASH_SPI_CLKDIV 2
#endif

/* Total flash size in bytes. */
#if !defined(PICO_FLASH_SIZE_BYTES)
#define PICO_FLASH_SIZE_BYTES (16 * 1024 * 1024)
#endif

/* System clock */
#define CLK_SYS_HZ SYS_CLK_HZ

/* For SDK I2S */
#define SDK_PIO pio0

/* For vendor/pico-task */
#define MAX_TASKS 4
#define TASK_STACK_SIZE 2048

/* For vendor/pico-tft */
#define TFT_SPI_DEV spi1
#define TFT_BAUDRATE (70 * 1000 * 1000)
#define TFT_SWAP_XY 1
#define TFT_FLIP_X 0
#define TFT_FLIP_Y 1
#define TFT_SCALE 2
#define TFT_DRIVER TFT_DRIVER_ST7789V
#define TFT_RST_DELAY 120
#define TFT_HW_ACCEL 1
#define TFT_VSYNC 0
#define TFT_PIO SDK_PIO

/* Nuvoton DSP */
#define DSP_I2C i2c0

/* Slave control */
#define DAP_CORE0 0x01002927u
#define DAP_CORE1 0x11002927u
#define DAP_RESCUE 0xf1002927u

/* Card options. */
#define SD_SPI_DEV spi0
#define SD_BAUDRATE 26500000

/* Master pinout */
#define SD_MISO_PIN 0
#define IR_TX_PIN 1
#define SD_SCK_PIN 2
#define SD_MOSI_PIN 3
#define CHG_ISET2_PIN 4
#define SD_CS_PIN 5
#define TFT_RST_PIN 6
#define TFT_RS_PIN 7
#define TFT_MISO_PIN 8
#define TFT_CS_PIN 9
#define TFT_SCK_PIN 10
#define TFT_MOSI_PIN 11
#define DSP_CDATA_PIN 12
#define DSP_CCLK_PIN 13
#define DSP_DIN_PIN 14
#define DSP_DOUT_PIN 15
#define DSP_LRCK_PIN 16
#define DSP_SCLK_PIN 17
#define BUS0_PIN 18
#define BUS1_PIN 19
#define BUS2_PIN 20
#define BUS3_PIN 21
#define UNUSED_GPIO22_PIN 22 // Unused
#define CLK_OUT_PIN 23
#define DAP_SWCLK_PIN 24
#define DAP_SWDIO_PIN 25
#define USB_CC1_PIN 26
#define USB_CC2_PIN 27
#define HP_SENSE_PIN 28
#define BAT_VSENSE_PIN 29

/* Slave SPI shared between Touch and RF */
#define SLAVE_RF_SPI spi0
#define SLAVE_RF_SPI_BAUDRATE 6000000

#define SLAVE_TOUCH_SPI spi0
#define SLAVE_TOUCH_SPI_BAUDRATE 4000000

/* Slave pinout */
#define SLAVE_TOUCH_MISO_PIN 0
#define SLAVE_TOUCH_CS_PIN 1
#define SLAVE_TOUCH_SCK_PIN 2
#define SLAVE_TOUCH_MOSI_PIN 3
#define SLAVE_UNUSED_GPIO4_PIN 4 // Unused
#define SLAVE_RF_CS_PIN 5
#define SLAVE_BUS3_PIN 6
#define SLAVE_BUS2_PIN 7
#define SLAVE_BUS1_PIN 8
#define SLAVE_BUS0_PIN 9
#define SLAVE_AUX0_PIN 10
#define SLAVE_AUX1_PIN 11
#define SLAVE_AUX2_PIN 12
#define SLAVE_AUX3_PIN 13
#define SLAVE_AUX4_PIN 14
#define SLAVE_AUX5_PIN 15
#define SLAVE_AUX6_PIN 16
#define SLAVE_AUX7_PIN 17
#define SLAVE_START_PIN 18
#define SLAVE_OFF_PIN 19
#define SLAVE_UNUSED_GPIO20_PIN 20 // Unused
#define SLAVE_RF_XOSC_PIN 21
#define SLAVE_Y_PIN 22
#define SLAVE_B_PIN 23
#define SLAVE_A_PIN 24
#define SLAVE_X_PIN 25
#define SLAVE_BRACK_L_PIN 26
#define SLAVE_BRACK_R_PIN 27
#define SLAVE_JOY_X_PIN 28
#define SLAVE_JOY_Y_PIN 29

#define SLAVE_VOL_SW_QSPI_PIN 0	    // SCLK
#define SLAVE_UNUSED_QSPI_SS_PIN 1  // Unused SS
#define SLAVE_UNUSED_QSPI_SD0_PIN 2 // Unused SD0
#define SLAVE_SELECT_QSPI_PIN 3	    // SD1
#define SLAVE_VOL_DOWN_QSPI_PIN 4   // SD2
#define SLAVE_VOL_UP_QSPI_PIN 5	    // SD3
