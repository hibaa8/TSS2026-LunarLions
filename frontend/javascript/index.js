// Global variable for currently selected team
let selectedTeam = 0;
let oldTeam = 0;

// Global variables for UI elements
let startButton = document.getElementById("tssStart");
let stopButton = document.getElementById("tssStop");
let prRunningIndex = -1;
let prRunningTeam = "";

let startPRButton = document.getElementById("prTssStart");
let stopPRButton = document.getElementById("prTssStop");

let uiaButton = document.getElementById("assignUIA");
let uia = document.getElementById("uiaTimerContainer");
let uiaStatus = document.getElementById("uiaStatus");
let uiaBullet = document.getElementById("uiaBulletPoint");
let uiaName = document.getElementById("uiaName");
let uiaFont = document.getElementById("uiaNameFont");

let dcuButton = document.getElementById("assignDCU");
let dcu = document.getElementById("dcuTimerContainer");
let dcuStatus = document.getElementById("dcuStatus");
let dcuBullet = document.getElementById("dcuBulletPoint");
let dcuName = document.getElementById("dcuName");
let dcuFont = document.getElementById("dcuNameFont");

let specButton = document.getElementById("assignSPEC");
let spec = document.getElementById("specTimerContainer");
let specStatus = document.getElementById("specStatus");
let specBullet = document.getElementById("specBulletPoint");
let specName = document.getElementById("specName");
let specFont = document.getElementById("specNameFont");

let xCoordinateEV1 = document.getElementById("xCoordinateEV1");
let yCoordinateEV1 = document.getElementById("yCoordinateEV1");
let headingEV1 = document.getElementById("headingEV1");
let xCoordinateEV2 = document.getElementById("xCoordinateEV2");
let yCoordinateEV2 = document.getElementById("yCoordinateEV2");
let headingEV2 = document.getElementById("headingEV2");
let xCoordinateRover = document.getElementById("xCoordinateRover");
let yCoordinateRover = document.getElementById("yCoordinateRover");

// Configurations for the sensor and switch mappings so that can we easily loop through them

// UIA sensor mappings
const UIA_SENSORS = {
  eva1_power: "eva1PowerSensor",
  eva2_power: "eva2PowerSensor",
  eva1_water_supply: "eva1WaterSupplySensor",
  eva2_water_supply: "eva2WaterSupplySensor",
  eva1_water_waste: "eva1WaterWasteSensor",
  eva2_water_waste: "eva2WaterWasteSensor",
  eva1_oxy: "eva1OxygenSensor",
  eva2_oxy: "eva2OxygenSensor",
  oxy_vent: "oxygenVentSensor",
  depress: "depressPumpSensor",
};

// DCU sensor mappings
const DCU_SENSORS = {
  "eva1.batt": "dcuEva1BatterySensor",
  "eva1.oxy": "dcuEva1OxygenSensor",
  "eva1.comm": "dcuEva1CommSensor",
  "eva1.fan": "dcuEva1FanSensor",
  "eva1.pump": "dcuEva1PumpSensor",
  "eva1.co2": "dcuEva1Co2Sensor",
  "eva2.batt": "dcuEva2BatterySensor",
  "eva2.oxy": "dcuEva2OxygenSensor",
  "eva2.comm": "dcuEva2CommSensor",
  "eva2.fan": "dcuEva2FanSensor",
  "eva2.pump": "dcuEva2PumpSensor",
  "eva2.co2": "dcuEva2Co2Sensor",
};

// Pressurized Rover sensor and switch mappings
const PR_SENSORS_AND_SWITCHES = {
  ac_heating: { sensor: "acHeatingSensor", switch: "acHeatingSwitch" },
  ac_cooling: { sensor: "acCoolingSensor", switch: "acCoolingSwitch" },
  lights_on: { sensor: "lightsOnSensor", switch: "lightsOnSwitch" },
  internal_lights_on: {
    sensor: "internalLightsSensor",
    switch: "internalLightsSwitch",
  },
  brakes: { sensor: "brakesSensor", switch: "brakesSwitch" },
  in_sunlight: { sensor: "inSunlightSensor", switch: "inSunlightSwitch" },
  co2_scrubber: { sensor: "co2ScrubberSensor", switch: "co2ScrubberSwitch" },
  dust_wiper: { sensor: "dustWiperSensor", switch: "dustWiperSwitch" },
  fan_pri: { sensor: "fanPriSensor", switch: "fanPriSwitch" },
  switch_dest: { sensor: "switchDestSensor", switch: "switchDestSwitch" },
};

