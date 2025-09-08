/**
 * UIA sensor and switch mappings configuration
 */
const UIA_SENSORS_AND_SWITCHES = {
  eva1_power: { sensor: "eva1PowerSensor", switch: "uia_eva1_power_switch" },
  eva2_power: { sensor: "eva2PowerSensor", switch: "uia_eva2_power_switch" }, 
  eva1_water_supply: { sensor: "eva1WaterSupplySensor", switch: "uia_eva1_water_supply_switch" },
  eva2_water_supply: { sensor: "eva2WaterSupplySensor", switch: "uia_eva2_water_supply_switch" },
  eva1_water_waste: { sensor: "eva1WaterWasteSensor", switch: "uia_eva1_water_waste_switch" },
  eva2_water_waste: { sensor: "eva2WaterWasteSensor", switch: "uia_eva2_water_waste_switch" },
  eva1_oxy: { sensor: "eva1OxygenSensor", switch: "uia_eva1_oxy_switch" },
  eva2_oxy: { sensor: "eva2OxygenSensor", switch: "uia_eva2_oxy_switch" },
  oxy_vent: { sensor: "oxygenVentSensor", switch: "uia_oxy_vent_switch" },
  depress: { sensor: "depressPumpSensor", switch: "uia_depress_switch" }
};

/**
 * Loads UIA data and updates all sensor/switch states using configuration-driven approach
 * Replaces 98+ lines of repetitive if/else blocks with simple loop
 */
function loadUIA() {
  $.getJSON("data/UIA.json", function (data) {
    // Update all UIA sensors and switches using configuration mapping
    Object.keys(UIA_SENSORS_AND_SWITCHES).forEach(key => {
      const config = UIA_SENSORS_AND_SWITCHES[key];
      updateSensorAndSwitch(config.sensor, config.switch, data.uia[key]);
    });
  });
}

//Runs on load of the page
function onload() {
  // immediately load
  loadUIA();

  // Continuously refreshes values from the UIA
  setInterval(function () {
    loadUIA();
  }, 1000);
}
