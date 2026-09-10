#pragma once
#include <sdk.h>

/* Intro scene for the allbuild boot sequence and its own menu section */
extern sdk_scene_t scene_intro;

/* Configure whether the intro is running as the initial power-on boot sequence */
void intro_set_boot_mode(bool boot_mode);
bool intro_is_boot_mode(void);

/* Trigger replaying the boot animation with sound chime */
void intro_start_animation(void);
