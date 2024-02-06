#include "esp32_sd_filemanager.h"
#include "esphome/core/log.h"
#include "SD_MMC.h"
#include "FS.h"
#include <ESPFMfGK.h>
#include "esphome/components/esp32_sdmmc/esp32_sdmmc.h"

namespace esphome {
namespace esp32_sd_filemanager {

static const char *TAG = "esp32_sd_filemanager";

const word filemanagerport = 8080;
ESPFMfGK filemgr(filemanagerport);

ESP32SDFM::ESP32SDFM() {}

ESP32SDFM::~ESP32SDFM() {}

void ESP32SDFM::setup() {
  /**/
  if (SD_MMC.begin("/sdcard", true)) { //@TODO This line of code needs to go in it's own component.  Open question, does getting a handle to the SD_MMC need to go in the component too.
    if (!filemgr.AddFS(SD_MMC, "SD-MMC-Card", false)) {
      Serial.println(F("Adding SD_MMC failed."));
    }
  } else {
    Serial.println(F("SD_MMC File System not inited."));
  }
  /**/

  filemgr.WebPageTitle = "FileManager";
  filemgr.BackgroundColor = "white";
  filemgr.textareaCharset = "accept-charset=\"utf-8\"";

  filemgr.begin();

}

float ESP32SDFM::get_setup_priority() const { return setup_priority::WIFI; }


void ESP32SDFM::loop() {
  esphome::esp32_sdmmc::global_ESP32SDMMC->get_sd_lock();
  filemgr.handleClient();
  esphome::esp32_sdmmc::global_ESP32SDMMC->return_sd_lock();
}

void ESP32SDFM::dump_config() {
    ESP_LOGCONFIG(TAG, "SD File Manager");
    ESP_LOGCONFIG(TAG, "  Port: %d", this->port_);
    if (this->is_failed()) {
        ESP_LOGE(TAG, "  Setup Failed");
    }
}

} //namespace esp32_sd_filemanager
} //namespace esphome