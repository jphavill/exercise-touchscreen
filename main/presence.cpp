#include "presence.h"

#include "driver/gpio.h"

#include "config.h"

void presence_init() {
  gpio_config_t io_cfg = {};
  io_cfg.pin_bit_mask = (1ULL << PRESENCE_PIN);
  io_cfg.mode = GPIO_MODE_INPUT;
  io_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_cfg.pull_up_en = GPIO_PULLUP_DISABLE;
  io_cfg.intr_type = GPIO_INTR_DISABLE;
  gpio_config(&io_cfg);
}

bool presence_detected() {
  return gpio_get_level(static_cast<gpio_num_t>(PRESENCE_PIN)) == 1;
}
