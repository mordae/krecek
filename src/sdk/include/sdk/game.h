#pragma once
#include <pico/stdlib.h>

void game_start(void);
void game_reset(void);
void game_audio(int nsamples);
void game_input(unsigned dt);
void game_paint(unsigned dt);
