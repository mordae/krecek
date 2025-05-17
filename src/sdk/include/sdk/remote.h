#include <pico/stdlib.h>

/* Remote remote register or panic. */
uint32_t remote_peek(uint32_t addr);

/* Write remote register or panic. */
void remote_poke(uint32_t addr, uint32_t value);
