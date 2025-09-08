/**
 * DCU sensor and switch mappings configuration
 */
const DCU_SENSORS_AND_SWITCHES = {
  "eva1.batt": { sensor: "eva1BatterySensor", switch: "eva1_dcu_batt_switch" },
  "eva1.oxy": { sensor: "eva1OxygenSensor", switch: "eva1_dcu_oxy_switch" },
  "eva1.comm": { sensor: "eva1CommSensor", switch: "eva1_dcu_comm_switch" },
  "eva1.fan": { sensor: "eva1FanSensor", switch: "eva1_dcu_fan_switch" },
  "eva1.pump": { sensor: "eva1PumpSensor", switch: "eva1_dcu_pump_switch" },
  "eva1.co2": { sensor: "eva1Co2Sensor", switch: "eva1_dcu_co2_switch" },
  "eva2.batt": { sensor: "eva2BatterySensor", switch: "eva2_dcu_batt_switch" },
  "eva2.oxy": { sensor: "eva2OxygenSensor", switch: "eva2_dcu_oxy_switch" },
  "eva2.comm": { sensor: "eva2CommSensor", switch: "eva2_dcu_comm_switch" },
  "eva2.fan": { sensor: "eva2FanSensor", switch: "eva2_dcu_fan_switch" },
  "eva2.pump": { sensor: "eva2PumpSensor", switch: "eva2_dcu_pump_switch" },
  "eva2.co2": { sensor: "eva2Co2Sensor", switch: "eva2_dcu_co2_switch" },
};

/**
 * Loads DCU data and updates all sensor/switch states using configuration-driven approach
 * Replaces 126+ lines of repetitive if/else blocks with simple loop
 */
function loadDCU() {
  $.getJSON("data/DCU.json", function (data) {
    // Update all DCU sensors and switches using configuration mapping
    Object.keys(DCU_SENSORS_AND_SWITCHES).forEach((key) => {
      // Handle nested object paths like "eva1.batt"
      const keys = key.split(".");
      let value = data.dcu;
      keys.forEach((k) => (value = value[k]));

      const config = DCU_SENSORS_AND_SWITCHES[key];
      updateSensorAndSwitch(config.sensor, config.switch, value);
    });
  });
}

//Runs on load of the page
function onload() {
  // Load immediately
  loadDCU();

  // Continuously refreshes values from the DCU
  setInterval(function () {
    loadDCU();
  }, 1000);
}
