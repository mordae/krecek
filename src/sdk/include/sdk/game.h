#pragma once
#include <pico/stdlib.h>

void game_start(void);
void game_reset(void);
void game_audio(int nsamples);
void game_input(unsigned dt);
void game_paint(unsigned dt);

/* Incoming communication message. */
typedef struct sdk_message {
	enum sdk_message_type {
		SDK_MSG_IR,
		SDK_MSG_RF,
	} type;

	union {
		struct {
			uint32_t data;
		} ir;
		struct {
			uint8_t *body;
			int length;
		} rf;
	};
} sdk_message_t;

void game_inbox(const sdk_message_t *msg);