// Updates team specific data when another team is selected
function swapTeams(newTeam) {
  // Update global team
  selectedTeam = newTeam - 1;

  // Update dropdown selection
  const dropdown = document.getElementById("roomSelect");
  if (dropdown) {
    dropdown.value = newTeam;
  }

  // Reload the team specific json files
  loadEVAStatus(selectedTeam);
  loadEVATelemetry(selectedTeam);

  loadPRStatus(selectedTeam);
  loadPRTelemetryData(selectedTeam);

  // Update button functionality
  // @TODO change these onclick values to just use the selectedTeam variable directly
  document.getElementById("tssStart").value = selectedTeam;
  document.getElementById("tssStop").value = selectedTeam;

  document.getElementById("prTssStart").value = selectedTeam;
  document.getElementById("prTssStop").value = selectedTeam;

  document.getElementById("assignUIA").value = selectedTeam;
  document.getElementById("assignDCU").value = selectedTeam;
  document.getElementById("assignSPEC").value = selectedTeam;

  // Assign page title based on room selected - get from JSON data instead of dropdown
  $.getJSON("../data/TEAMS.json", function (data) {
    const teamNames = [
      data.teams.team_1,
      data.teams.team_2,
      data.teams.team_3,
      data.teams.team_4,
      data.teams.team_5,
      data.teams.team_6,
      data.teams.team_7,
      data.teams.team_8,
      data.teams.team_9,
      data.teams.team_10,
      data.teams.team_11,
    ];
    const teamName = teamNames[selectedTeam] || "------";
    // Team title functionality removed
  });

  // Sets the cookie anytime a new team is selected
  setCookie("roomNum", selectedTeam, 1);
}

// Loads the coordinates of EVA 1 and EVA 2
function loadLocation() {
  $.getJSON("../data/IMU.json", function (data) {
    xCoordinateEV1.innerText = data.imu.eva1.posx.toFixed(2);
    yCoordinateEV1.innerText = data.imu.eva1.posy.toFixed(2);
    headingEV1.innerText = data.imu.eva1.heading.toFixed(2);
    xCoordinateEV2.innerText = data.imu.eva2.posx.toFixed(2);
    yCoordinateEV2.innerText = data.imu.eva2.posy.toFixed(2);
    headingEV2.innerText = data.imu.eva2.heading.toFixed(2);
  });
}

// Sets the cookie to remember the last team selected
function setCookie(cname, cvalue, exdays) {
  const d = new Date();
  d.setTime(d.getTime() + exdays * 24 * 60 * 60 * 1000);
  let expires = "expires=" + d.toUTCString();

  // Sets cookie value for the room num and gives it an expiration date
  document.cookie = cname + "=" + cvalue + ";" + expires + ";path=/";
}

// Gets the cookie for the last team selected
function getCookie(cname) {
  let name = cname + "=";
  let decodedCookie = decodeURIComponent(document.cookie);
  let ca = decodedCookie.split(";");

  // Looks for room num cookie
  for (let i = 0; i < ca.length; i++) {
    let c = ca[i];
    while (c.charAt(0) == " ") {
      c = c.substring(1);
    }
    if (c.indexOf(name) == 0) {
      return Number(c.substring(name.length, c.length));
    }
  }
  return 0;
}

// Loads UIA data and updates all sensor states
function loadUIA() {
  $.getJSON("../data/UIA.json", function (data) {
    // Update all UIA sensors using configuration mapping
    Object.keys(UIA_SENSORS).forEach((key) => {
      updateSensor(UIA_SENSORS[key], data.uia[key]);
    });
  });
}

