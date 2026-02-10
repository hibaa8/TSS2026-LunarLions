#include "throw_errors.h"
#include <time.h>
#include <stdlib.h>

/**
* Determines the type of error (pressure, fan RPM high, fan RPM low) to throw determined by random chance.
* @return int indicating which error to throw (0 = pressure error, 1 = fan RPM high error, 2 = fan RPM low error)
*/
int error_to_throw() {

        // Seed the random number generator only ONCE
        srand(time(NULL));

        int random_value = rand() % 3; // Random value between 0 and 2
        return random_value;
}

/**
    * Determines the time in which an error will be thrown
    * @return int indicating the time in delta_time between start and 
    * 300* delta_time from start in which the error will be thrown
 */
int time_to_throw_error() {
    int random_value = rand() % 300; // Random time between 0 and 300 * delta_time
    return random_value;
}

/**
    * Throws the specified error by calling the appropriate error function.
    * 
    * @param error_type Integer indicating which error to throw (0 = pressure, 1 = fan RPM high, 2 = fan RPM low, 3 = power)
    * @return bool indicating success or failure of error throwing
 */
bool throw_error(sim_engine_t* engine) {
    switch(engine->error_type) {
        case 0:
            return throw_O2_storage_error(engine);
        case 1:
            return throw_fan_RPM_high_error(engine);
        case 2:
            return throw_fan_RPM_low_error(engine);
        default:
            return false;
    }
}

/**
 * Rapidly decreases O2 pressure to simulate a leak.
 * Pressure will drop to below 20% of normal within 10 seconds.
 * Will update UI and JSON files accordingly.
 * @param engine Pointer to the simulation engine
 * @return bool indicating success or failure
 * 
*/
bool throw_O2_storage_error(sim_engine_t* engine) {
    sim_component_t* eva1 = sim_engine_get_component(engine, "eva1");
        if (eva1 == NULL) {
            printf("Simulation tried to access non-existent component 'eva1' for O2 storage error\n");
            return false;
        }

    //set the field start_time to the component simulation time
    sim_field_t* field = sim_engine_find_field_within_component(eva1, "oxy_pri_storage");
    if (field) {
        field->start_time = eva1->simulation_time;
    } else {
        printf("Simulation tried to access non-existent field 'oxy_pri_storage' for O2 storage error\n");
        return false;
    }

    //set the field algorithm to rapid linear decrease
    field->algorithm = SIM_ALGO_RAPID_LINEAR_DECAY;

    return true;
}

/**
 * Rapidly increases fan RPM to simulate a malfunction.
 * Fan RPM will exceed 120% of normal within 10 seconds.
 * Will update UI and JSON files accordingly.
 * @param engine Pointer to the simulation engine
 * @return bool indicating success or failure
 * 
*/
bool throw_fan_RPM_high_error(sim_engine_t* engine) {

    sim_component_t* eva1 = sim_engine_get_component(engine, "eva1");
    if (eva1 == NULL) {
        printf("Simulation tried to access non-existent component 'eva1' for fan RPM high error\n");
        return false;
    }

    //set the field start_time to the component simulation time
    sim_field_t* field = sim_engine_find_field_within_component(eva1, "fan_pri_rpm");
    if (field) {
        field->start_time = eva1->simulation_time;
    } else {
        printf("Simulation tried to access non-existent field 'fan_pri_rpm' for fan RPM high error\n");
        return false;
    }

    //set the field algorithm to rapid linear growth
    field->algorithm = SIM_ALGO_RAPID_LINEAR_GROWTH;
    return true;
}

/**
 * Rapidly decreases fan RPM to simulate a malfunction.
 * Fan RPM will drop below 80% of normal within 10 seconds.
 * Will update UI and JSON files accordingly.
 * @param engine Pointer to the simulation engine
 * @return bool indicating success or failure
 * 
*/
bool throw_fan_RPM_low_error(sim_engine_t* engine) {

    sim_component_t* eva1 = sim_engine_get_component(engine, "eva1");
    if (eva1 == NULL) {
        printf("Simulation tried to access non-existent component 'eva1' for fan RPM low error\n");
        return false;
    }

    //set the field start_time to the component simulation time
    sim_field_t* field = sim_engine_find_field_within_component(eva1, "fan_pri_rpm");
    if (field) {
        field->start_time = eva1->simulation_time;
    } else {
        printf("Simulation tried to access non-existent field 'fan_pri_rpm' for fan RPM low error\n");
        return false;
    }

    //set the field algorithm to rapid linear decay
    field->algorithm = SIM_ALGO_RAPID_LINEAR_DECAY;
    return true;
}