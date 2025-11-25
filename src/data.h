#ifndef DATA_H
#define DATA_H

#include <stdbool.h>
#include <time.h>
#include <stdint.h>
#include "lib/cjson/cJSON.h"
#include "lib/simulation/sim_engine.h"
#include <stdlib.h>
#include <stdio.h>  

// UDP command mapping structure
typedef struct {
    unsigned int command;
    const char* path;        // full dot-separated path like "rover.pr_telemetry.brakes"
    const char* data_type;   // bool or float, this makes the parsing easier
} udp_command_mapping_t;

struct backend_data_t {
    // Timing information
    uint32_t start_time;
    uint32_t server_up_time;

    // DUST rover simulation
    int running_pr_sim;
    bool pr_sim_paused;

    // Simulation engine
    sim_engine_t* sim_engine;
};

// Backend Lifecycle Functions
struct backend_data_t* init_backend();
void increment_simulation(struct backend_data_t* backend);
void cleanup_backend(struct backend_data_t*  backend);

// UDP Request Handlers
void handle_udp_get_request(unsigned int command, unsigned char* data, struct backend_data_t* backend);
bool handle_udp_post_request(unsigned int command, unsigned char* data, struct backend_data_t* backend);

// Data management
void update_json_file(const char* filename, const char* section, const char* field_path, char* new_value);
void sync_simulation_to_json(struct backend_data_t* backend);
cJSON* get_json_file(const char* filename);
void send_json_file(const char* filename, unsigned char* data);
void update_eva_station_timing(void);
void reset_eva_station_timing(void);

// Helper functions
void reverse_bytes(unsigned char* bytes);
bool big_endian();
bool html_form_json_update(char* request_content, struct backend_data_t* backend);
double get_field_from_json(const char* filename, const char* field_path, double default_value);

// UDP data extraction helpers
bool extract_bool_value(unsigned char* data);
float extract_float_value(unsigned char* data);

// UDP command to JSON path mapping table
static const udp_command_mapping_t udp_command_mappings[] = {
    // ROVER commands (sent from the DUST Unreal Engine simulation over UDP)
    {1103, "rover.pr_telemetry.cabin_heating", "bool"},
    {1104, "rover.pr_telemetry.cabin_cooling", "bool"},
    {1105, "rover.pr_telemetry.co2_scrubber", "bool"},
    {1106, "rover.pr_telemetry.lights_on", "bool"}, // @TODO check if this will take 0 and 1 as inputs as well as true or false

    {1107, "rover.pr_telemetry.brakes", "bool"},
    {1109, "rover.pr_telemetry.throttle", "float"},
    {1110, "rover.pr_telemetry.steering", "float"},
    {1111, "rover.pr_telemetry.rover_pos_x", "float"},
    {1112, "rover.pr_telemetry.rover_pos_y", "float"},
    {1113, "rover.pr_telemetry.rover_pos_z", "float"},
    {1114, "rover.pr_telemetry.heading", "float"},
    {1115, "rover.pr_telemetry.pitch", "float"},
    {1116, "rover.pr_telemetry.roll", "float"},
    {1117, "rover.pr_telemetry.distance_traveled", "float"},
    {1118, "rover.pr_telemetry.speed", "float"},
    {1119, "rover.pr_telemetry.surface_incline", "float"},

    //1132 signal strength

    {1130, "rover.pr_telemetry.lidar", "array<float>"}, // NOTE: lidar is float array, this data type is handled specially in the UDP handler
    {1131, "rover.pr_telemetry.sunlight", "float"},

    // UIA commands (sent from the peripheral device over UDP)
    {2001, "eva.uia.eva1_power", "bool"},
    {2002, "eva.uia.eva1_oxy", "bool"},
    {2003, "eva.uia.eva1_water_supply", "bool"},
    {2004, "eva.uia.eva1_water_waste", "bool"},
    {2005, "eva.uia.eva2_power", "bool"},
    {2006, "eva.uia.eva2_oxy", "bool"},
    {2007, "eva.uia.eva2_water_supply", "bool"},
    {2008, "eva.uia.eva2_water_waste", "bool"},
    {2009, "eva.uia.oxy_vent", "bool"},
    {2010, "eva.uia.depress", "bool"},

    // DCU commands (sent from the peripheral device over UDP)
    {2011, "eva.dcu.eva1.batt", "bool"},
    {2012, "eva.dcu.eva1.oxy", "bool"},
    {2013, "eva.dcu.eva1.comm", "bool"},
    {2014, "eva.dcu.eva1.fan", "bool"},
    {2015, "eva.dcu.eva1.pump", "bool"},
    {2016, "eva.dcu.eva1.co2", "bool"},

    // IMU position commands from the TSS-Location-App Python server
    {2017, "eva.imu.eva1.posx", "float"},
    {2018, "eva.imu.eva1.posy", "float"},
    {2019, "eva.imu.eva1.heading", "float"},
    {2020, "eva.imu.eva2.posx", "float"},
    {2021, "eva.imu.eva2.posy", "float"},
    {2022, "eva.imu.eva2.heading", "float"},

    {0, NULL, NULL} // Sentinel
};

#endif // DATA_H