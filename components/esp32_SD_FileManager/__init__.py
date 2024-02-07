import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_PORT

esp32_sd_filemanager_ns = cg.esphome_ns.namespace('esp32_sd_filemanager')

ESP32SDFM = esp32_sd_filemanager_ns.class_('ESP32SDFM', cg.Component)

DEPENDENCIES = ["esp32", "esp32_sdmmc"]
AUTO_LOAD = ["psram"]

CONFIG_SCHEMA = cv.ENTITY_BASE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(ESP32SDFM),
        cv.Required(CONF_PORT): cv.port,
    }).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    server = cg.new_Pvariable(config[CONF_ID])
    cg.add(server.set_port(config[CONF_PORT]))
    cg.add_library("FS","2.0.0")
    cg.add_library("SPI", "2.0.0")
    cg.add_library("WiFi", "2.0.0")
    cg.add_library("ESPmDNS", "2.0.0")
    cg.add_library("Webserver", "2.0.0")
    cg.add_library("SD_MMC", "2.0.0")
    cg.add_library("SD", "2.0.0")
    cg.add_library("arduino-libraries/Ethernet", "^2.0.2")
    cg.add_library("FFat", "2.0.0")
    cg.add_library("LittleFS", "2.0.0")
    #cg.add_library("holgerlembke/ESP32 File Manager for Generation Klick ESPFMfGK ", "^2.0.10")
    cg.add_library(None, None, "https://github.com/dlgreenwald/ESPHome-Bird-Detector/raw/main/lib/ei-bird-classifier-arduino-1.0.5.zip")
    await cg.register_component(server, config)