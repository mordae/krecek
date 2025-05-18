#include <pico/stdlib.h>

#include <dap.h>

uint32_t remote_peek(uint32_t addr)
{
	uint32_t out;

	if (!dap_peek(addr, &out))
		panic("dap_peek(0x%08x) failed\n", (unsigned)addr);

	return out;
}

void remote_poke(uint32_t addr, uint32_t value)
{
	if (!dap_poke(addr, value))
		panic("dap_poke(0x%08x, 0x%08x) failed\n", (unsigned)addr, (unsigned)value);
}

void remote_poke_many(uint32_t addr, const uint32_t *data, int len)
{
	if (!dap_poke_many(addr, data, len))
		panic("dap_poke_many(0x%08x) failed\n", (unsigned)addr);
}
