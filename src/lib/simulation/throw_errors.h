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
bool throw_CO2_scrubber_error(sim_engine_t* engine);

//determine which error to throw and when to throw it per run
int error_to_throw();
int time_to_throw_error();


#endif // THROW_ERRORS_H