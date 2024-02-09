import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID, CONF_BYTES, ICON_DATABASE, CONF_TOTAL, ICON_COUNTER, STATE_CLASS_MEASUREMENT, DEVICE_CLASS_DATA_SIZE, DEVICE_CLASS_EMPTY 

esp32_sd_mmc_ns = cg.esphome_ns.namespace('esp32_sdmmc')

NUMFILES = "NumFiles"
DISKUSED = "DiskUsed"
DISRREM = "DiskRemaining"

ESP32SDMMC = esp32_sd_mmc_ns.class_('ESP32SDMMC', cg.PollingComponent, cg.EntityBase)

DEPENDENCIES = ["esp32"]
AUTO_LOAD = ["psram"]

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(ESP32SDMMC),
            cv.Required(NUMFILES): sensor.sensor_schema(
                unit_of_measurement=CONF_TOTAL,
                icon=ICON_COUNTER,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_EMPTY ,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Required(DISKUSED): sensor.sensor_schema(
                unit_of_measurement=CONF_BYTES,
                icon="mdi:content-save-move",
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_DATA_SIZE ,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Required(DISRREM): sensor.sensor_schema(
                unit_of_measurement=CONF_BYTES,
                icon="mdi:content-save-move",
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_DATA_SIZE ,
                state_class=STATE_CLASS_MEASUREMENT,
            ),  
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(cv.polling_component_schema("60s"))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    sens = await sensor.new_sensor(config[NUMFILES])
    cg.add(var.set_numFiles(sens))
    sens = await sensor.new_sensor(config[DISKUSED])
    cg.add(var.set_diskUsed(sens))
    sens = await sensor.new_sensor(config[DISRREM])
    cg.add(var.set_DiskRemaining(sens))
    cg.add_library("FS","2.0.0");
    cg.add_library("SD_MMC", "2.0.0")
#    cg.add_library("WiFi", "2.0.0")
    