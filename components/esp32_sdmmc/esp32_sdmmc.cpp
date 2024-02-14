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

float ESP32SDMMC::get_setup_priority() const { return setup_priority::DATA; }


void ESP32SDMMC::update() {
    get_sd_lock(TAG);
    uint16_t data_eco2;
    this->numFiles_->publish_state(countFiles("/", 3));
    this->diskUsed_->publish_state(SD_MMC.usedBytes());
    this->DiskRemaining_->publish_state(SD_MMC.totalBytes()-SD_MMC.usedBytes());
    return_sd_lock(TAG);
}


uint16_t ESP32SDMMC::countFiles(const char * dirname, uint8_t levels){
    ESP_LOGV(TAG, "Listing directory: %s", dirname);

    uint16_t count = 0;
    File root = SD_MMC.open(dirname);
    if(!root){
        ESP_LOGV(TAG, "Failed to open directory");
        return 0;
    }
    if(!root.isDirectory()){
        ESP_LOGV(TAG, "Not a directory");
        return 0;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            if(levels){
                count = count + countFiles(file.name(), levels -1);
            }
        } else {
            count = count+1;
        }
        ESP_LOGV(TAG, "Current Count @ %s is %i", dirname, count);
        file = root.openNextFile();
    }
    ESP_LOGV(TAG, "Count @ %s is %i", dirname, count);
    return count;
}

void ESP32SDMMC::dump_config() {
    ESP_LOGCONFIG(TAG, "SD MMC:");
    uint8_t cardType = SD_MMC.cardType();

    if(cardType == CARD_NONE){
        ESP_LOGCONFIG(TAG, "   SD Card Type: No SD_MMC card attached");
        return;
    }
    if(cardType == CARD_MMC){
        ESP_LOGCONFIG(TAG, "   SD Card Type: MMC");
    } else if(cardType == CARD_SD){
        ESP_LOGCONFIG(TAG, "   SD Card Type: SDSC");
    } else if(cardType == CARD_SDHC){
        ESP_LOGCONFIG(TAG, "   SD Card Type: SDHC");
    } else {
        ESP_LOGCONFIG(TAG, "   SD Card Type: UNKNOWN");
    }
    ESP_LOGCONFIG(TAG, "   Total space: %lluMB", SD_MMC.totalBytes() / (1024 * 1024));
    ESP_LOGCONFIG(TAG, "   Used space: %lluMB", SD_MMC.usedBytes() / (1024 * 1024));  
}

void  ESP32SDMMC::get_sd_lock(const char* clientTag){
    ESP_LOGI(TAG, "SD Card Lock Requested: %s", clientTag);
    this->sd_lock_.lock();
    ESP_LOGI(TAG, "SD Card Lock Aquired: %s", clientTag);
}

//not sure these pointers are right....should they be *& ?!?!
void ESP32SDMMC::return_sd_lock(const char* clientTag){
    this->sd_lock_.unlock();
    ESP_LOGI(TAG, "SD Card Lock Returned: %s", clientTag);
}

} //namespace esp32_sdmmc
} //namespace esphome