import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

esp32_sd_mmc_ns = cg.esphome_ns.namespace('esp32_sdmmc')

ESP32SDMMC = esp32_sd_mmc_ns.class_('ESP32SDMMC', cg.PollingComponent, cg.EntityBase)

DEPENDENCIES = ["esp32"]
AUTO_LOAD = ["psram"]

CONFIG_SCHEMA = cv.ENTITY_BASE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(ESP32SDMMC),
    }).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add_library("FS","2.0.0");
    cg.add_library("SD_MMC", "2.0.0")
    cg.add_library("WiFi", "2.0.0")
#    cg.add_library(
#        "ESPFMfGK",
#        None,
#        "https://github.com/Sensirion/arduino-gas-index-algorithm.git#3.2.1",
#    )
    