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
      ESP_LOGE(TAG, "SD Card Mount Failed");
    }

    uint8_t cardType = SD_MMC.cardType();
    if (cardType == CARD_NONE) {
      ESP_LOGE(TAG, "No SD Card attached");
    }

    ESP_LOGI(TAG, "SD Card Successfully Initialized");
}

void ESP32SDMMC::listDir(const char * dirname, uint8_t levels){
    ESP_LOGI(TAG, "Listing directory: %s\n", dirname);

    File root = SD_MMC.open(dirname);
    if(!root){
        ESP_LOGI(TAG, "Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        ESP_LOGI(TAG, "Not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            ESP_LOGI(TAG, "  DIR : %s", file.name());
            if(levels){
                listDir(file.name(), levels -1);
            }
        } else {
            ESP_LOGI(TAG, "  FILE: %s", file.name());
            ESP_LOGI(TAG, "  SIZE: %i", file.size());
        }
        file = root.openNextFile();
    }
}

float ESP32SDMMC::get_setup_priority() const { return setup_priority::DATA; }


void ESP32SDMMC::update() {
}

void ESP32SDMMC::dump_config() {
    ESP_LOGCONFIG(TAG, "SD MMC");
    uint8_t cardType = SD_MMC.cardType();

    if(cardType == CARD_NONE){
        ESP_LOGCONFIG(TAG, "SD Card Type: No SD_MMC card attached");
        return;
    }
    if(cardType == CARD_MMC){
        ESP_LOGCONFIG(TAG, "SD Card Type: MMC");
    } else if(cardType == CARD_SD){
        ESP_LOGCONFIG(TAG, "SD Card Type: SDSC");
    } else if(cardType == CARD_SDHC){
        ESP_LOGCONFIG(TAG, "SD Card Type: SDHC");
    } else {
        ESP_LOGCONFIG(TAG, "SD Card Type: UNKNOWN");
    }
    ESP_LOGCONFIG(TAG, "Total space: %lluMB", SD_MMC.totalBytes() / (1024 * 1024));
    ESP_LOGCONFIG(TAG, "Used space: %lluMB", SD_MMC.usedBytes() / (1024 * 1024));  
}

void  ESP32SDMMC::get_sd_lock(){
    ESP_LOGI(TAG, "SD Card Lock Aquired");
    this->sd_lock_.lock();
}

void ESP32SDMMC::return_sd_lock(){
    ESP_LOGI(TAG, "SD Card Lock Returned");
    this->sd_lock_.unlock();
}

} //namespace esp32_sdmmc
} //namespace esphome