#pragma once

#define CLK_SYS_HZ 132000000

/* For vendor/pico-task */
#define MAX_TASKS 4
#define TASK_STACK_SIZE 2048

/* For vendor/pico-tft */
#define TFT_CS_PIN 1
#define TFT_SCK_PIN 2
#define TFT_MOSI_PIN 3
#define TFT_RST_PIN 6
#define TFT_RS_PIN 4
#define TFT_SPI_DEV spi0
#define TFT_BAUDRATE 80000000
#define TFT_SWAP_XY 1
#define TFT_FLIP_X 1
#define TFT_FLIP_Y 1
#define TFT_SCALE 2

#define DAP_SWDIO_PIN 25
#define DAP_SWCLK_PIN 24

#define DAP_CORE0 0x01002927u
#define DAP_CORE1 0x11002927u
#define DAP_RESCUE 0xf1002927u

/* For the secondary chip */
#define CLK_OUT_PIN 23

/* Slave pinout */
#define SLAVE_TFT_LED_PIN 13

#define SLAVE_A_PIN 22
#define SLAVE_B_PIN 24
#define SLAVE_X_PIN 23
#define SLAVE_Y_PIN 25

#define SLAVE_START_PIN 19
#define SLAVE_SELECT_PIN 20

#define SLAVE_BRACK_R_PIN 26
#define SLAVE_BRACK_L_PIN 27
#define SLAVE_JOY_X_PIN 28
#define SLAVE_JOY_Y_PIN 29

#define SLAVE_AUX0_PIN 9
#define SLAVE_AUX1_PIN 10
#define SLAVE_AUX2_PIN 11
#define SLAVE_AUX3_PIN 12

#define SLAVE_AUX4_QSPI_PIN 2
#define SLAVE_AUX5_QSPI_PIN 3
#define SLAVE_AUX6_QSPI_PIN 4
#define SLAVE_AUX7_QSPI_PIN 5

#define SLAVE_OFF_QSPI_PIN 0
