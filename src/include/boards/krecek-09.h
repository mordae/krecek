#pragma once

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
