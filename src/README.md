# TSS Development Documentation

This document is intended to provide an overview of the current telemetry stream server repository, allowing new developers or interested students to understand the structure and how to make changes/contribute to the telemetry stream server.

![](/frontend/images/tss-structure.png)
_NASA SUITS block diagram of all peripherals and software_

## Introduction

### Navigation

- <a href="#frontend-development">Frontend Development</a>
- <a href="#server-development">Server Development</a>
- <a href="#telemetry-simulation-development">Telemetry Simulation Development</a>
- <a href="#peripheral-devices">Peripheral Devices</a>

### Background

The NASA SUITS project has taken on several iterations since its start in 2018. This telemetry server marks a significant rewrite from the previous iteration of [TSS 2025](https://github.com/SUITS-Techteam/TSS-2025). We identified several key improvement points including a new interface, better simulated telemetry values, and other features that we wanted to focus on.

TSS was developed for the NASA SUITS challenge with the goal of providing a reliable stream of data to participating teams to use within their interfaces and hardware designs. This started out with several basic linear decay and growth algorithms to model basic resource consumption, but has since been expanded to accommodate complex relationships between multiple telemetry values (e.g. EVA location values calculating speed, increasing heart rate). These complex telemetry relationships help to create a more realistic scenario for students to develop with.

In the previous SUITS iteration, we added a new segment to the challenge to highlight the need of interfaces for a pressurized rover. Unlike the EVA, where teams could take advantage of AR hardware in the JSC Rock Yard, we knew that we could not use a physical rover as an asset during test week. This led to using [DUST](https://software.nasa.gov/software/MSC-27522-1), a simulation developed at NASA of the Lunar South Pole built in Unreal Engine.

TSS now acts as a hub for communications with both teams and external assets, with all communications being proxied through the server. For example, if a team wants to control the pressurized rover in DUST, they will issue a command over UDP to the server, TSS will then send a response confirming the change and then relay that change to the DUST instance to change the throttle value of the rover. Proxying all of these requests allows us to maintain a level of consistency across the entire peripheral stack. Please reference the block diagram above to get a better idea of how these devices communicate with each other.

### Structure

The project is separated into several folders:

- `/data`: contains the JSON files that are read/written to for telemetry storage
- `/documents`: contains supporting documentation, images, etc. for students to use during development
- `/frontend`: all frontend assets, primarily HTML, JS, and CSS
- `/scripts`: supporting Python scripts to allow users to test various parts of their implementation
- `/src`: all C code that runs the server infrastructure and simulation engine

## Frontend Development

![Image of the user interface of the main page of TSS](../frontend/images/tss-main-page.png)
_Primary interface for interacting with TSS_

The frontend for TSS was developed in an extremely simple stack using HTML, CSS, and JavaScript. Through several iterations of the server, we have opted to keep the majority of the backend written in C. This meant that using more modern languages or libraries (e.g. React) would significantly complicate the development of the frontend.

This year, we have gone through and consolidated all of the separate pages from previous designs and brought them all into one singular frontend. Each section has it's own "panel", and we have separated the unique segments of the challenge (EVA vs rover) into two separate columns to maintain a differentiator in the data being displayed.

### UI elements

The frontend is built with a minimal interface and reusable styling. All of the style code is consolidated into a singular stylesheet file that is titled <a href="../frontend/index.css">index.css</a>. The styling has slowly morphed into a large file than was originally planned and could likely be due for refactoring and simplification.

The primary color scheme is:

- Black (background): `#000000ff`
- Blue (title card background): `#06212cff`
- Light Blue (borders): `#3889abff`
- Green (buttons): `#28ae5f`
- Dark Gray (buttons): `#2d2d2d`
- Light Gray (borders and switches): `#8e8e8eff`

### Data syncing

Throughout the infrastructure, you'll note that we primarily use UDP sockets to communicate back and forth to make data changes and issue commands. The frontend is the one section of the tech stack that we have opted to still use HTTPS for data fetching. The server still serves the JSON files over HTTPS, and can be fetched with a relative URL such as: `/data/ROVER.json`.

The frontend JavaScript fetches the three JSON files: `ROVER.json`, `EVA.json`, and `LTV.json` every second. A snippet of that code is referenced here:

```js
// Fetch fresh data from the backend every one second
setInterval(() => {
  fetchData();
  updateClock();
  updateConnectionStatus();
}, 1000);
```

After fetching the data from the backend, we still need to take the fresh data and display it on the interface. This is done with a nifty setup that allows us to add new telemetry values and elements to the HTML without creating repetitive code in JavaScript. You'll note that every value being updated has a HTML attribute labeled `data-path`, here is an example below:

```html
<div class="telemetry-value">
  <span
    class="telemetry-data"
    data-path="eva.telemetry.eva1.scrubber_a_co2_storage"
    data-units="%"
  >
    ------
  </span>
</div>
```

The data path, in this instance `eva.telemetry.eva1.scrubber_a_co2_storage`, directly corresponds to a field in a resulting JSON file. For example this data path would be for the `EVA.json` file and uses a period as a delimeter to denote a field name. As a result, we could expect to find the value to update this HTML element in a JSON file structured like below:

`EVA.json`

```json
{
  "telemetry": {
    "eva1": {
      "scrubber_a_co2_storage": "new value!"
    }
  }
}
```

Here is a snippet from the <a href="../frontend/index.js">index.js</a> file that quickly identifies all of the frontend elements that need to be updated with new telemetry values.

```js
const elements = document.querySelectorAll("[data-path]");
elements.forEach((el) => {
  // Process fetching the correct field from the JSON file and updating the HTML element's value
});
```

## Server Development

### UDP sockets

The server was converted to communicate primarily over the UDP protocol for the 24-25 challenge, after having bad experiences with several teams sending a high number of HTTP requests that brought the small server hosted on the local network down during test week. Moving to the UDP protocol helped remedy those issues and created an additional unique challenge for teams to develop with a networking protocol that they don't typically encounter.

The user datagram protocol is connectionless, which means that it can be much faster than TCP but can also be less reliable since there is no confirmation of delivery. The diagram below illustrates how the UDP protocol is used in C. If you are interested in learning more, I highly recommend this article by Cloudflare: [Everything you ever wanted to know about UDP sockets but were afraid to ask](https://blog.cloudflare.com/everything-you-ever-wanted-to-know-about-udp-sockets-but-were-afraid-to-ask-part-1/).

![](https://www.cs.dartmouth.edu/~campbell/cs60/UDPsockets.jpg)

_Image Credit: <a href="https://www.cs.dartmouth.edu/~campbell/cs60/socketprogramming.html">Dartmouth Computer Science</a>_

### Server structure

- `data.c`: Majority of the data handling, processing UDP requests, routing to JSON files, other data helpers, etc
- `network.c`: Core networking functionality, creating socket connections, etc
- `server.c`: Sets up the frontend HTTP server, UDP sockets, sim engine, and other helper functions to communicate with DUST and peripherals.

### Data handling

Requests to change a value can be done over HTTP (from the frontend) or via UDP (peripherals, student devices, etc). In both cases, they are eventually converted into a string format that represents a file name and field path to update the resulting JSON field with a new value. For example, if someone flips the EVA 1 power switch on the physical UIA, it will send a UDP packet to the server with the command number `2003`, this command number will be converted to a data path based on the hard coded table found in <a href="/src/data.h">data.h: udp_command_mappings</a>, in this case that would be `eva.uia.eva1_power`. This is a very similar mechanism done in reverse to the frontend data update code highlighted above.

### DUST connection

Since TSS is a proxy for commands to the DUST pressurized rover simulation, we need to constantly transmit rover values to the Unreal Engine. This is done first by having the DUST instance connect to the same server address, after which it will send a UDP packet to the server with the command number 3000 indicating that it wants to register with TSS. The server will then process this request and save the IP address of the DUST instance so that it can send UDP packets back to DUST (see the function tss_to_unreal in `server.c`) to change the throttle, brakes, etc. A snippet of that code from `server.c` is included below:

```c
} else if (command == 3000) {  // Unreal Engine registration (DUST simulation)

// Set the Unreal Engine IP address so that can forward commands like brakes and throttle to the simulation
unreal_addr = client->udp_addr;
unreal_addr_len = client->address_length;
unreal = true;

printf("Unreal address set to %s:%d\n", inet_ntoa(client->udp_addr.sin_addr),
ntohs(client->udp_addr.sin_port));

drop_udp_client(&udp_clients, client);
```

## Telemetry Simulation Development

### Overview

A simulation engine was developed in C to help create a realistic scenario that incorporated the live telemetry values from the rover and EVA (primarily location and speed), and create complex relationships with various other biometrics and environmental values.

The code for the library can be found in <a href="/src/lib/simulation">src/lib/simulation</a>. It is divided into two main files, `sim_algorithms.c` which specifies the supported algorithms for the simulation, and `sim_engine.c` which includes all of the helper functions and main code to initiate a simulation (this is called upon within the backend server to create a new simulation instance).

The supported algorithms are:

- Linear Growth
- Linear Decay
- Sine Wave
- Dependent Value (custom algorithm)
- External Value

The first three are quite basic and used for extremely simple time based telemetry calculations. What makes this powerful is the use of the final two types of algorithms, which can be used to pull in external values (e.g. the location of an EVA which is a real world value and not simulated), and custom equations that are dependent on both simuilated values and external values.

### Configuration

Instead of hardcoding every field that we wanted to simulate, we opted to create a format that was easily configurable. This took the form of JSON files that live within a <a href="/src/lib/simulation/config">config folder</a> in the root of the simulation library folder. Each file is representative of a single "component" that you want to simulate; in our case that would be `rover`, `eva1`, and `eva2`. Below is an example of a config file with some of the supported algorithms mentioned above:

```json
{
  "component_name": "rover",
  "fields": {
    "cabin_heating": {
      "type": "float",
      "algorithm": "external_value",
      "file_path": "ROVER.json",
      "field_path": "pr_telemetry.cabin_heating"
    },
    "cabin_temperature": {
      "type": "float",
      "algorithm": "dependent_value",
      "formula": "external_temp * 0.15 + sunlight * 8.0 + cabin_heating * 15.0 - cabin_cooling * 10.0 + 20.0",
      "depends_on": [
        "external_temp",
        "sunlight",
        "cabin_heating",
        "cabin_cooling"
      ]
    },
    "coolant_pressure": {
      "type": "float",
      "algorithm": "sine_wave",
      "base_value": 500.0,
      "amplitude": 2.0,
      "frequency": 0.01
    },
    "rover_elapsed_time": {
      "type": "float",
      "algorithm": "linear_growth",
      "start_value": 0,
      "end_value": 1350,
      "duration_seconds": 1350.0
    }
  }
}
```

## Peripheral Devices

The peripheral devices used during test week communicate with TSS over the UDP protocol. The code for these devices are not available publicly.
