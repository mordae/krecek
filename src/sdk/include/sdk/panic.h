#pragma once
#include <pico/stdlib.h>

/* Our panic that flushes stdout. */
void __attribute__((__noreturn__, __format__(printf, 1, 2))) sdk_panic(const char *fmt, ...);
