#include "esphome/core/log.h"
#include "esp32_sdmmc.h"
#include "SD_MMC.h"
#include "FS.h"

namespace esphome {
namespace esp32_sdmmc {

static const char *TAG = "esp32_sdmmc";


ESP32SDMMC *global_ESP32SDMMC; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

ESP32SDMMC::ESP32SDMMC() {
    global_ESP32SDMMC = this;
}

void ESP32SDMMC::setup() {
    global_ESP32SDMMC = this;
    if (!SD_MMC.begin()) {
      Serial.println("SD Card Mount Failed");
    }

    uint8_t cardType = SD_MMC.cardType();
    if (cardType == CARD_NONE) {
      Serial.println("No SD Card attached");
    }
}

float ESP32SDMMC::get_setup_priority() const { return setup_priority::DATA; }


void ESP32SDMMC::update() {

}

void ESP32SDMMC::dump_config() {
    ESP_LOGCONFIG(TAG, "SD MMC");
}

void  ESP32SDMMC::get_sd_lock(){
    this->sd_lock_.lock();
}

void ESP32SDMMC::return_sd_lock(){
    this->sd_lock_.unlock();
}

} //namespace esp32_sdmmc
} //namespace esphome