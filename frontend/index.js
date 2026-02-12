// GLOBAL VARIABLES
let connectionFails = 0; // number of consecutive connection failures, resets on successful fetch
let dustConnected = false; // tracks DUST/Unreal Engine connection status
let evaStarted = false; //variable to track if EVA has been started yet

//telemetry ranges
const TELEMETRY_RANGES  = {
  "eva.telemetry.eva1.primary_battery_level": { min: 20, max: 100, units: "%" },
  "eva.telemetry.eva1.secondary_battery_level": { min: 20, max: 100, units: "%" },
  "eva.telemetry.eva1.oxy_pri_storage": { min: 20, max: 100, units: "%" },
  "eva.telemetry.eva1.oxy_sec_storage": { min: 20, max: 100, units: "%" },
  "eva.telemetry.eva1.oxy_pri_pressure": { min: 600, max: 3000, units: "psi" },
  "eva.telemetry.eva1.oxy_sec_pressure": { min: 600, max: 3000, units: "psi" },
  "eva.telemetry.eva1.coolant_storage": { min: 80, max: 100, units: "%" },
  "eva.telemetry.eva1.heart_rate": { min: 50, max: 160, units: "bpm" },
  "eva.telemetry.eva1.oxy_consumption": { min: 0.05, max: 0.15, units: "psi/min" },
  "eva.telemetry.eva1.co2_production": { min: 0.05, max: 0.15, units: "psi/min" },
  "eva.telemetry.eva1.suit_pressure_oxy": { min: 3.5, max: 4.1, units: "psi" },
  "eva.telemetry.eva1.suit_pressure_co2": { min: 0, max: 0.1, units: "psi" },
  "eva.telemetry.eva1.suit_pressure_other": { min: 0, max: 0.5, units: "psi" },
  "eva.telemetry.eva1.suit_pressure_total": { min: 3.5, max: 4.5, units: "psi" },
  "eva.telemetry.eva1.helmet_pressure_co2": { min: 0, max: 0.15, units: "psi" },
  "eva.telemetry.eva1.fan_pri_rpm": { min: 20000, max: 30000, units: "rpm" },
  "eva.telemetry.eva1.fan_sec_rpm": { min: 20000, max: 30000, units: "rpm" },
  "eva.telemetry.eva1.scrubber_a_co2_storage": { min: 0, max: 60, units: "%" },
  "eva.telemetry.eva1.scrubber_b_co2_storage": { min: 0, max: 60, units: "%" },
  "eva.telemetry.eva1.temperature": { min: 10, max: 32, units: "°C" },
  "eva.telemetry.eva1.coolant_liquid_pressure": { min: 100, max: 700, units: "psi" },
  "eva.telemetry.eva1.coolant_gas_pressure": { min: 0, max: 700, units: "psi" },
};



/**
 * Called when the index.html page is loaded, sets up the periodic data fetching
 */
function onload() {
  updateClock(); // set clock immediately on load (will be updated every second later)

  // Fetch fresh data from the backend every one second
  setInterval(() => {
    fetchData();
    updateClock();
    updateTelemetryStatus();
    updateDustStatus();
  }, 1000);


  // Set up event listeners for switches and buttons
  setupEventListeners();
}

// DATA MANAGEMENT

/**
 * Fetches the latest EVA and ROVER data and updates the DOM elements accordingly.
 * In the html, there is a data-path attribute on the elements that basically registers that field as needing to be updated with data from the server
 * The path specifies where in the JSON data the value can be found, e.g. "eva.telemetry.eva1.temperature" or "rover.pr_telemetry.battery_voltage"
 *
 * Note: this is a bit of a hodgepodge to mantain backwards compatibility with the old system e.g. timers, boolean switches, etc
 */
