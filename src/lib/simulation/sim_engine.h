#ifndef SIM_ENGINE_H
#define SIM_ENGINE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "../cjson/cJSON.h"

///////////////////////////////////////////////////////////////////////////////////
//                                  Constants
///////////////////////////////////////////////////////////////////////////////////

#define SIM_DATA_ROOT "data"
#define SIM_CONFIG_ROOT "src/lib/simulation/config"

///////////////////////////////////////////////////////////////////////////////////
//                                  Data Types
///////////////////////////////////////////////////////////////////////////////////

typedef enum {
    SIM_TYPE_FLOAT
} sim_field_type_t;

typedef enum {
    SIM_ALGO_SINE_WAVE,
    SIM_ALGO_LINEAR_DECAY,
    SIM_ALGO_LINEAR_GROWTH,
    SIM_ALGO_DEPENDENT_VALUE,
    SIM_ALGO_EXTERNAL_VALUE
} sim_algorithm_type_t;

typedef union {
    float f;
} sim_value_t;

typedef struct {
    char* field_name;
    char* component_name;
    sim_field_type_t type;
    sim_algorithm_type_t algorithm;
    sim_value_t current_value;
    sim_value_t previous_value;

    // Algorithm parameters (parsed from JSON)
    cJSON* params;

    // Dependencies
    char** depends_on;
    int depends_count;

    // Internal state for algorithms
    float start_time;
    bool initialized;
} sim_field_t;

typedef struct {
    char* component_name;
    sim_field_t* fields;
    int field_count;
    bool running;  // Controls whether this component's fields update
    float simulation_time;  // Component-specific simulation time
} sim_component_t;

typedef struct {
    sim_component_t* components;
    int component_count;

    sim_field_t** update_order;  // Fields sorted by dependencies
    int total_field_count;

    bool initialized;
} sim_engine_t;

///////////////////////////////////////////////////////////////////////////////////
//                                 Engine API
///////////////////////////////////////////////////////////////////////////////////

// Engine lifecycle
sim_engine_t* sim_engine_create();
void sim_engine_destroy(sim_engine_t* engine);

// Configuration loading
bool sim_engine_load_predefined_configs(sim_engine_t* engine);
bool sim_engine_load_component(sim_engine_t* engine, const char* json_file_path);

// Simulation control
bool sim_engine_initialize(sim_engine_t* engine);
void sim_engine_update(sim_engine_t* engine, float delta_time);
void sim_engine_start_component(sim_engine_t* engine, const char* component_name);
void sim_engine_stop_component(sim_engine_t* engine, const char* component_name);
void sim_engine_reset_component(sim_engine_t* engine, const char* component_name,
                               void (*update_json)(const char*, const char*, const char*, char*));

// Field access
sim_value_t sim_engine_get_field_value(sim_engine_t* engine, const char* field_name);

// Component status
bool sim_engine_is_component_running(sim_engine_t* engine, const char* component_name);

// Utility functions
sim_field_t* sim_engine_find_field(sim_engine_t* engine, const char* field_name);

#endif // SIM_ENGINE_H