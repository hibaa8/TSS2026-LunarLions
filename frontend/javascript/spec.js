/**
 * Toggles the EVA 1 dropdown visibility
 */
function dropdownEVA1() {
  document.getElementById("dropdownEV1").classList.toggle("show");
}

/**
 * Toggles the EVA 2 dropdown visibility  
 */
function dropdownEVA2() {
  document.getElementById("dropdownEV2").classList.toggle("show");
}

/**
 * SPEC PAGE JAVASCRIPT - Handles spectroscopy data loading and display
 * Utility functions are now loaded from utils.js
 */

/**
 * Configuration mapping for SPEC data fields to UI elements
 */
const SPEC_FIELDS = ['SiO2', 'TiO2', 'Al2O3', 'FeO', 'MnO', 'MgO', 'CaO', 'K2O', 'P2O3', 'other'];

/**
 * Loads SPEC data and updates both EVA displays
 * Uses shared updateEvaSpecData function from utils.js
 */
function loadSPEC() {
  $.getJSON("data/SPEC.json", function (data) {
    // Update EVA 1 SPEC data using shared utility function
    updateEvaSpecData(data.spec.eva1, "1", SPEC_FIELDS);
    
    // Update EVA 2 SPEC data using shared utility function
    updateEvaSpecData(data.spec.eva2, "2", SPEC_FIELDS);
  });
}

/**
 * Initializes the SPEC page on load
 * Loads initial data and sets up continuous refresh interval
 */
function onload() {
  // Load SPEC data immediately
  loadSPEC();

  // Continuously refresh SPEC values from both EVAs every second
  setInterval(function () {
    loadSPEC();
  }, 1000);
}
