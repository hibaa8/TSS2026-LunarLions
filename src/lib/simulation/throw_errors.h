#ifndef THROW_ERRORS_H
#define THROW_ERRORS_H

#include <stdbool.h>

#include "sim_engine.h"
#include "sim_algorithms.h"

//Error throwing functions
bool throw_error(sim_engine_t* engine);
bool throw_O2_storage_error(sim_engine_t* engine);
bool throw_fan_RPM_high_error(sim_engine_t* engine);
bool throw_fan_RPM_low_error(sim_engine_t* engine);

//determine which error to throw and when to throw it per run
int error_to_throw();
int time_to_throw_error();

//function to update error states based on DCU field settings, called in data.c before sim_engine_update to ensure error states are updated before each simulation update
void update_EVA_error_simulation_error_states(sim_engine_t* sim_engine);
void update_O2_error_state(sim_engine_t* sim_engine);
void update_fan_error_state(sim_engine_t* sim_engine);
void update_power_error_state(sim_engine_t* sim_engine);

#endif // THROW_ERRORS_H