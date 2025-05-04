#pragma once

/*
 * Reboot the console into firmware in given slot.
 * We have 16 slots numbered from 0 to 15.
 * Each has 1 MiB of flash space.
 */
void sdk_reboot_into_slot(unsigned slot);
