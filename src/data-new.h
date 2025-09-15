#ifndef DATA_H
#define DATA_H

#include <stdbool.h>
#include <time.h>
#include <stdint.h>
#include "lib/cjson/cJSON.h"
#include "lib/simulation/sim_engine.h"
#include <stdlib.h>
#include <stdio.h>  


#define NUM_TEAMS 10

enum ROVER_SECTIONS {
    pr_telemetry
};

enum EVA_SECTIONS {
    telemetry,
    status,
    dcu,
    error,
    imu,
    uia
};

struct backend_data_t {
    // Timing information
    uint32_t start_time;
    uint32_t server_up_time;

    // DUST rover simulation
    int running_pr_sim;
    bool pr_sim_paused;

    // Simulation engine (one for each team)
    sim_engine_t* sim_engine[NUM_TEAMS];
};


// Backend Lifecycle Functions
struct backend_data_t* init_backend();
void start_simulation(struct backend_data_t* backend);
void cleanup_backend(struct backend_data_t*  backend);

// UDP Request Handlers
void handle_udp_get_request(unsigned int command, unsigned char* data, struct backend_data_t* backend);
void handle_udp_post_request(unsigned int command, unsigned char* data, struct backend_data_t* backend);

// Data management
void update_json_file(const char* filename, const int team_number, const char* section, const char* field, char* new_value);
void sync_simulation_to_json(struct backend_data_t* backend, int team_index);
cJSON* get_json_file(const char* filename, const int team_number);
void send_json_file(const char* filename, const int team_number, unsigned char* data);
void send_json_section(const char* filename, const int team_number, const char* section_name, unsigned char* data);

// Helper functions
void reverse_bytes(unsigned char* bytes);
bool big_endian();
bool update_resource(char* request_content, struct backend_data_t* backend);
float get_rover_field_value(struct backend_data_t* backend, int team_index, const char* field_name);

#endif // DATA_H