#pragma once

//#include <cinttypes>
//#include <freertos/FreeRTOS.h>

//#include <Arduino.h>

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
//#include "esphome/core/preferences.h"

//#include "FS.h"

struct httpd_req;

namespace esphome {
namespace esp32_sd_filemanager {

//copied from CFS header as including it breaks esphome async web server
#define ESP_VFS_PATH_MAX 15
#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + CONFIG_SPIFFS_OBJ_NAME_LEN)

/* Max size of an individual file. Make sure this
 * value is same as that set in upload_script.html */
#define MAX_FILE_SIZE   (200*1024) // 200 KB
#define MAX_FILE_SIZE_STR "200KB"

/* Scratch buffer size */
#define SCRATCH_BUFSIZE  8192

struct file_server_data {
    /* Base path of file storage */
    char base_path[ESP_VFS_PATH_MAX + 1];

    /* Scratch buffer for temporary storage during file transfer */
    uint8_t scratch[SCRATCH_BUFSIZE];
};

class ESP32SDFM : public Component {
 public:
  ESP32SDFM();
  ~ESP32SDFM();

  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override;
  void set_port(uint16_t port) { this->port_ = port; }
  void loop() override;

 protected:
  esp_err_t start_file_server(const char *base_path);

  static esp_err_t index_html_get_handler(httpd_req *req);
  static esp_err_t favicon_get_handler(httpd_req *req);
  static esp_err_t http_resp_dir_html(httpd_req *req, const char *dirpath);
  static esp_err_t set_content_type_from_file(httpd_req *req, const char *filename);
  static const char* get_path_from_uri(char *dest, const char *base_path, const char *uri, size_t destsize);
  static esp_err_t download_get_handler(httpd_req *req);
  static esp_err_t upload_post_handler(httpd_req *req);
  static esp_err_t delete_post_handler(httpd_req *req);


  uint16_t port_{0};
  void *httpd_{nullptr};

 private:

};

} //namespace esp32_sd_filemanager
} //namespace esphome