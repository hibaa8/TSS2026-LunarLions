#include "throw_errors.h"
#include <time.h>
#include <stdlib.h>

/**
* Determines the type of error (pressure, fan RPM high, fan RPM low) to throw determined by random chance.
* @return int indicating which error to throw (0 = pressure error, 1 = fan RPM high error, 2 = fan RPM low error, 3 = CO2 scrubber error)
*/
int error_to_throw() {

        // Seed the random number generator only ONCE
        srand(time(NULL));

        int random_value = rand() % NUM_ERRORS; // Random value between 0 and 3
        return random_value;
}

/**
    * Determines the time in which an error will be thrown
    * @return int indicating the time in delta_time between start and 
    * 300* delta_time from start in which the error will be thrown
 */
int time_to_throw_error() {
     // Seed the random number generator only ONCE
        srand(time(NULL));

    int random_value = rand() % 10+1; // Random time between 1 and 10 * delta_time
    printf("Random time to throw error (in seconds): Task Board completion time + %d seconds\n", random_value);
    return random_value;
}

/**
    * Throws the specified error by calling the appropriate error function.
    * 
    * @param error_type Integer indicating which error to throw (0 = pressure, 1 = fan RPM high, 2 = fan RPM low, 3 = power)
    * @return bool indicating success or failure of error throwing
 */
bool throw_random_error(sim_engine_t* engine) {
    engine->error_type = error_to_throw();
    printf("Error type determined to throw: %d\n", engine->error_type);
    switch(engine->error_type) {
        case SUIT_PRESSURE_OXY_LOW:
            return throw_O2_suit_pressure_low_error(engine);
        case SUIT_PRESSURE_OXY_HIGH:
            return throw_O2_suit_pressure_high_error(engine);
        case FAN_RPM_HIGH:
            return throw_fan_RPM_high_error(engine);
        case FAN_RPM_LOW:
            return throw_fan_RPM_low_error(engine);
        default:
            return false;
    }
}

/**
 * Rapidly decreases O2 pressure to simulate a leak.
 * Pressure will drop according to rapid linear decay algorithm.
 * Will update UI and JSON files accordingly.
 * @param engine Pointer to the simulation engine
 * @return bool indicating success or failure
 * 
*/
bool throw_O2_suit_pressure_low_error(sim_engine_t* engine) {
    sim_component_t* eva1 = sim_engine_get_component(engine, "eva1");
        if (eva1 == NULL) {
            printf("Simulation tried to access non-existent component 'eva1' for O2 storage error\n");
            return false;
        }

    //set the field start_time to 0 so the rapid decay starts from the current value at the time of error
    sim_field_t* field = sim_engine_find_field_within_component(eva1, "suit_pressure_oxy");
    if (field) {
        field->start_time = 0.0f; //restart the timer for the rapid linear growth algorithm so it starts growing from the current value at the time of error
        field->active = true; //make the field active so it starts updating based on the new algorithm
    } else {
        printf("Simulation tried to access non-existent field 'suit_pressure_oxy' for O2 storage error\n");
        return false;
    }

    //set the field algorithm to rapid linear decrease
    field->algorithm = SIM_ALGO_RAPID_LINEAR_DECAY;
    printf("O2 suit pressure low error thrown: rapidly decreasing O2 pressure\n");

    return true;
}

/**
 * Rapidly increases O2 pressure to simulate a leak.
 * Pressure will increase according to rapid linear decay algorithm.
 * Will update UI and JSON files accordingly.
 * @param engine Pointer to the simulation engine
 * @return bool indicating success or failure
 * 
*/
bool throw_O2_suit_pressure_high_error(sim_engine_t* engine) {
    sim_component_t* eva1 = sim_engine_get_component(engine, "eva1");
        if (eva1 == NULL) {
            printf("Simulation tried to access non-existent component 'eva1' for O2 storage error\n");
            return false;
        }

    //set the field start_time to 0 so the rapid decay starts from the current value at the time of error
    sim_field_t* field = sim_engine_find_field_within_component(eva1, "suit_pressure_oxy");
    if (field) {
        field->start_time = 0.0f; //restart the timer for the rapid linear growth algorithm so it starts growing from the current value at the time of error
        field->active = true; //make the field active so it starts updating based on the new algorithm
    } else {
        printf("Simulation tried to access non-existent field 'suit_pressure_oxy' for O2 storage error\n");
        return false;
    }

    //set the field algorithm to rapid linear decrease
    field->algorithm = SIM_ALGO_RAPID_LINEAR_GROWTH;
    printf("O2 suit pressure high error thrown: rapidly increasing O2 pressure\n");

    return true;
}


/**
 * Rapidly increases fan RPM to simulate a malfunction.
 * Fan RPM will increase according to rapid linear growth algorithm.
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

    //set the field start_time to 0 so the rapid growth starts from the current value at the time of error
    sim_field_t* field = sim_engine_find_field_within_component(eva1, "fan_pri_rpm");
    if (field) {
        field->start_time = 0.0f;  //restart the timer for the rapid linear growth algorithm so it starts growing from the current value at the time of error
        field->active = true; //make the field active so it starts updating based on the new algorithm
    } else {
        printf("Simulation tried to access non-existent field 'fan_pri_rpm' for fan RPM high error\n");
        return false;
    }

    //set the field algorithm to rapid linear growth
    field->algorithm = SIM_ALGO_RAPID_LINEAR_GROWTH;
    printf("Fan RPM high error thrown. Algorithm set to SIM_ALGO_RAPID_LINEAR_GROWTH for field 'fan_pri_rpm'\n");
    return true;
}

/**
 * Rapidly decreases fan RPM to simulate a malfunction.
 * Fan RPM will drop according to rapid linear decay algorithm.
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

    //set the field start_time to 0 so the rapid decay starts from the current value at the time of error
    sim_field_t* field = sim_engine_find_field_within_component(eva1, "fan_pri_rpm");
    if (field) {
        field->start_time = 0.0f; //restart the timer for the rapid decay algorithm so it starts decaying from the current value at the time of error
    } else {
        printf("Simulation tried to access non-existent field 'fan_pri_rpm' for fan RPM low error\n");
        return false;
    }

    //set the field algorithm to rapid linear decay
    field->algorithm = SIM_ALGO_RAPID_LINEAR_DECAY;
    printf("Fan RPM low error thrown. Algorithm set to SIM_ALGO_RAPID_LINEAR_DECAY for field 'fan_pri_rpm'\n");
    return true;
}

