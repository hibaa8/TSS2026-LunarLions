# TSS 2026

NASA Spacesuit User Interface Technologies for Students ([NASA SUITS](https://www.nasa.gov/learning-resources/spacesuit-user-interface-technologies-for-students/)) is a design challenge in which college students from across the country help design user interface solutions for future spaceflight needs.
The following is a web interface for the SUITS telemetry stream server designed and developed for the challenge.

<div style="display: flex; flex-direction: row; width: fit-content; gap: 10px;">
    <img src="https://www.nasa.gov/wp-content/uploads/2023/02/52112919543-3eff64ea32-k.jpg" alt="Image of a person wearing glasses with an augmented display at night" height="300px"/>
    <div style="display: flex; flex-direction: column; gap: 10px;">
        <img src="https://www.nasa.gov/wp-content/uploads/2023/01/1.png" height="140px"/>
        <img src="https://www.nasa.gov/wp-content/uploads/2023/02/jsc2023e029847-small.jpg" height="140px"/>
    </div>
</div>

## Introduction

### Navigation
- <a href="#getting-started">Getting Started</a>
- <a href="#peripheral-devices">Peripheral Devices</a>
- <a href="#dust-simulator">DUST Simulator</a>
- <a href="#development">Development</a>
- <a href="#testing">Testing</a>

### What is TSS?

TSS stands for telemetry stream server. It is the centralized server for sending and recieving data for the challenge. All data from the lunar simulator DUST is sent to TSS, and any commands to control the pressurized rover, or fetch data will be sent to TSS. The following document will detail how you can run your own instance of the server and begin developing your software and hardware.

### Placeholder

## Getting Started


1. Clone the repository:

`
git clone https://github.com/SUITS-Techteam/TSS-2025.git
`

2. Navigate into the root of the repository on your terminal of choice

3. To build the TSS Server, run:

`
./build.bat` **NOTE:** If running on Windows, you will have to setup [WSL](https://learn.microsoft.com/en-us/windows/wsl/about) and install GCC. If you don't, several errors will be displayed when trying to build the server.

4. To run the TSS Server, run:

`
./server.exe
`

You should see the following lines appear...

```
Launching Server at IP: 172.20.182.43:14141
Configuring Local Address...
Creating HTTP Socket...
Binding HTTP Socket...
Listening to HTTP Socket...
Creating UDP Socket...
Binding UDP Socket...
Listening to UDP Socket...
Backend and simulation engine initialized successfully
```

5. Type the IP address printed in the first output for "Launching Server at IP: xxx.xx.xxx.xx:14141". This will open the website for the TSS server.

6. From this website, you can interact with the server. This is where you can monitor the state of the simulation, verify the display of your system, and virtually interact with the EVA devices like you will be doing in May.

![Image of the user interface of the main page of TSS](frontend/images/tss-main-page.png)

## Peripheral Devices
The devices listed below are physical devices that will be used during test week to create a realistic scenario for both the EVA and rover teams. The sensor data listed below will be synced with the telemetry server and can be fetched for use within your interface.

### UIA
The umbilical interface assembly (UIA) is a component used at the beginning of an EVA to transfer power and fluids to the suit. A picture of the UIA you will be using during test week can be found [here](test)

| Sensor       | Value True | Value False | Description                        |
| ------------ | ---------- | ----------- | ---------------------------------- |
| EMU1 POWER   | ON         | OFF         | Remotely powers the suit for EVA 1 |
| EV1 SUPPLY   | OPEN       | CLOSED      | Fills EVA 1's liquid coolant       |
| EV1 WASTE    | OPEN       | CLOSED      | Flushes EVA 1's liquid coolant     |
| EV1 OXYGEN   | OPEN       | CLOSED      | Fills EVA 1's oxygen tanks         |
| EMU2 POWER   | ON         | OFF         | Remotely powers the suit for EVA 2 |
| EV2 SUPPLY   | OPEN       | CLOSED      | Fills EVA 1's liquid coolant       |
| EV2 WASTE    | OPEN       | CLOSED      | Flushes EVA 1's liquid coolant     |
| EV2 OXYGEN   | OPEN       | CLOSED      | Fills EVA 1's oxygen tanks         |
| O2 Vent      | OPEN       | CLOSED      | Flushes both EVAs oxygen tanks     |
| DEPRESS PUMP | ON         | OFF         | Pressurizes both EVA suits         |

### DCU
The display and control unit (DCU) used for this challenge is a component that allows the user to control various settings of their suit's operation during an EVA. For example, if scrubber A's CO2 storage fills up, you could flip a switch on the DCU to flush it while switching to scubber B. A picture of the DCU you will be using during test week can be found [here](test)


| Sensor  | Value True | Value False     | Description                                                                                           |
| ------- | ---------- | --------------- | ----------------------------------------------------------------------------------------------------- |
| BATTERY | SUIT BATT  | UMBILICAL POWER | Describes if the suit is running off its local battery or UIA power                                   |
| OXYGEN  | PRI TANK   | SEC TANK        | Describes if the suit is pulling from primary or secondary oxygen tanks                               |
| COMMS   | Channel A  | Channel B       | Describes if the suit is connected to Channel A or Channel B on comms                                 |
| FAN     | PRI FAN    | SEC FAN         | Describes if the suit is using the primary fan or secondary fan                                       |
| PUMP    | OPEN       | CLOSED          | Describes if the coolant pump for the suit is open or closed (allows water to be flushed or supplied) |
| CO2     | Scrubber A | Scrubber B      | Describes which scrubber is currently filling with CO2 (other is venting)                             |

## DUST Simulator
[DUST](https://software.nasa.gov/software/MSC-27522-1) (Digital Lunar Exploration Sites Unreal Simulation Tool) is a 3D visualization of the Lunar South Pole built in Unreal Engine. Since we can not use a physical rover to drive around in during test week, you will use this software developed by NASA to simulate a pressurized rover.

### Requesting Access
After being selected as a team to participate in the rover segment of the NASA SUITS challenge, your team lead will be contacted via email with additional instructions to request access to the software. Please note that if your team was selected for the EVA segment of the challenge, you will not need to use this software.

### Connecting to the simulator
After opening the simulator on a Windows PC, a screen prompting you to enter an IP address should show up. This is prompting you to enter the website address for the TSS server, which is used to communicate back and forth with the simulator. You will want to setup the server on the same laptop or via a local network.

### Controls
During the challenge, you will be expected to issue commands to control the rover via TSS (see <a href="#rover-commanding">rover commanding</a>). However for debug purposes, we have included several keyboard shortcuts that can be ran in the simulator to control the rover, reset the position, etc.

| Keyboard Shortcut | Action |
| -------------- | ---------
Cmd + R | Reset the rover's position back to the starting base

## Development
The telemetry server is an important part of the challenge as it will serve as the main way to communicate with the DUST lunar simulator for the rover team and fetch telemetry for both the EVA and rover teams. You'll take this telemetry data and use it within the respective interfaces that you are designing and developing ahead of test week. This section will outline how to connect to the data stream, data formats, and other helpful information for development.

### UDP Socket communication

To create a more realistic scenario, we are requiring students to request and send commands over the [user datagram protocol](https://www.cloudflare.com/learning/ddos/glossary/user-datagram-protocol-udp/) (UDP) instead of over a typical HTTPS connection. For both fetching data and issuing actions and commands, you will use a specified command number, more details can be found below. Please note that all requests should be formatted in [big endian](https://www.geeksforgeeks.org/dsa/little-and-big-endian-mystery/) format.

| Timestamp (unit32) | Command number (uint32) |
| ------------------ | ----------------------- |
| 4 bytes            | 4 bytes                 |

The timestamp is a UNIX timestamp, and the command numbers will be covered below. The socket will respond with a message of the following format. The command number will be the same one you sent, and the output data will be ___ @TODO

| Timestamp (unit32) | Command number (uint32) | Output Data |
| ------------------ | ----------------------- | ---------------------- |
| 4 bytes            | 4 bytes                 | variable bytes                |



Here's a list of get commands you can send to the socket. They are related to the json files in `frontend/data`, and will be listed as such.
| Command number | Referenced .json file|
| -------------- | ---------
0 | [EVA.json](/data/EVA.json)
1 | [ROVER.json](/data/ROVER.json)
2 | [LTV.json](/data/LTV.json)

Here is an example packet you can could send to fetch the ROVER.json file:

```
0000 0000 0000 (fill this in)
```
            
### Rover LIDAR

The pressurized rover in the DUST simulation has 13 'LIDAR' sensors. Each of these sensors are points that shoot out a ray 10 meters in a direction.The value of each sensor will be the distance in centimeters the ray took to hit something, or -1 if it didn't hit anything. The return data of the 172 command above will actually be a list of 13 float values instead of the normal 1 float. Here is a description of each sensor in order:

| Sensor index | Sensor Coordinates                 | Sensor location description   | Sensor Orientation                                |
| ------------ | ----------------------------------------- | ----------------------------- | ------------------------------------------------- |
| 0            | (X=170.000000,Y=-150.000000,Z=15.000000)  | Hub of the front left wheel   | Yawed 30 degrees left (CCW) of vehicle forward    |
| 1            | (X=200.000000,Y=-40.000000,Z=20.000000)   | Front left of vehicle frame   | Yawed 20 degrees left (CCW) of vehicle forward    |
| 2            | (X=200.000000,Y=0.000000,Z=20.000000)     | Front center of vehicle frame | Vehicle forward                                   |
| 3            | (X=200.000000,Y=40.00000,Z=20.000000)     | Front right of vehicle frame  | Yawed 20 degrees right (CW) of vehicle forward    |
| 4            | (X=170.000000,Y=150.000000,Z=15.000000)   | Hub of front right wheel      | Yawed 30 degrees right (CW) of vehicle forward    |
| 5            | (X=200.000000,Y=-40.000000,Z=20.000000)   | Front left of vehicle frame   | Pitched 25 degrees down of vehicle forward        |
| 6            | (X=200.000000,Y=40.000000,Z=20.000000)    | Front right of vehicle frame  | Pitched 25 degrees down of vehicle forward        |
| 7            | (X=0.000000,Y=-100.000000,Z=-0.000000)    | Center Left of vehicle frame  | Pitched 20 degrees down of vehicle left           |
| 8            | (X=0.000000,Y=100.000000,Z=-0.000000)     | Center Right of vehicle frame | Pitched 20 degrees down of vehicle right          |
| 9            | (X=-135.000000,Y=-160.000000,Z=15.000000) | Hub of back left wheel        | Yawed 40 degrees left (CW) of vehicle backwards   |
| 10           | (X=-180.000000,Y=-60.000000,Z=15.000000)  | Rear left of vehicle frame    | Vehicle backwards                                 |
| 11           | (X=-180.000000,Y=60.000000,Z=15.000000)   | Rear right of vehicle frame   | Vehicle backwards                                 |
| 12           | (X=-135.000000,Y=160.000000,Z=15.000000)  | Hub of back right wheel       | Yawed 40 degrees right (CCW) of vehicle backwards |

### Rover Commanding

Commanding the rover is done through the same socket connection, using the following command format:

| Timestamp (unit32) | Command number (uint32) | Input Data (float) |
| ------------------ | ----------------------- | ------------------ |
| 4 bytes            | 4 bytes                 | 4 bytes            |

| Command number | Command  | Data input       |
| -------------- | -------- | ---------------- |
| 1107           | Brakes   | float: 0 or 1    |
| 1109           | Throttle | float: -100 (reverse), 100 |
| 1110           | Steering | float: -1.0, 1.0 |
| 1106           | Lights   | float: 0 or 1    |
(@TODO add the other commands)

Here is an example UDP packet to increase the rover to full throttle (100).

```
0000 ____ (finish)
```

## Testing

It is incredibly important to test your hardware and software ahead of test week in May. The interface for TSS is intended to allow you to debug certain parts of your design in the absence of the physical <a href="#peripheral-devices">peripheral devices</a>. In the web interface, you'll note sections for both the UIA and DCU, with switches you can flip. These can be enabled and disables to test your systems and note how they can impact telemetry values.

### Scripts
We have created various scripts to support testing and simulate real world values ahead of test week.

- Simulated position values: `python simulate_position.py <tss_server_address>`
- @TODO add more