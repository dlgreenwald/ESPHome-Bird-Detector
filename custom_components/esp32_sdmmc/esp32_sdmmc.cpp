#include "esphome/core/log.h"
#include "esp32_sdmmc.h"
#include "SD_MMC.h"
#include "FS.h"

namespace esphome {
namespace esp32_sdmmc {

static const char *TAG = "esp32_sdmmc";

void ESP32SDMMC::setup() {
    
}

float ESP32SDMMC::get_setup_priority() const { return setup_priority::DATA; }


void ESP32SDMMC::update() {

}

void ESP32SDMMC::dump_config() {
    ESP_LOGCONFIG(TAG, "SD MMC");
}

} //namespace esp32_sdmmc
} //namespace esphome