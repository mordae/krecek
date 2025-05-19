#pragma once
#include <stdint.h>

/*
 * Schedule 32b to transmit over IR.
 *
 * There is a short stack of scheduled transmissions to support some
 * degree of buffering, but don't transmit too much anyway. Remember
 * that we can only send at most about 2000 bits per second.
 */
bool sdk_send_ir(uint32_t word);

#define SDK_RF_ALL 255
#define SDK_RF_MAX 60

/*
 * Send up to SDK_MAX_RF_SIZE=60 bytes of data over radio.
 * Address can be 0x00 or 0xff for broadcast.
 */
bool sdk_send_rf(uint8_t addr, const uint8_t *data, int len);

/*
 * Decode IR sample.
 *
 * Performed automatically by game_audio(), unless overridden.
 * If you do override it, you need to call this with IR samples,
 * i.e. left input audio channel.
 *
 * Once a sample is decoded, game_inbox() is called.
 */
void sdk_decode_ir(int16_t sample);

/*
 * Same as sdk_decode_ir(), but also gives you access to raw quadrature
 * and demodulated data for monitoring. All these are not touched when
 * not available at this point. You can set them to INT_MIN or INT_MAX to
 * safely detect this situation since output range is int16_t. You can
 * also pass in NULL if you are not interested in that value.
 */
void sdk_decode_ir_raw(int16_t sample, int *I, int *Q, int *dm);
