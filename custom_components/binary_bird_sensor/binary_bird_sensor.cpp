#include "esphome/core/log.h"
#include "binary_bird_sensor.h"
#include "FS.h"
#include "SD_MMC.h"
#include "esphome/components/time/real_time_clock.h"
#include "Bird-classifier_inferencing.h"
#include "edge-impulse-sdk/dsp/image/processing.hpp"

namespace esphome {
namespace binary_bird_sensor {

static const int IMAGE_REQUEST_TIMEOUT = 5000;

#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define ms_TO_S_FACTOR 1000ULL    /* Conversion factor from milliseconds to seconds */
#define TIME_TO_WAKE_HRS 18       /* Once in deep sleep, will wait 18 hours to wake */
#define s_TO_HOUR_FACTOR 3600     /* 60 seconds in a minute, 60 minutes in a hour */
#define EI_CAMERA_RAW_FRAME_BUFFER_COLS 48
#define EI_CAMERA_RAW_FRAME_BUFFER_ROWS 48
#define EI_CAMERA_FRAME_BYTE_SIZE 3

static const char *TAG = "binary_bird_sensor.binary_sensor";

uint8_t *snapshot_buf;

void BinaryBirdSensor::setup() {
    if (!esp32_camera::global_esp32_camera || esp32_camera::global_esp32_camera->is_failed()) {
        this->mark_failed();
        return;
    }

    this->semaphore_ = xSemaphoreCreateBinary();

    esp32_camera::global_esp32_camera->add_image_callback([this](std::shared_ptr<esp32_camera::CameraImage> image) {
        if (image->was_requested_by(esp32_camera::API_REQUESTER)) {
        this->image_ = std::move(image);
        xSemaphoreGive(this->semaphore_);
        }
    });

    //SETUP SD....@TODO This needs to be in a component and the component should offer a MUTEX guard.
    if (!SD_MMC.begin()) {
      Serial.println("SD Card Mount Failed");
    }

    uint8_t cardType = SD_MMC.cardType();
    if (cardType == CARD_NONE) {
      Serial.println("No SD Card attached");
    }

    this->last_update_ = millis();
}
  
void BinaryBirdSensor::update() {
    esp32_camera::global_esp32_camera->request_image(esphome::esp32_camera::API_REQUESTER);
    auto image = this->wait_for_image_();

    
    if (!image) {
        ESP_LOGW(TAG, "SNAPSHOT: failed to acquire frame");
        return;
    }

    classify(image);

    //only update this if a bird is found
    this->last_update_ = millis();
}

void BinaryBirdSensor::dump_config() {
    ESP_LOGCONFIG(TAG, "Binary Bird Sensor");
    if (this->is_failed()) {
        ESP_LOGE(TAG, "  Setup Failed");
    }
}

std::shared_ptr<esphome::esp32_camera::CameraImage> BinaryBirdSensor::wait_for_image_() {
  std::shared_ptr<esphome::esp32_camera::CameraImage> image;
  image.swap(this->image_);

  if (!image) {
    // retry as we might still be fetching image
    xSemaphoreTake(this->semaphore_, IMAGE_REQUEST_TIMEOUT / portTICK_PERIOD_MS);
    image.swap(this->image_);
  }

  return image;
}

float BinaryBirdSensor::get_setup_priority() const { return setup_priority::LATE; }

bool BinaryBirdSensor::classify(esphome::esp32_camera::CameraImage image){
    snapshot_buf = (uint8_t *)malloc(EI_CAMERA_RAW_FRAME_BUFFER_COLS * EI_CAMERA_RAW_FRAME_BUFFER_ROWS * EI_CAMERA_FRAME_BYTE_SIZE);

    // check if allocation was successful
    if (snapshot_buf == nullptr) {
        ESP_LOGE("ERR: Failed to allocate snapshot buffer!\n");
        return;
    }

    ei::signal_t signal;
    signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
    signal.get_data = &ei_camera_get_data;

    if (ei_camera_capture(image, (size_t)EI_CLASSIFIER_INPUT_WIDTH, (size_t)EI_CLASSIFIER_INPUT_HEIGHT, snapshot_buf) == false) {
        ESP_LOGE("Failed to capture image\r\n");
        free(snapshot_buf);
        return;
    }

    // Run the classifier
    ei_impulse_result_t result = { 0 };

    EI_IMPULSE_ERROR err = run_classifier(&signal, &result, false /* debug */);
    if (err != EI_IMPULSE_OK) {
        ESP_LOGE("ERR: Failed to run classifier (%d)\n", err);
        free(snapshot_buf);
        return;
    }

    // print the predictions
    ESP_LOGI("Predictions (DSP: %d ms., Classification: %d ms., Anomaly: %d ms.): \n",
              result.timing.dsp, result.timing.classification, result.timing.anomaly);

    //bird found!
    if(result.classification[0].value > 0.8)
    {
        ESP_LOGI("**** Bird found with %f probability! ****\n", result.classification[0].value);
        saveToSDcard(image);
    }else{
        ESP_LOGV("**** Bird NOT found with %f probability! ****\n", result.classification[0].value);
    }

    free(snapshot_buf);
    return;
}

void BinaryBirdSensor::saveToSDcard(esphome::esp32_camera::CameraImage image) {
  uint8_t *frame_buf = image.get_data_buffer();
  size_t buf_len = image.get_data_length();

  auto t = this->time_->now();
  if (!t.is_valid()) {
    ESP_LOGE("Failed to obtain time");
    return;
  }

  // Path where new picture will be saved in SD Card
  char timeStringBuff[50];
  printf(timeStringBuff, sizeof(timeStringBuff), "%04d-%02d-%02d %02d:%02d:%02d", t.year, t.month, t.day_of_month, t.hour, t.minute, t.second);
  String time_string(timeStringBuff);
  String file_name = "/bird_" + time_string + ".jpg";
  //dir for photos
  const char *bird_path = "/Photos";

  fs::FS &fs = SD_MMC;
  if (fs.mkdir(bird_path)) {
    ESP_LOGI("path created");
  } else {
    ESP_LOGI("path already created");
  }
  String path = String(bird_path) + file_name;
  ESP_LOGI("Picture file name: %s\n", path.c_str());

  File file = fs.open(path.c_str(), FILE_WRITE);

  if (!file) {
    ESP_LOGI("Failed to open file in writing mode");
  } else {
    file.write(frame_buf, buf_len);  // payload (image), payload length
    ESP_LOGI("Saved file to path: %s\n", path.c_str());
  }
  file.close();
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
bool BinaryBirdSensor::ei_camera_capture(esphome::esp32_camera::CameraImage image, uint32_t img_width, uint32_t img_height, uint8_t *out_buf) {
  bool do_resize = false;

  camera_fb_t *fb = image.get_raw_buffer();

  if (!fb) {
    ESP_LOGE("Camera capture failed\n");
    return false;
  }

  bool converted = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, snapshot_buf);

  if (!converted) {
    ESP_LOGE("Conversion failed\n");
    return false;
  }

  if ((img_width != EI_CAMERA_RAW_FRAME_BUFFER_COLS)
      || (img_height != EI_CAMERA_RAW_FRAME_BUFFER_ROWS)) {
    do_resize = true;
  }

  if (do_resize) {
    ei::image::processing::crop_and_interpolate_rgb888(
      out_buf,
      EI_CAMERA_RAW_FRAME_BUFFER_COLS,
      EI_CAMERA_RAW_FRAME_BUFFER_ROWS,
      out_buf,
      img_width,
      img_height);
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