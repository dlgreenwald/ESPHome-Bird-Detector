#pragma once

#ifdef USE_ESP32

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "esphome/core/component.h"
#include "esphome/components/esp32_camera_plus/esp32_camera.h"
#include "esphome/components/time/real_time_clock.h"
#include <esp_camera.h>

namespace esphome {
namespace binary_bird_sensor {

class BinaryBirdSensor : public PollingComponent {
 public:
  BinaryBirdSensor();
  ~BinaryBirdSensor();

  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override;
  std::shared_ptr<esp32_camera::CameraImage> image_;

  void set_time(time::RealTimeClock *time) { time_ = time; }
  void set_collect_images(bool isCollecting){ isCollecting_ = isCollecting; }
  time::RealTimeClock *get_time() const { return time_; }
  
 protected:
   static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr);
   bool ei_camera_capture(camera_fb_t *fb, uint32_t img_width, uint32_t img_height, uint8_t *out_buf);
   void saveToSDcard(const char * dirname, uint8_t *frame_buf, size_t buf_len);
   void classify(camera_fb_t *fb);
//   std::shared_ptr<esphome::esp32_camera::CameraImage> wait_for_image_();

    SemaphoreHandle_t semaphore_;
    
    camera_fb_t *framebuffer;

   uint32_t last_update_{0};
   time::RealTimeClock *time_;
   bool isCollecting_;
// };
};

} //namespace binary_bird_sensor
} //namespace esphome

#endif  // USE_ESP32