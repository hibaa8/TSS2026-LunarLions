#ifndef THROW_ERRORS_H
#define THROW_ERRORS_H

#include <stdbool.h>

#include "sim_engine.h"
#include "sim_algorithms.h"

//number of total random errors we have (used in error_to_throw function to determine range of random number generation)
#define NUM_ERRORS 4

//Error throwing functions
bool throw_random_error(sim_engine_t* engine);
bool throw_O2_suit_pressure_high_error(sim_engine_t* engine);
bool throw_O2_suit_pressure_low_error(sim_engine_t* engine);
bool throw_fan_RPM_high_error(sim_engine_t* engine);
bool throw_fan_RPM_low_error(sim_engine_t* engine);

//determine which error to throw and when to throw it per run
int error_to_throw();
int time_to_throw_error();

//types of errors
typedef enum {
    SUIT_PRESSURE_OXY_LOW,
    SUIT_PRESSURE_OXY_HIGH,
    FAN_RPM_HIGH,
    FAN_RPM_LOW
} error_type_t;



#endif // THROW_ERRORS_H