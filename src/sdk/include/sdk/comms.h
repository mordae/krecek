#pragma once
#include <stdint.h>

/*
 * Schedule 32b to transmit over IR.
 * If another transmission pending, return false.
 */
bool sdk_send_ir(uint32_t word);
