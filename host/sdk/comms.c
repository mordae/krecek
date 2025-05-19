#include <sdk/comms.h>
#include <stdlib.h>

bool sdk_send_ir(uint32_t word)
{
	(void)word;
	return true;
}

void sdk_decode_ir_raw(int16_t sample, int *oI, int *oQ, int *dm)
{
	(void)sample;
	(void)oI;
	(void)oQ;
	(void)dm;
}

void sdk_decode_ir(int16_t sample)
{
	return sdk_decode_ir_raw(sample, NULL, NULL, NULL);
}

bool sdk_send_rf(uint8_t addr, const uint8_t *data, int len)
{
	(void)addr;
	(void)data;
	(void)len;
	return true;
}
