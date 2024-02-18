#ifdef USE_ESP32

#include "binary_bird_sensor.h"
#include "esphome/core/log.h"
#include "FS.h"
#include "SD_MMC.h"
#include "esphome/components/time/real_time_clock.h"
#include "Bird-classifier_inferencing.h"
#include "edge-impulse-sdk/dsp/image/processing.hpp"
#include "esphome/components/esp32_sdmmc/esp32_sdmmc.h"
#include "esphome/components/esp32_camera_plus/esp32_camera.h"
#include "esphome/core/log.h"
#include <esp_camera.h>
#include <cstdlib>
#include <utility>

namespace esphome {
namespace binary_bird_sensor {

static const int IMAGE_REQUEST_TIMEOUT = 5000;

#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define ms_TO_S_FACTOR 1000ULL    /* Conversion factor from milliseconds to seconds */
#define TIME_TO_WAKE_HRS 18       /* Once in deep sleep, will wait 18 hours to wake */
#define s_TO_HOUR_FACTOR 3600     /* 60 seconds in a minute, 60 minutes in a hour */
#define EI_CAMERA_RAW_FRAME_BUFFER_COLS           320
#define EI_CAMERA_RAW_FRAME_BUFFER_ROWS           240
#define EI_CAMERA_FRAME_BYTE_SIZE                 3

static const char *TAG = "binary_bird_sensor";

uint8_t *snapshot_buf;

BinaryBirdSensor::BinaryBirdSensor() {}

BinaryBirdSensor::~BinaryBirdSensor() {}

void BinaryBirdSensor::setup() {
    if (!esp32_camera::global_esp32_camera || esp32_camera::global_esp32_camera->is_failed()) {
        this->mark_failed();
        return;
    }

    if (!esphome::esp32_sdmmc::global_ESP32SDMMC || esphome::esp32_sdmmc::global_ESP32SDMMC->is_failed()) {
      this->mark_failed();
    return;
  }

    this->semaphore_ = xSemaphoreCreateBinary();

    //Put and empty CameraImage in image_
    //std::shared_ptr<esphome::esp32_camera::CameraImage> image;
    //image.swap(this->image_);

    esp32_camera::global_esp32_camera->add_image_callback([this](std::shared_ptr<esp32_camera::CameraImage> image) {
      if (image == nullptr) {
        ESP_LOGE(TAG, "Null pointer recieved");
      }else{
        //ESP_LOGD(TAG, "Image Callback to Sensor");
        //ESP_LOGE(TAG, "    image.use_count() == %i  (object @ %i )" , image.use_count(), image);
        //ESP_LOGE(TAG, "    image->was_requested_by(esp32_camera::WEB_REQUESTER)==%s", image->was_requested_by(esp32_camera::WEB_REQUESTER)?"true":"false");
        //ESP_LOGE(TAG, "    image->was_requested_by(esp32_camera::IDLE)==%s", image->was_requested_by(esp32_camera::IDLE)?"true":"false");
        //ESP_LOGE(TAG, "    image->was_requested_by(esp32_camera::API_REQUESTER)==%s", image->was_requested_by(esp32_camera::API_REQUESTER)?"true":"false");

        //move the new image over the top of the old image.
        //this->image_ = std::move(image); //<- doesn't work
        //std::move(image); // <-- works
        //image_ = image; //<doesn't work

        // std::shared_ptr<esp32_camera::CameraImage> image2;
        // image2 = std::move(image); <-- doesn't work

        if(image->was_requested_by(esp32_camera::CameraRequester::API_REQUESTER)){
          //ESP_LOGD(TAG, "Image Callback to Sensor");
          //ESP_LOGE(TAG, "    framebuffer moved to private member for later use: %d", framebuffer);
          //saveToSDcard(image->get_data_buffer(), image->get_data_length()); // <- Works

          ESP_LOGD(TAG, "Image Size: %u", image->get_data_length());
          if(image->get_data_length()>15000){
            //save high res photos if they come in as bird photos.
            ESP_LOGI(TAG, "High Res image taken, saving Image and setting resolution back to low.");
            saveToSDcard("/Birds", image->get_data_buffer(), image->get_data_length());
            //change resolution back to low
            esp32_camera::global_esp32_camera->change_camera_resolution(esp32_camera::ESP32_CAMERA_SIZE_320X240);
          }else{
            //Otherwise run classificationon them.
            camera_fb_t *fb = image->get_raw_buffer(); //<- works
            classify(fb);
          }
          


        }
        // if (this->image_ == nullptr){
        //   this->image_ = std::move(image);
        //   xSemaphoreGive(this->semaphore_);
        //   ESP_LOGD(TAG, "   Image moved");
        // }
      }
    });
    this->last_update_ = millis();
}
  
void BinaryBirdSensor::update() {
    // if (image_ == nullptr){
    //   ESP_LOGE(TAG, "Null pointer in image_");
    // }else{
    //   ESP_LOGD(TAG, "Update");
    //   ESP_LOGE(TAG, "    image_.use_count() == %i  (object @ %i )" , image_.use_count(), image_);
    //   ESP_LOGE(TAG, "    image_->was_requested_by(esp32_camera::WEB_REQUESTER)==%s", image_->was_requested_by(esp32_camera::WEB_REQUESTER)?"true":"false");
    //   ESP_LOGE(TAG, "    image_->was_requested_by(esp32_camera::IDLE)==%s", image_->was_requested_by(esp32_camera::IDLE)?"true":"false");
    //   ESP_LOGE(TAG, "    image_->was_requested_by(esp32_camera::API_REQUESTER)==%s", image_->was_requested_by(esp32_camera::API_REQUESTER)?"true":"false");
    //   framebuffer = nullptr;
    // }
    // if (esp32_camera::global_esp32_camera == nullptr){
    //   ESP_LOGE(TAG, "global camera instance is null!");
    // }

    uint32_t now = millis();
    if(now-last_update_>30000){
      ESP_LOGI(TAG, "Requesting Image!");
      esp32_camera::global_esp32_camera->request_image(esp32_camera::CameraRequester::API_REQUESTER);
        //only update this when we actually run.
        this->last_update_ = now;
    }
    // auto image = this->wait_for_image_();

    // if (!image) {
    //     ESP_LOGW(TAG, "SNAPSHOT: failed to acquire frame");
    //     return;
    // }

    //classify(image_);

    //done with this image.
    //this->image_ = nullptr;

}

void BinaryBirdSensor::dump_config() {
    ESP_LOGCONFIG(TAG, "Binary Bird Sensor");
    if (this->is_failed()) {
        ESP_LOGE(TAG, "  Setup Failed");
    }
}

// std::shared_ptr<esphome::esp32_camera::CameraImage> BinaryBirdSensor::wait_for_image_() {
//   ESP_LOGD(TAG, "In Wait for Image");
//   std::shared_ptr<esphome::esp32_camera::CameraImage> image;
//   image.swap(this->image_);
//   ESP_LOGD(TAG, "Image Swap Complete");
//   if (!image) {
//     ESP_LOGD(TAG, "Image Not Valid Waiting");
//     // retry as we might still be fetching image
//     xSemaphoreTake(this->semaphore_, IMAGE_REQUEST_TIMEOUT / portTICK_PERIOD_MS);
//     ESP_LOGD(TAG, "Back from Wait");
//     image.swap(this->image_);
//   }

//   return image;
// }

float BinaryBirdSensor::get_setup_priority() const { return setup_priority::LATE; }

void BinaryBirdSensor::classify( camera_fb_t *fb ){
    if(isCollecting_){
      saveToSDcard("/raw", fb->buf, fb->len);
    }
    

    snapshot_buf = (uint8_t *)ps_malloc(EI_CAMERA_RAW_FRAME_BUFFER_COLS * EI_CAMERA_RAW_FRAME_BUFFER_ROWS * EI_CAMERA_FRAME_BYTE_SIZE);
   // ESP_LOGE(TAG, "In Classify!");
    // check if allocation was successful
    if (snapshot_buf == nullptr) {
        ESP_LOGE(TAG, "ERR: Failed to allocate snapshot buffer!");
        return;
    } else {
      //ESP_LOGD(TAG, "Snapshot buffer allocated!");
    }

    ei::signal_t signal;
    signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
    signal.get_data = &ei_camera_get_data;

    if (ei_camera_capture(fb, (size_t)EI_CLASSIFIER_INPUT_WIDTH, (size_t)EI_CLASSIFIER_INPUT_HEIGHT, snapshot_buf) == false) {
        ESP_LOGE(TAG, "Failed to capture image");
        free(snapshot_buf);
        return;
    }

    // Run the classifier
    ei_impulse_result_t result = { 0 };

    EI_IMPULSE_ERROR err = run_classifier(&signal, &result, false /* debug */);
    if (err != EI_IMPULSE_OK) {
        ESP_LOGE(TAG, "ERR: Failed to run classifier (%d)", err);
        free(snapshot_buf);
        return;
    }

    // print the predictions
    ESP_LOGI(TAG, "Predictions (DSP: %d ms., Classification: %d ms., Anomaly: %d ms.):",
              result.timing.dsp, result.timing.classification, result.timing.anomaly);

    //bird found!
    if(result.classification[0].value > 0.5) //originally .80
    {
        ESP_LOGD(TAG, "**** Bird found with %f probability! ****", result.classification[0].value);
        //Change to high change_camera_resolution
        ESP_LOGI(TAG, "Changing mode to High Res");
        esp32_camera::global_esp32_camera->change_camera_resolution(esp32_camera::ESP32_CAMERA_SIZE_640X480);
        //Request image
        ESP_LOGI(TAG, "Requesting Image!");
        this->last_update_ = 0; //reset timer so next loop will request image.
        //The handler will check for a high res photo and change the resolution back
    }else{
        ESP_LOGD(TAG, "**** Bird NOT found with %f probability! ****", result.classification[0].value);
    }

    free(snapshot_buf);
    return;
}

void BinaryBirdSensor::saveToSDcard(const char * dirname, uint8_t *frame_buf, size_t buf_len) {
  // uint8_t *frame_buf = image->get_data_buffer();
  // size_t buf_len = image->get_data_length();

  auto t = this->time_->now();
  if (!t.is_valid()) {
    ESP_LOGE(TAG, "Failed to obtain time");
    return;
  }

  // Path where new picture will be saved in SD Card
  char timeStringBuff[50];
  t.strftime(timeStringBuff, 50, "%b-%d-%Y-%H%M%S");
  String time_string(timeStringBuff);
  String file_name = "/bird_" + time_string + ".jpg";
  //dir for photos
  const char *bird_path = dirname;

esphome::esp32_sdmmc::global_ESP32SDMMC->get_sd_lock(TAG);

  fs::FS &fs = SD_MMC;
  if (fs.mkdir(bird_path)) {
    ESP_LOGV(TAG, "path created");
  } else {
    ESP_LOGV(TAG, "path already created");
  }
  String path = String(bird_path) + file_name;
  ESP_LOGV(TAG, "Picture file name: %s", path.c_str());

  File file = fs.open(path.c_str(), FILE_WRITE);

  if (!file) {
    ESP_LOGV(TAG, "Failed to open file in writing mode");
  } else {
    file.write(frame_buf, buf_len);  // payload (image), payload length
    ESP_LOGV(TAG, "Saved file to path: %s", path.c_str());
  }
  file.close();

  esphome::esp32_sdmmc::global_ESP32SDMMC->return_sd_lock(TAG);
}

/**
 * @brief      Capture, rescale and crop image
 *
 * @param[in]  img_width     width of output image
 * @param[in]  img_height    height of output image
 * @param[in]  out_buf       pointer to store output image, NULL may be used
 *                           if ei_camera_frame_buffer is to be used for capture and resize/cropping.
 *
 * @retval     false if not initialised, image captured, rescaled or cropped failed
 *
 */
bool BinaryBirdSensor::ei_camera_capture(camera_fb_t *fb  , uint32_t img_width, uint32_t img_height, uint8_t *out_buf) {
  bool do_resize = false;

  //ESP_LOGE( TAG, "Capture and resize image");

  if (fb == nullptr) {
    ESP_LOGE( TAG, "Image is null pointer");
    return false;
  }

  if (!fb) {
    ESP_LOGE(TAG, "Camera capture failed");
    return false;
  }

  //ESP_LOGE(TAG, "FB -> buf: %d", fb->buf );
  //ESP_LOGE(TAG, "FB -> buf: %i", fb->len );
  bool converted = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, snapshot_buf);

