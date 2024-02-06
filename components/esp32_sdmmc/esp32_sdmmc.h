#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace esp32_sdmmc {

class ESP32SDMMC;

class ESP32SDMMC : public PollingComponent {
 public:
  ESP32SDMMC();
  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override;
  void get_sd_lock();
  void return_sd_lock();
 protected:
  Mutex sd_lock_;
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
extern ESP32SDMMC *global_ESP32SDMMC;

} //namespace esp32_sdmmc
} //namespace esphome