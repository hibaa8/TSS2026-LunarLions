/**
 * Updates a sensor's visual state (green for active, gray for inactive)
 * @param {string} elementId - The ID of the sensor element to update
 * @param {boolean} isActive - Whether the sensor should be active (green) or inactive (gray)
 */
function updateSensor(elementId, isActive) {
  const element = document.getElementById(elementId);
  if (element) {
    element.style.backgroundColor = isActive ? "rgba(40, 174, 95, 1)" : "rgba(100, 100, 100, 1)";
  }
}

/**
 * Formats seconds into HH:MM:SS time format
 * @param {number} seconds - Total seconds to format
 * @returns {string} Formatted time string (HH:MM:SS)
 */
function formatTime(seconds) {
  const h = Math.floor(seconds / 3600);
  const m = Math.floor((seconds % 3600) / 60);
  const s = Math.floor(seconds % 60);
  return `${String(h).padStart(2, '0')}:${String(m).padStart(2, '0')}:${String(s).padStart(2, '0')}`;
}

/**
 * Updates a telemetry field with a formatted value and unit
 * @param {string} elementId - The ID of the element to update
 * @param {number} value - The numeric value to display
 * @param {string} unit - The unit to append (e.g., "psi", "%", "rpm")
 * @param {number} decimals - Number of decimal places to show (default: 2)
 */
function updateTelemetryField(elementId, value, unit = '', decimals = 2) {
  const element = document.getElementById(elementId);
  if (element) {
    const formattedValue = Number(value).toFixed(decimals);
    element.innerText = unit ? `${formattedValue} ${unit}` : formattedValue;
  }
}

/**
 * Updates a sensor and its corresponding switch based on boolean state
 * @param {string} sensorId - The ID of the sensor element
 * @param {string} switchId - The ID of the switch element
 * @param {boolean} isActive - Whether the component is active
 */
function updateSensorAndSwitch(sensorId, switchId, isActive) {
  updateSensor(sensorId, isActive);
  const switchElement = document.getElementById(switchId);
  if (switchElement) {
    switchElement.checked = isActive;
  }
}

/**
 * Updates telemetry fields for a specific EVA using utility functions
 * @param {Object} evaData - EVA telemetry data object
 * @param {string} evaNum - EVA number ("1" or "2")
 */
function updateEVATelemetry(evaData, evaNum) {
  // Time fields
  document.getElementById(`o2Time${evaNum}`).innerText = formatTime(Number(evaData.oxy_time_left));
  
  // Storage percentages
  updateTelemetryField(`primaryO2Storage${evaNum}`, evaData.oxy_pri_storage, "%", 0);
  updateTelemetryField(`secondaryO2Storage${evaNum}`, evaData.oxy_sec_storage, "%", 0);
  
  // Pressures
  updateTelemetryField(`primaryO2Pressure${evaNum}`, evaData.oxy_pri_pressure, "psi");
  updateTelemetryField(`secondaryO2Pressure${evaNum}`, evaData.oxy_sec_pressure, "psi");
  updateTelemetryField(`suitO2Pressure${evaNum}`, evaData.suit_pressure_oxy, "psi");
  updateTelemetryField(`suitCO2Pressure${evaNum}`, evaData.suit_pressure_co2, "psi");
  updateTelemetryField(`suitOtherPressure${evaNum}`, evaData.suit_pressure_other, "psi");
  updateTelemetryField(`suitTotalPressure${evaNum}`, evaData.suit_pressure_total, "psi");
  updateTelemetryField(`scrubberAPressure${evaNum}`, evaData.scrubber_a_co2_storage, "psi");
  updateTelemetryField(`scrubberBPressure${evaNum}`, evaData.scrubber_b_co2_storage, "psi");
  updateTelemetryField(`h2OGasPressure${evaNum}`, evaData.coolant_gas_pressure, "psi");
  updateTelemetryField(`h2OLiquidPressure${evaNum}`, evaData.coolant_liquid_pressure, "psi");
  updateTelemetryField(`helmetCO2Pressure${evaNum}`, evaData.helmet_pressure_co2, "psi");
  
  // Consumption/Production rates
  updateTelemetryField(`o2Consumption${evaNum}`, evaData.oxy_consumption, "ml/min", 1);
  updateTelemetryField(`co2Production${evaNum}`, evaData.co2_production, "ml/min", 1);
  
  // Fan speeds
  updateTelemetryField(`primaryFan${evaNum}`, evaData.fan_pri_rpm, "rpm", 0);
  updateTelemetryField(`secondaryFan${evaNum}`, evaData.fan_sec_rpm, "rpm", 0);
  
  // Health metrics
  updateTelemetryField(`heartRate${evaNum}`, evaData.heart_rate, "bpm");
  updateTelemetryField(`temperature${evaNum}`, evaData.temperature, "deg F");
  updateTelemetryField(`coolant${evaNum}`, evaData.coolant_ml, "ml");
}

/**
 * Updates SPEC data for a specific EVA
 * @param {Object} evaData - EVA spec data object
 * @param {string} evaNum - EVA number ("1" or "2")
 * @param {Array} specFields - Array of spectroscopy field names
 */
function updateEvaSpecData(evaData, evaNum, specFields) {
  // Update rock name
  document.getElementById(`eva${evaNum}_rock_name`).innerText = evaData.name;
  
  // Update all spectroscopy fields
  specFields.forEach(field => {
    const elementId = `eva${evaNum}_${field.toLowerCase()}`;
    document.getElementById(elementId).innerText = evaData.data[field];
  });
  
  // Update dropdown selection
  document.getElementById(`dropdownEV${evaNum}`).value = evaData.id;
}