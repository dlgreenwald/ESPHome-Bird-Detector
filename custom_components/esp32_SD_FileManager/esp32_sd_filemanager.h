#pragma once

#include "esphome/core/component.h"
#include <ESPFMfGK.h>

namespace esphome {
namespace esp32_sd_filemanager {

class ESP32SDFM : public Component {
 public:
  ESP32SDFM();
  ~ESP32SDFM();

  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override;
  void loop() override;

 protected:
};

} //namespace esp32_sd_filemanager
} //namespace esphome