async function fetchData() {
  let evaData, roverData, ltvData;

  try {
    // Create abort controllers with 1 second timeout
    const evaController = new AbortController();
    const roverController = new AbortController();
    const ltvController = new AbortController();

    const timeoutIds = [
      setTimeout(() => evaController.abort(), 2000),
      setTimeout(() => roverController.abort(), 2000),
      setTimeout(() => ltvController.abort(), 2000),
    ];

    // Fetch EVA, ROVER, and LTV data simultaneously
    const [evaResponse, roverResponse, ltvResponse] = await Promise.all([
      fetch(`/data/EVA.json`, { signal: evaController.signal }),
      fetch(`/data/ROVER.json`, { signal: roverController.signal }),
      fetch(`/data/LTV.json`, { signal: ltvController.signal }),
    ]);

    // Clear timeouts on successful response
    timeoutIds.forEach((id) => clearTimeout(id));

    [evaData, roverData, ltvData] = await Promise.all([
      evaResponse.json(),
      roverResponse.json(),
      ltvResponse.json(),
    ]);

    //check if telemetry component has been started by seeing if EVA.status.started is true
    evaStarted = evaData?.status?.started === true;

    connectionFails = 0;
    dustConnected =
      (roverData?.pr_telemetry?.dust_connected && connectionFails <= 2) ||
      false;
  } catch (error) {
    console.error("Fatal error fetching data:", error);
    connectionFails++;
    return;
  }

  // Update the EVA, ROVER, and LTV fields in the DOM
  const elements = document.querySelectorAll("[data-path]");
  elements.forEach((el) => {
    const path = el.getAttribute("data-path");

    let value;

    if (path.startsWith("eva.")) {
      value = getNestedValue(evaData, path.slice(4));
    }

    if (path.startsWith("rover.")) {
      value = getNestedValue(roverData, path.slice(6));
    }

    if (path.startsWith("ltv.")) {
      value = getNestedValue(ltvData, path.slice(4));
    }

    // Special handling for signal strength
    if (path === "ltv.signal.strength") {
      if (value === 1.00) {
        el.textContent = "NOT IN RANGE";
        return;
      }
    }

    // Handle checkboxes/switches (set checked property for boolean values)
    if (el.type === "checkbox") {
      el.checked = Boolean(value);
      return; // don't set textContent for checkboxes
    }

    // Handle action buttons (START/RESET) - enable/disable based on current state
    if (el.tagName === "BUTTON" && el.hasAttribute("data-action")) {
      const action = el.getAttribute("data-action");
      const isRunning = Boolean(value);

      if (action === "start") {
        // START button: enabled when NOT running
        el.disabled = isRunning;
        el.style.opacity = isRunning ? "0.5" : "1";
      } else if (action === "reset") {
        // RESET button: enabled when IS running
        el.disabled = !isRunning;
        el.style.opacity = !isRunning ? "0.5" : "1";
      }

      return; // don't set textContent for action buttons
    }

    // Handle time formatting if data-format="time" is specified
    const format = el.getAttribute("data-format");
    if (format === "time" && typeof value === "number") {
      value = formatTime(value);
    } else if (format === "status" && typeof value === "boolean") {
      value = value ? "Complete" : "Incomplete";
    } else if (typeof value === "number") {
      // Handle other number formatting for text elements
      value = value.toFixed(2);
    }

    // Append units if specified
    const units = el.getAttribute("data-units");
    if (units) {
      value = `${value} ${units}`;
    }

    el.textContent = value;
  });
}

/**
 * Updates the server with the new value for a specific field using data-path format
 *
 * @param path Data path (e.g., "eva.dcu.eva1.batt" or "rover.status.started")
 * @param value New value for the field
 */
async function updateServerData(path, value) {
  try {
    const params = new URLSearchParams();
    params.append(path, value);

    const response = await fetch(`/`, {
      method: "POST",
      headers: {
        "Content-Type": "application/x-www-form-urlencoded",
      },
      body: params,
    });
  } catch (error) {
    console.error("Fatal error updating server data:", error);
  }
}

// EVENT LISTENERS