// Loads DCU data and updates all sensor states
function loadDCU() {
  $.getJSON("../data/DCU.json", function (data) {
    // Update all DCU sensors using configuration mapping
    Object.keys(DCU_SENSORS).forEach((key) => {
      // Handle nested object paths like "eva1.batt"
      const keys = key.split(".");
      let value = data.dcu;
      keys.forEach((k) => (value = value[k]));
      updateSensor(DCU_SENSORS[key], value);
    });
  });
}

/**
 * Loads EVA status, timers, and station assignments for the specified team
 * Updates EVA timer displays and station status indicators
 */
function loadEVAStatus(team) {
  $.getJSON("../data/teams/" + team + "/EVA_STATUS.json", function (data) {
    // Format and display EVA timers using utility function
    document.getElementById("evaTimer").innerText =
      "EVA Time: " + formatTime(data.eva.total_time);
    document.getElementById("uiaTimer").innerText = formatTime(
      data.eva.uia.time
    ).substring(3); // Remove hours for MM:SS
    document.getElementById("specTimer").innerText = formatTime(
      data.eva.spec.time
    ).substring(3); // Remove hours for MM:SS
    document.getElementById("dcuTimer").innerText = formatTime(
      data.eva.dcu.time
    ).substring(3); // Remove hours for MM:SS

    // Button UI States Visuals
    var evaStarted = data.eva.started;
    var evaPaused = data.eva.paused;
    var evaComplete = data.eva.completed;
    if (evaComplete || !evaStarted) {
      stopTSS();
    } else if (evaStarted && evaPaused) {
      pauseTSS();
    } else if (evaStarted && !evaPaused) {
      resumeTSS();
    } else {
      resumeTSS();
    }

    loadLights(team);

    // Stations Status
    updateStationStatus(
      evaStarted,
      data.eva.uia.started,
      data.eva.uia.completed,
      uia,
      uiaStatus,
      uiaButton,
      uiaBullet,
      uiaFont,
      uiaName
    );
    updateStationStatus(
      evaStarted,
      data.eva.dcu.started,
      data.eva.dcu.completed,
      dcu,
      dcuStatus,
      dcuButton,
      dcuBullet,
      dcuFont,
      dcuName
    );
    updateStationStatus(
      evaStarted,
      data.eva.spec.started,
      data.eva.spec.completed,
      spec,
      specStatus,
      specButton,
      specBullet,
      specFont,
      specName
    );
  });
}

/**
 * Loads Pressurized Rover status and timer for the specified team
 * Updates PR timer display and button states
 */
function loadPRStatus(team) {
  $.getJSON("../data/teams/" + team + "/ROVER_TELEMETRY.json", function (data) {
    // Format and display PR timer using utility function
    document.getElementById("prTimer").innerText =
      "PR Time: " + formatTime(data.pr_telemetry.mission_elapsed_time);

    // Button UI States Visuals
    var prStarted = data.pr_telemetry.sim_running;
    var prPaused = data.pr_telemetry.sim_paused;
    var prComplete = data.pr_telemetry.sim_completed;

    if (prRunningIndex >= 0 && prRunningIndex != team) {
      document.getElementById("prButtons").style.display = "none";
      document.getElementById("prTimer").style.display = "none";
      document.getElementById("prAlreadyRunningText").style.display = "flex";
      document.getElementById("prAlreadyRunningText").style.justifyContent =
        "center";
      document.getElementById("prAlreadyRunningText").style.alignItems =
        "center";
      document.getElementById("prAlreadyRunningText").innerText =
        "PR Sim already running for team " + prRunningTeam;
    } else {
      document.getElementById("prButtons").style.display = "contents";
      if (prStarted)
        document.getElementById("prTimer").style.display = "contents";
      document.getElementById("prAlreadyRunningText").style.display = "none";
    }

    if (prComplete) {
      prRunningIndex = -1;
      stopPRTSS();
    } else if (prPaused) {
      pausePRTSS();
    } else if (prStarted) {
      prRunningIndex = team;
      prRunningTeam = document.getElementById(
        "room" + (team + 1) + "Name"
      ).innerText;
      resumePRTSS();
    }
  });
}

