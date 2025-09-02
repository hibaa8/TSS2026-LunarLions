
// global variable for UI elements
let video = document.getElementById('videoFeed');
let map = document.getElementById('rockYardMap');
let poi1x = document.getElementById('poi1x');
let poi2x = document.getElementById('poi2x');
let poi3x = document.getElementById('poi3x');
let poi1y = document.getElementById('poi1y');
let poi2y = document.getElementById('poi2y');
let poi3y = document.getElementById('poi3y');

// Loads the rover video feed
function loadVideoFeed() {
    video.src = "http://192.168.51.163:8080/stream?topic=/camera/image_raw&type=ros_compressed";
}

// Loads the rock yard maps
function loadMap() {
    map.src = "./images/rock-yard-map.png";
}

// Loads the rock yard maps
function loadMap() {
    map.src = "./images/rock-yard-map.png";
}

//Runs on load of the page
function onload() {

    // Init all the variables
    video = document.getElementById('videoFeed');
    map = document.getElementById('rockYardMap');

    loadMap();

    // Continuously refreshes for the video feed
    setInterval(function() {
        loadVideoFeed();
    }, 1000);
}