#ifndef SIM_ALGORITHMS_H
#define SIM_ALGORITHMS_H

#include "sim_engine.h"

///////////////////////////////////////////////////////////////////////////////////
//                           Algorithm Functions
///////////////////////////////////////////////////////////////////////////////////

// Algorithm update functions
sim_value_t sim_algo_sine_wave(sim_field_t* field, float current_time);
sim_value_t sim_algo_linear_decay(sim_field_t* field, float current_time);
sim_value_t sim_algo_linear_growth(sim_field_t* field, float current_time);
sim_value_t sim_algo_dependent_value(sim_field_t* field, float current_time, sim_engine_t* engine);

// Algorithm parameter validation
bool sim_algo_validate_sine_wave_params(cJSON* params);
bool sim_algo_validate_linear_decay_params(cJSON* params);
bool sim_algo_validate_linear_growth_params(cJSON* params);
bool sim_algo_validate_dependent_value_params(cJSON* params);

// Utility functions
float sim_algo_evaluate_formula(const char* formula, sim_engine_t* engine);
sim_algorithm_type_t sim_algo_parse_type_string(const char* algo_string);
const char* sim_algo_type_to_string(sim_algorithm_type_t type);

#endif // SIM_ALGORITHMS_H