/**
 * Loads EVA telemetry data and updates all telemetry fields for both EVAs
 * Displays oxygen levels, pressures, consumption rates, and health metrics
 */
function loadEVATelemetry(team) {
  $.getJSON("../data/teams/" + team + "/EVA_TELEMETRY.json", function (data) {
    // Update shared EVA time for both EVAs
    const evaTime = formatTime(Number(data.telemetry.eva_time));
    document.getElementById("evaTimeTelemetryState1").innerText = evaTime;
    document.getElementById("evaTimeTelemetryState2").innerText = evaTime;

    // Update EVA 1 telemetry
    updateEVATelemetry(data.telemetry.eva1, "1");

    // Update EVA 2 telemetry
    updateEVATelemetry(data.telemetry.eva2, "2");
  });
}

// Loads title for team specific data depending on which team is selected
function loadTitle(oldTeam) {
  $.getJSON("../data/TEAMS.json", function (data) {
    var teamnames = Object.values(data.teams);
    // Team title functionality removed
  });
}

// Loads team names for dropdown
function loadTeams() {
  $.getJSON("../data/TEAMS.json", function (data) {
    const dropdown = document.getElementById("roomSelect");
    const teamNames = [
      data.teams.team_1,
      data.teams.team_2,
      data.teams.team_3,
      data.teams.team_4,
      data.teams.team_5,
      data.teams.team_6,
      data.teams.team_7,
      data.teams.team_8,
      data.teams.team_9,
      data.teams.team_10,
      data.teams.team_11,
    ];

    // Update dropdown options with team names
    for (let i = 0; i < teamNames.length; i++) {
      dropdown.options[i].text = `Room ${i + 1} - ${teamNames[i]}`;
    }

    prRunningIndex = -1;
    prRunningTeam = "";
    for (var i = 0; i < data.teams.team_num; i++) {
      loadLights(i);
    }

    if (prRunningIndex >= 0) {
      prRunningTeam = teamNames[prRunningIndex];
    }
  });
}

// Loads status lights for room
function loadLights(team) {
  $.getJSON("../data/teams/" + team + "/EVA_STATUS.json", function (data) {
    $.getJSON(
      "../data/teams/" + team + "/ROVER_TELEMETRY.json",
      function (pr_data) {
        // Button UI States Visuals
        var evaStarted = data.eva.started || pr_data.pr_telemetry.sim_running;
        var evaPaused =
          (data.eva.started && data.eva.paused) ||
          (pr_data.pr_telemetry.sim_running && pr_data.pr_telemetry.sim_paused);
        var evaComplete = data.eva.completed || pr_data.pr_telemetry.completed;

        if (
          pr_data.pr_telemetry.sim_running ||
          pr_data.pr_telemetry.sim_paused
        ) {
          prRunningIndex = team;
        }

        // Update dropdown option styling based on team status
        const dropdown = document.getElementById("roomSelect");
        const option = dropdown.options[team];
        if (option) {
          if (
            (evaStarted && !evaComplete) ||
            pr_data.pr_telemetry.sim_running
          ) {
            option.style.color = "rgba(0, 240, 10, 1)"; // Green for active
          } else if (evaComplete) {
            option.style.color = "rgba(0, 0, 255, 1)"; // Blue for completed
          } else {
            option.style.color = "rgba(255, 255, 255, 1)"; // White for inactive
          }
        }
      }
    );
  });
}

/**
 * Loads Pressurized Rover telemetry data and updates sensors, switches, and telemetry displays
 * Updates rover positioning, environmental data, and system status indicators
 */