function setupEventListeners() {
  // When any of the switch values are toggled, send the new value to the server
  const switches = document.querySelectorAll(
    'input[type="checkbox"][data-path]'
  );

  switches.forEach((switchEl) => {
    switchEl.addEventListener("change", (event) => {
      const path = event.target.getAttribute("data-path");
      const value = event.target.checked;
      updateServerData(path, value);
    });
  });

  // When any of the buttons with data-path are clicked, send the value to the server
  const buttons = document.querySelectorAll("button[data-path]");

  buttons.forEach((button) => {
    button.addEventListener("click", (event) => {
      const path = event.target.getAttribute("data-path");
      const action = event.target.getAttribute("data-action");

      // For action buttons, directly update the status field
      if (action === "start") {
        // Special handler to only allow pinging when the DUST sim is connected, @TODO see if this can be generalized later
        if (path === "ltv.signal.ping_requested") {
          if (!dustConnected) {
            alert(
              "DUST simulator is not connected. Please refer to TSS documentation for learning how to connect to the simulator."
            );
            return;
          }
        }

        // Start: set the field to true
        updateServerData(path, true);
      } else if (action === "reset") {
        // Reset: set the field to false
        updateServerData(path, false);
      } else {
        // Fallback to old data-value system for backward compatibility
        const value = event.target.getAttribute("data-value") === "true";
        updateServerData(path, value);
      }
    });
  });
}

// HELPER FUNCTIONS


/**
 * Retrieves a nested value from an object/json using a dot-separated path
 *
 * @param obj Object to retrieve the value from
 * @param path Dot-separated path to the desired value
 * @returns The value at the specified path or undefined if not found
 */
function getNestedValue(obj, path) {
  if (!path) return undefined;

  return path.split(".").reduce((current, key) => {
    return current && current[key] !== undefined ? current[key] : undefined;
  }, obj);
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
  return `${String(h).padStart(2, "0")}:${String(m).padStart(2, "0")}:${String(
    s
  ).padStart(2, "0")}`;
}

/**
 * Sets a cookie in the browser
 *
 * @param cname Cookie name
 * @param cvalue Cookie value
 */
function setCookie(cname, cvalue) {
  document.cookie = `${cname}=${cvalue};path=/`;
}

/**
 * Retrieves a cookie from the browser
 *
 * @param cname Cookie name
 * @returns Cookie value or null if not found
 */
function getCookie(cname) {
  const name = `${cname}=`;
  const decodedCookie = decodeURIComponent(document.cookie);
  const ca = decodedCookie.split(";");

  for (let i = 0; i < ca.length; i++) {
    let c = ca[i];
    while (c.charAt(0) === " ") {
      c = c.substring(1);
    }
    if (c.indexOf(name) === 0) {
      return c.substring(name.length, c.length);
    }
  }

  return null;
}

/**
 * Updates the clock in the navigation bar based on military time format
 */
function updateClock() {
  const now = new Date();
  const hours = String(now.getHours()).padStart(2, "0");
  const minutes = String(now.getMinutes()).padStart(2, "0");
  const seconds = String(now.getSeconds()).padStart(2, "0");
  const timeString = `${hours}:${minutes}:${seconds}`;

  const clockElement = document.getElementById("nav-clock");
  if (clockElement) {
    clockElement.textContent = timeString;
  }
}

/**
 * Updates the telemetry status indicator in the navigation bar
 */
function updateTelemetryStatus() {
  const statusElement = document.getElementById("telemetry-status");
  if (statusElement) {
    const isConnected = connectionFails <= 2;
    const statusText = isConnected
      ? "Telemetry Connected"
      : "Telemetry Disconnected";
    statusElement.innerHTML = `● ${statusText}`;
    statusElement.style.color = isConnected ? "#28ae5f" : "#d82121ff";
  }
}

/**
 * Updates the DUST connection status indicator in the navigation bar
 */
function updateDustStatus() {
  const statusElement = document.getElementById("dust-status");
  if (statusElement) {
    const statusText = dustConnected ? "DUST Connected" : "DUST Disconnected"; // make sure that TSS is connected too to show that DUST is connected
    statusElement.innerHTML = `● ${statusText}`;
    statusElement.style.color = dustConnected ? "#28ae5f" : "#3889abff";
  }
}
