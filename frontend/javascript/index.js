// GLOBAL VARIABLES
let currentRoom = 0;

/**
 * Called when the index.html page is loaded, sets up the periodic data fetching
 */
function onload() {
  updateClock(); // set clock immediately on load (will be updated every second later)

  // Get the current room from the cookie
  currentRoom = getCookie("room") || 0;

  // Fetch fresh data from the backend every one second
  setInterval(() => {
    fetchData();
    updateClock();
  }, 1000);

  // Fetch the teams to populate the dropdown
  getTeams();

  // Set up event listeners for switches and buttons
  setupEventListeners();
}

// DATA MANAGEMENT

/**
 * Fetches the latest EVA and ROVER data and updates the DOM elements accordingly.
 * In the html, there is a data-path attribute on the elements that basically registers that field as needing to be updated with data from the server
 * The path specifies where in the JSON data the value can be found, e.g. "eva.telemetry.eva1.temperature" or "rover.pr_telemetry.battery_voltage"
 * 
 * Note: this is a bit of a hodgepodge to mantain backwards compatibility with the old system I.E. timers, boolean switches, etc
 */
async function fetchData() {
  // Fetch both EVA and ROVER data simultaneously
  const [evaResponse, roverResponse] = await Promise.all([
    fetch(`/data/teams/${currentRoom}/EVA.json`),
    fetch(`/data/teams/${currentRoom}/ROVER.json`)
  ]);

  const [evaData, roverData] = await Promise.all([
    evaResponse.json(),
    roverResponse.json()
  ]);

  // Update the EVA and ROVER fields in the DOM
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

    // Handle checkboxes/switches (set checked property for boolean values)
    if (el.type === "checkbox") {
      el.checked = Boolean(value);
      return; // don't set textContent for checkboxes
    }

    // Handle action buttons (START/STOP) - enable/disable based on current state
    if (el.tagName === "BUTTON" && el.hasAttribute("data-action")) {
      const action = el.getAttribute("data-action");
      const isRunning = Boolean(value);

      if (action === "start") {
        // START button: enabled when NOT running
        el.disabled = isRunning;
        el.style.opacity = isRunning ? "0.5" : "1";
      } else if (action === "stop") {
        // STOP button: enabled when IS running
        el.disabled = !isRunning;
        el.style.opacity = !isRunning ? "0.5" : "1";
      }
      
      return; // don't set textContent for action buttons
    }

    // Handle time formatting if data-format="time" is specified
    const format = el.getAttribute("data-format");
    if (format === "time" && typeof value === "number") {
      value = formatTime(value);
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
  const params = new URLSearchParams();
  params.append(path, value);
  params.append("room", currentRoom);

  await fetch(`/`, {
    method: "POST",
    headers: {
      "Content-Type": "application/x-www-form-urlencoded",
    },
    body: params,
  });
}

/**
 * Fetches the list of teams and populates the team selection dropdown
 */
async function getTeams() {
  // Fetch the TEAMS.json data to update the dropdown
  const response = await fetch(`/data/TEAMS.json`);
  const teamsData = await response.json();

  // Update the team selector dropdown
  const teamSelector = document.getElementById("roomSelector");
  teamSelector.innerHTML = ""; // Clear existing options

  // Iterate over the teams object
  Object.entries(teamsData.teams).forEach(([teamKey, teamName], index) => {
    const option = document.createElement("option");
    option.value = index;
    option.textContent = `${teamName} | Room: ${index}`;
    teamSelector.appendChild(option);
  });

  // Set the current room selection
  teamSelector.value = currentRoom;
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

      let value;
      if (action === "start") {
        value = true;
      } else if (action === "stop") {
        value = false;
      } else {
        // Fallback to old data-value system for backward compatibility
        value = event.target.getAttribute("data-value") === "true";
      }

      updateServerData(path, value);
    });
  });

  // Change the current room when the dropdown is changed
  document
    .getElementById("roomSelector")
    .addEventListener("change", (event) => {
      currentRoom = event.target.value;
      setCookie("room", currentRoom);

      // Fetch the new data after changing the room
      fetchData();
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
  return `${String(h).padStart(2, '0')}:${String(m).padStart(2, '0')}:${String(s).padStart(2, '0')}`;
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