function loadPRTelemetryData(team) {
  $.getJSON("../data/teams/" + team + "/ROVER_TELEMETRY.json", function (data) {
    // Update all PR sensors and switches using configuration mapping
    Object.keys(PR_SENSORS_AND_SWITCHES).forEach((key) => {
      const config = PR_SENSORS_AND_SWITCHES[key];
      updateSensorAndSwitch(
        config.sensor,
        config.switch,
        data.pr_telemetry[key]
      );
    });

    // Convert power consumption percentages to kWh
    const total_battery_capacity = 4320000; // Electric car capacity in Joules
    const power_consumption_rate =
      ((total_battery_capacity * data.pr_telemetry.power_consumption_rate) /
        100 /
        1000) *
      3600;
    const motor_power_consumption =
      ((total_battery_capacity * data.pr_telemetry.motor_power_consumption) /
        100 /
        1000) *
      3600;

    // Update PR positioning and movement data
    updateTelemetryField("throttle", data.pr_telemetry.throttle, "%");
    updateTelemetryField("steering", data.pr_telemetry.steering);
    updateTelemetryField("current_pos_x", data.pr_telemetry.current_pos_x);
    updateTelemetryField("current_pos_y", data.pr_telemetry.current_pos_y);
    updateTelemetryField("current_pos_alt", data.pr_telemetry.current_pos_alt);
    updateTelemetryField("heading", data.pr_telemetry.heading, "°");
    updateTelemetryField("roll", data.pr_telemetry.roll, "°");
    updateTelemetryField("pitch", data.pr_telemetry.pitch, "°");
    updateTelemetryField(
      "distance_traveled",
      data.pr_telemetry.distance_traveled,
      "m"
    );
    updateTelemetryField("speed", data.pr_telemetry.speed, "m/s");
    updateTelemetryField(
      "surface_incline",
      data.pr_telemetry.surface_incline,
      "°"
    );

    // Update environmental and system data
    updateTelemetryField(
      "oxygen_pressure",
      data.pr_telemetry.oxygen_pressure,
      "psi"
    );
    updateTelemetryField("oxygen_levels", data.pr_telemetry.oxygen_levels, "%");
    updateTelemetryField("oxygen_tank", data.pr_telemetry.oxygen_tank, "%");
    updateTelemetryField(
      "solar_panel_dust_accum",
      data.pr_telemetry.solar_panel_dust_accum,
      "%"
    );
    updateTelemetryField("battery_level", data.pr_telemetry.battery_level, "%");
    updateTelemetryField("ac_fan_pri", data.pr_telemetry.ac_fan_pri, "rpm");
    updateTelemetryField("ac_fan_sec", data.pr_telemetry.ac_fan_sec, "rpm");
    updateTelemetryField(
      "cabin_pressure",
      data.pr_telemetry.cabin_pressure,
      "psi"
    );
    updateTelemetryField(
      "cabin_temperature",
      data.pr_telemetry.cabin_temperature,
      "°C"
    );
    updateTelemetryField(
      "power_consumption_rate",
      power_consumption_rate,
      "kWh"
    );
    updateTelemetryField(
      "solar_panel_efficiency",
      data.pr_telemetry.solar_panel_efficiency
    );
    updateTelemetryField(
      "external_temp",
      data.pr_telemetry.external_temp,
      "°C"
    );
    updateTelemetryField(
      "pr_coolant_level",
      data.pr_telemetry.pr_coolant_level,
      "L"
    );
    updateTelemetryField(
      "pr_coolant_pressure",
      data.pr_telemetry.pr_coolant_pressure,
      "psi"
    );
    updateTelemetryField(
      "pr_coolant_tank",
      data.pr_telemetry.pr_coolant_tank,
      "%"
    );
    updateTelemetryField(
      "motor_power_consumption",
      motor_power_consumption,
      "kWh"
    );
    updateTelemetryField(
      "terrain_condition",
      data.pr_telemetry.terrain_condition
    );
    updateTelemetryField(
      "mission_elapsed_time",
      data.pr_telemetry.mission_elapsed_time
    );
    updateTelemetryField(
      "mission_planned_time",
      data.pr_telemetry.mission_planned_time
    );
    updateTelemetryField(
      "point_of_no_return",
      data.pr_telemetry.point_of_no_return
    );
    updateTelemetryField(
      "distance_from_base",
      data.pr_telemetry.distance_from_base,
      "m"
    );
  });
}

