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
