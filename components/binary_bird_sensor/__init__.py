import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import time
from esphome.const import CONF_ID, CONF_TIME_ID

binary_bird_sensor_ns = cg.esphome_ns.namespace('binary_bird_sensor')

BinaryBirdSensor = binary_bird_sensor_ns.class_('BinaryBirdSensor', cg.PollingComponent)

DEPENDENCIES = ["esp32_camera_plus", "time", "esp32_sdmmc"]

CONF_COLLECT_IMAGES = "collect_images"

CONFIG_SCHEMA = (
    cv.ENTITY_BASE_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(BinaryBirdSensor),
            cv.GenerateID(CONF_TIME_ID): cv.use_id(time.RealTimeClock),
            cv.Optional(CONF_COLLECT_IMAGES, default={"false"}): cv.boolean,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(cv.polling_component_schema(".5s"))
)



async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

    time_ = await cg.get_variable(config[CONF_TIME_ID])
    cg.add(var.set_time(time_))

    
    cg.add(var.set_collect_images(config[CONF_COLLECT_IMAGES]))

    cg.add_library("FS","2.0.0")
    cg.add_library(None, None, "https://github.com/dlgreenwald/ESPHome-Bird-Detector/raw/main/lib/ei-bird-classifier-arduino-1.0.5.zip")
    await cg.register_component(var, config)