// Updates telemetry for each team
function updateTelemetry() {
  // Update button functionality
  document.getElementById("tssStart").value = selectedTeam;
  document.getElementById("tssStop").value = selectedTeam;

  document.getElementById("prTssStart").value = selectedTeam;
  document.getElementById("prTssStop").value = selectedTeam;

  document.getElementById("assignUIA").value = selectedTeam;
  document.getElementById("assignDCU").value = selectedTeam;
  document.getElementById("assignSPEC").value = selectedTeam;
  // assignROV removed - element doesn't exist in HTML
}

// Runs on load of the page
function onload() {
  // Init all the variables
  startButton = document.getElementById("tssStart");
  stopButton = document.getElementById("tssStop");

  startPRButton = document.getElementById("prTssStart");
  stopPRButton = document.getElementById("prTssStop");

  uiaButton = document.getElementById("assignUIA");
  uia = document.getElementById("uiaTimerContainer");
  uiaStatus = document.getElementById("uiaStatus");
  uiaBullet = document.getElementById("uiaBulletPoint");
  uiaName = document.getElementById("uiaName");
  uiaFont = document.getElementById("uiaNameFont");

  dcuButton = document.getElementById("assignDCU");
  dcu = document.getElementById("dcuTimerContainer");
  dcuStatus = document.getElementById("dcuStatus");
  dcuBullet = document.getElementById("dcuBulletPoint");
  dcuName = document.getElementById("dcuName");
  dcuFont = document.getElementById("dcuNameFont");

  specButton = document.getElementById("assignSPEC");
  spec = document.getElementById("specTimerContainer");
  specStatus = document.getElementById("specStatus");
  specBullet = document.getElementById("specBulletPoint");
  specName = document.getElementById("specName");
  specFont = document.getElementById("specNameFont");

  // ROV elements removed - they don't exist in HTML
  // rovButton = document.getElementById("assignROV");
  // rov = document.getElementById("rovTimerContainer");
  // rovStatus = document.getElementById("rovStatus");
  // rovBullet = document.getElementById("rovBulletPoint");
  // rovName = document.getElementById("rovName");
  // rovFont = document.getElementById("rovNameFont");

  // videoFeed element removed - it doesn't exist in HTML
  // video = document.getElementById("videoFeed");

  xCoordinateEV1 = document.getElementById("xCoordinateEV1");
  yCoordinateEV1 = document.getElementById("yCoordinateEV1");
  headingEV1 = document.getElementById("headingEV1");
  xCoordinateEV2 = document.getElementById("xCoordinateEV2");
  yCoordinateEV2 = document.getElementById("yCoordinateEV2");
  headingEV2 = document.getElementById("headingEV2");
  xCoordinateRover = document.getElementById("xCoordinateRover");
  yCoordinateRover = document.getElementById("yCoordinateRover");

  // Grabs cookie to update current room selected
  oldTeam = getCookie("roomNum");
  console.log("selected team is " + oldTeam);
  loadTitle(oldTeam);

  // Set initial dropdown selection
  const dropdown = document.getElementById("roomSelect");
  if (dropdown) {
    dropdown.value = oldTeam + 1;
    console.log("Set dropdown to:", oldTeam + 1);
  }

  selectedTeam = oldTeam;

  // Places team names into the front end
  loadTeams();

  //Load immediately
  loadUIA();
  loadDCU();
  loadEVAStatus(selectedTeam);
  loadPRStatus(selectedTeam);
  loadEVATelemetry(selectedTeam);
  loadPRTelemetryData(selectedTeam);
  loadLocation();
  updateTelemetry();

  // Continuously refreshes values from the UIA, DCU, EVA, and Telemetry
  setInterval(function () {
    loadUIA();
    loadDCU();
    loadEVAStatus(selectedTeam);
    loadPRStatus(selectedTeam);
    loadEVATelemetry(selectedTeam);
    loadPRTelemetryData(selectedTeam);
    loadLocation();
  }, 1000);
}

