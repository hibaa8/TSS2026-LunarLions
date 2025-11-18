# TSS Development Documentation
This document is intended to provide an overview of the current telemetry stream server repository, allowing new developers or interested students to understand the structure and how to make changes/contribute to the telemetry stream server.

## Introduction

### Navigation

### Background

The NASA SUITS project has taken on several iterations since it's start in 2018. This telemetry server marks a significant rewrite from the previous iteration [TSS 2025](https://github.com/SUITS-Techteam/TSS-2025). We identified several key improvement points including a new interface, better simulated telemetry values, and other features that we wanted to include in a new iteration.

TSS was developed for the NASA SUITS challenge with the goal of providing a realiable stream of data to particpating teams to use within their interfaces and hardware designs. This started out with several basic linear decay and growth algorithms to model basic resource consumption, but has since been expanded to accomodate complex relationships between multiple telemetry values (e.g. EVA location values calculating speed, increasing heart rate). These complex telemetry relationships help to create a more realistic scenario for students to develop with.

In the previous SUITS iteration, we added a new segment to the challenge to highlight the need of interfaces for a pressurized rover. Unlike the EVA, where teams could take advantage of AR hardware in the JSC Rock Yard, we knew that we could not use a physical rover as an asset during test week. This led to using [DUST](https://software.nasa.gov/software/MSC-27522-1), a simulation developed at NASA of the Lunar South Pole built in Unreal Engine.

TSS now acts as a hub for communications with both teams and external assets, with all communications being proxied through the server. For example, if a team wants to control the pressurized rover in DUST, then they 

### Structure
The project is separated into several folders:

- `/data`: contains the JSON files that are read/written to for telemetry storage
- `/documents`: contains supporting documentation, images, etc for students to use during development
- `/frontend`: all frontend assets, primarily HTML, JS, and CSS
- `/scripts`: supporting Python scripts to allow users to test various parts of their implementation
- `/src`:

## Frontend Development

## Server Development

## Simulation Development