  if (!converted) {
    ESP_LOGE(TAG, "Conversion failed");
    return false;
  }

  if ((img_width != EI_CAMERA_RAW_FRAME_BUFFER_COLS)
      || (img_height != EI_CAMERA_RAW_FRAME_BUFFER_ROWS)) {
    do_resize = true;
   // ESP_LOGE( TAG, "Resize Needed");
  }

  if (do_resize) {
    ei::image::processing::crop_and_interpolate_rgb888(
      out_buf,
      EI_CAMERA_RAW_FRAME_BUFFER_COLS,
      EI_CAMERA_RAW_FRAME_BUFFER_ROWS,
      out_buf,
      img_width,
      img_height);
     // ESP_LOGE( TAG, "Resize Complete");
  }


  return true;
}

int BinaryBirdSensor::ei_camera_get_data(size_t offset, size_t length, float *out_ptr) {
  // we already have a RGB888 buffer, so recalculate offset into pixel index
  size_t pixel_ix = offset * 3;
  size_t pixels_left = length;
  size_t out_ptr_ix = 0;

  while (pixels_left != 0) {
    out_ptr[out_ptr_ix] = (snapshot_buf[pixel_ix] << 16) + (snapshot_buf[pixel_ix + 1] << 8) + snapshot_buf[pixel_ix + 2];

    // go to the next pixel
    out_ptr_ix++;
    pixel_ix += 3;
    pixels_left--;
  }
  // and done!
  return 0;
}

} //namespace binary_bird_sensor
} //namespace esphome

#endif  // USE_ESP32