// Called when a new room is selected in dropdown
function roomSelect(inputVal) {
  // Convert string to number if needed
  const roomNumber = parseInt(inputVal);
  console.log("Room selected:", roomNumber);

  // minus 1 handles the zero indexing of the team number folders
  swapTeams(roomNumber);
}

// Updates station frontend
function updateStationStatus(
  evaStarted,
  started,
  complete,
  Station,
  StationStatus,
  StationButton,
  StationBullet,
  StationFont,
  StationName
) {
  // Updates when not started
  if (!evaStarted) {
    StationButton.textContent = "ASSIGN";
    StationButton.name = "";
    StationStatus.style.color = "rgba(150, 150, 150, 1)";
    StationStatus.textContent = "Incomplete";
    Station.style.display = "none";

    var elem = document.getElementsByClassName("station-btn");
    for (var i = 0; i < elem.length; i++) {
      elem[i].style.display = "none";
    }

    StationBullet.style.display = "initial";
    StationBullet.style.backgroundColor = "rgba(150, 150, 150, 1)";
    StationFont.style.color = "rgba(150, 150, 150, 1)";
    StationName.style.backgroundColor = "rgba(100, 100, 100, 1)";
  }

  // Updates when running but not paused
  else if (!started && !complete) {
    StationButton.textContent = "ASSIGN";
    StationButton.name = "eva_start_" + StationName.innerText + "_team";
    StationStatus.style.color = "rgba(150, 150, 150, 1)";
    StationStatus.textContent = "Incomplete";
    Station.style.display = "none";

    var elem = document.getElementsByClassName("station-btn");
    for (var i = 0; i < elem.length; i++) {
      elem[i].style.display = "initial";
    }

    StationBullet.style.display = "initial";
    StationBullet.style.backgroundColor = "rgba(150, 150, 150, 1)";
    StationFont.style.color = "rgba(150, 150, 150, 1)";
    StationName.style.backgroundColor = "rgba(100, 100, 100, 1)";
  }

  // Updates when running but not resumed
  else if (started && !complete) {
    StationButton.textContent = "UNASSIGN";
    StationButton.name = "eva_end_" + StationName.innerText + "_team";
    StationStatus.style.color = getComputedStyle(document.documentElement).getPropertyValue('--yellow');
    StationStatus.textContent = "Current";
    Station.style.display = "initial";

    var elem = document.getElementsByClassName("station-btn");
    for (var i = 0; i < elem.length; i++) {
      elem[i].style.display = "initial";
    }

    StationBullet.style.display = "initial";
    StationBullet.style.backgroundColor = getComputedStyle(document.documentElement).getPropertyValue('--yellow');
    StationFont.style.color = "rgba(255, 255, 255, 1)";
    StationName.style.backgroundColor = "rgba(0, 0, 255, 1)";
  }

  // Updates when completed
  else if (complete) {
    StationButton.textContent = "ASSIGN";
    StationButton.name = "eva_end_" + StationName.innerText + "_team";
    StationStatus.style.color = "rgba(0, 240, 10, 1)";
    StationStatus.textContent = "Completed";
    Station.style.display = "initial";

    StationButton.style.display = "none";

    StationBullet.style.display = "none";
    StationBullet.style.backgroundColor = getComputedStyle(document.documentElement).getPropertyValue('--yellow');
    StationFont.style.color = "rgba(255, 255, 255, 1)";
    StationName.style.backgroundColor = "rgba(0, 0, 255, 1)";
  }
}

// Updates Telemetry frontend when TSS is paused
function pauseTSS() {
  startButton.name = "eva_unpause_team";
  startButton.style.backgroundColor = getComputedStyle(document.documentElement).getPropertyValue('--yellow');
  startButton.textContent = "RESUME";
  stopButton.style.backgroundColor = getComputedStyle(document.documentElement).getPropertyValue('--button-red');
  document.getElementById("evaTimer").style.display = "contents";

  var array = new Array(
    uiaStatus,
    specStatus,
    dcuStatus,
    uia,
    spec,
    dcu,
    uiaButton,
    specButton,
    dcuButton
  );
  for (var i = 0; i < 3; i++) {
    if (array[i].textContent == "Completed") {
      array[i + 3].style.display = "initial";
      array[i + 6].style.display = "none";
    } else {
      array[i + 3].style.display = "none";
      array[i + 6].style.display = "none";
    }
  }
}

