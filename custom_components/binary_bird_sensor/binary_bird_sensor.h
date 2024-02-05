#pragma once

#include "esphome/core/component.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/esp32_camera/esp32_camera.h"
#include "esphome/components/time/real_time_clock.h"

namespace esphome {
namespace binary_bird_sensor {

class BinaryBirdSensor : public binary_sensor::BinarySensor, public PollingComponent {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override;
  
 protected:
  static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr);
  bool ei_camera_capture(esphome::esp32_camera::CameraImage image, uint32_t img_width, uint32_t img_height, uint8_t *out_buf);
  void saveToSDcard(esphome::esp32_camera::CameraImage image);
  void classify(esphome::esp32_camera::CameraImage image);
  std::shared_ptr<esphome::esp32_camera::CameraImage> wait_for_image_();

  SemaphoreHandle_t semaphore_;
  std::shared_ptr<esphome::esp32_camera::CameraImage> image_;

  uint32_t last_update_{0};
  time::RealTimeClock *time_;
};

} //namespace binary_bird_sensor
} //namespace esphome