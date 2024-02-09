#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace esp32_sdmmc {

class ESP32SDMMC;

//class ESP32SDMMC : public PollingComponent, public sensor::Sensor  {
class ESP32SDMMC : public PollingComponent {
 public:
  void set_numFiles(sensor::Sensor *numFiles) { numFiles_ = numFiles; }
  void set_diskUsed(sensor::Sensor *diskUsed) { diskUsed_ = diskUsed; }
  void set_DiskRemaining(sensor::Sensor *diskRemaining) { DiskRemaining_ = diskRemaining; }

  ESP32SDMMC();
  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override;
  void get_sd_lock(const char* clientTag);
  void return_sd_lock(const char* clientTag);
 protected:
  uint16_t countFiles(const char * dirname, uint8_t levels);
  Mutex sd_lock_;

  sensor::Sensor *numFiles_{nullptr};
  sensor::Sensor *diskUsed_{nullptr};
  sensor::Sensor *DiskRemaining_{nullptr};
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
extern ESP32SDMMC *global_ESP32SDMMC;

} //namespace esp32_sdmmc
} //namespace esphome