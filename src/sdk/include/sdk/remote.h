#include <pico/stdlib.h>

/* Remote single word from remote memory or panic. */
uint32_t remote_peek(uint32_t addr);

/* Write single word to remote memory or panic. */
void remote_poke(uint32_t addr, uint32_t value);

/* Write multiple words to remote memory or panic. */
void remote_poke_many(uint32_t addr, const uint32_t *data, int len);
