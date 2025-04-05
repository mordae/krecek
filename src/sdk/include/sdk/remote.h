#include <pico/stdlib.h>

/* Remote remote register or panic. */
uint32_t remote_peek(uint32_t addr);

/* Write remote register or panic. */
void remote_poke(uint32_t addr, uint32_t value);

/* Read remote gpio state. */
int remote_gpio_get(int gpio);

/* Read remote gpio state. */
void remote_gpio_set(int gpio, bool value);

/* Put remote gpio pin into high-Z mode. */
void remote_gpio_clear(int gpio);

/* Read remote qspi gpio state. */
int remote_qspi_get(int gpio);

/* Read remote qspi gpio state. */
void remote_qspi_set(int gpio, bool value);

/* Put remote qspi gpio pin into high-Z mode. */
void remote_qspi_clear(int gpio);

/* Sample and read remote ADC gpio. */
int remote_adc_read(int gpio);

/* Configure remote bank0 gpio pad. */
void remote_gpio_pad_set(bool od, bool ie, int drive, bool pue, bool pde, bool schmitt,
			 bool slewfast, int pad);

/* Configure remote qspi gpio pad. */
void remote_qspi_pad_set(bool od, bool ie, int drive, bool pue, bool pde, bool schmitt,
			 bool slewfast, int pad);
