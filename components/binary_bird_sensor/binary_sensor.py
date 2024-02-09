import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_ID

binary_bird_sensor_ns = cg.esphome_ns.namespace('binary_bird_sensor')

BinaryBirdSensor = binary_bird_sensor_ns.class_('BinaryBirdSensor', binary_sensor.BinarySensor, cg.Component)

DEPENDENCIES = ["esp32_camera", "time", "esp32_sdmmc"]

CONFIG_SCHEMA = binary_sensor.BINARY_SENSOR_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(BinaryBirdSensor),
}).extend(cv.COMPONENT_SCHEMA)


def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    cg.add_library("FS","2.0.0")
    cg.add_library(None, None, "https://github.com/dlgreenwald/ESPHome-Bird-Detector/raw/main/lib/ei-bird-classifier-arduino-1.0.5.zip")
    yield cg.register_component(var, config)
    yield binary_sensor.register_binary_sensor(var, config)