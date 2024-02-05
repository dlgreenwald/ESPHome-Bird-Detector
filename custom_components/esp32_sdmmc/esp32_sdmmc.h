#pragma once

#include "esphome/core/component.h"

namespace esphome {
namespace esp32_sdmmc {

class ESP32SDMMC : public PollingComponent {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override;
};

} //namespace esp32_sdmmc
} //namespace esphome