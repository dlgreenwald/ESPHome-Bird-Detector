#include "esp32_sd_filemanager.h"
#include "esphome/core/log.h"
#include "SD_MMC.h"
#include "FS.h"
#include <ESPFMfGK.h>

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
  //@TODO Wrap handle client in a MUTEX controlled by a new SD_MMC component
    filemgr.handleClient();
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