// Updates Telemetry frontend when TSS is resumed
function resumeTSS() {
  startButton.style.backgroundColor = getComputedStyle(document.documentElement).getPropertyValue('--button-grey');
  startButton.textContent = "PAUSE";
  stopButton.style.backgroundColor = getComputedStyle(document.documentElement).getPropertyValue('--button-red');

  startButton.name = "eva_pause_team";
  startButton.style.backgroundColor = getComputedStyle(document.documentElement).getPropertyValue('--button-grey');
  startButton.textContent = "PAUSE";
  stopButton.style.backgroundColor = getComputedStyle(document.documentElement).getPropertyValue('--button-red');
  document.getElementById("evaTimer").style.display = "contents";

  var array = new Array(
    uiaStatus,
    specStatus,
    dcuStatus,
    uia,
    spec,
    dcu,
    uiaButton,
    specButton,
    dcuButton
  );
  for (var i = 0; i < 3; i++) {
    if (array[i].textContent == "Completed") {
      array[i + 3].style.display = "initial";
      array[i + 6].style.display = "none";
    } else if (array[i].textContent == "Current") {
      array[i + 3].style.display = "initial";
      array[i + 6].style.display = "initial";
    } else if (array[i].textContent == "Incomplete") {
      array[i + 3].style.display = "none";
      array[i + 6].style.display = "initial";
    }
  }
}

// Updates Telemetry frontend when TSS is stopped
function stopTSS() {
  startButton.name = "eva_start_team";
  startButton.style.backgroundColor = getComputedStyle(document.documentElement).getPropertyValue('--green');
  startButton.textContent = "START";
  stopButton.style.backgroundColor = getComputedStyle(document.documentElement).getPropertyValue('--button-grey');

  var elem = document.getElementsByClassName("station-btn");
  for (var i = 0; i < elem.length; i++) {
    elem[i].style.display = "none";
  }

  document.getElementById("evaTimer").style.display = "none";
}

// Updates Telemetry frontend when TSS is paused
function pausePRTSS() {
  startPRButton.name = "pr_unpause_team";
  startPRButton.style.backgroundColor = getComputedStyle(document.documentElement).getPropertyValue('--yellow');
  startPRButton.textContent = "RESUME";
  stopPRButton.style.backgroundColor = getComputedStyle(document.documentElement).getPropertyValue('--button-red');
  stopPRButton.disabled = false;
  document.getElementById("prTimer").style.display = "contents";
}

// Updates Telemetry frontend when TSS is resumed
function resumePRTSS() {
  startPRButton.style.backgroundColor = getComputedStyle(document.documentElement).getPropertyValue('--button-grey');
  startPRButton.textContent = "PAUSE";
  stopPRButton.style.backgroundColor = getComputedStyle(document.documentElement).getPropertyValue('--button-red');

  startPRButton.name = "pr_pause_team";
  startPRButton.style.backgroundColor = getComputedStyle(document.documentElement).getPropertyValue('--button-grey');
  startPRButton.textContent = "PAUSE";
  stopPRButton.style.backgroundColor = getComputedStyle(document.documentElement).getPropertyValue('--button-red');
  stopPRButton.disabled = false;
  document.getElementById("prTimer").style.display = "contents";
}

// Updates Telemetry frontend when TSS is stopped
function stopPRTSS() {
  startPRButton.name = "pr_start_team";
  startPRButton.style.backgroundColor = getComputedStyle(document.documentElement).getPropertyValue('--green');
  startPRButton.textContent = "START";
  stopPRButton.style.backgroundColor = getComputedStyle(document.documentElement).getPropertyValue('--button-grey');
  stopPRButton.disabled = true;
  document.getElementById("prTimer").style.display = "none";
}
