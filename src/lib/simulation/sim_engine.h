#ifndef SIM_ENGINE_H
#define SIM_ENGINE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "../cjson/cJSON.h"

///////////////////////////////////////////////////////////////////////////////////
//                                  Data Types
///////////////////////////////////////////////////////////////////////////////////

typedef enum {
    SIM_TYPE_FLOAT,
    SIM_TYPE_INT, 
    SIM_TYPE_BOOL
} sim_field_type_t;

typedef enum {
    SIM_ALGO_SINE_WAVE,
    SIM_ALGO_LINEAR_DECAY,
    SIM_ALGO_LINEAR_GROWTH,
    SIM_ALGO_DEPENDENT_VALUE
} sim_algorithm_type_t;

typedef union {
    float f;
    int i;
    bool b;
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
} sim_component_t;

typedef struct {
    sim_component_t* components;
    int component_count;
    
    sim_field_t** update_order;  // Fields sorted by dependencies
    int total_field_count;
    
    float simulation_time;
    float last_update_time;
    bool initialized;
} sim_engine_t;

///////////////////////////////////////////////////////////////////////////////////
//                                 Engine API
///////////////////////////////////////////////////////////////////////////////////

// Engine lifecycle
sim_engine_t* sim_engine_create(void);
void sim_engine_destroy(sim_engine_t* engine);

// Configuration loading
bool sim_engine_load_from_directory(sim_engine_t* engine, const char* directory_path);
bool sim_engine_load_component(sim_engine_t* engine, const char* json_file_path);

// Simulation control
bool sim_engine_initialize(sim_engine_t* engine);
void sim_engine_update(sim_engine_t* engine, float delta_time);
void sim_engine_reset(sim_engine_t* engine);

// Field access
sim_value_t sim_engine_get_field_value(sim_engine_t* engine, const char* field_name);
sim_value_t sim_engine_get_component_field_value(sim_engine_t* engine, const char* component_name, const char* field_name);
bool sim_engine_set_field_value(sim_engine_t* engine, const char* field_name, sim_value_t value);

// Utility functions
sim_field_t* sim_engine_find_field(sim_engine_t* engine, const char* field_name);
void sim_engine_print_status(sim_engine_t* engine);

#endif // SIM_ENGINE_H