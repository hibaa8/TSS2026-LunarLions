#include "sim_algorithms.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>

///////////////////////////////////////////////////////////////////////////////////
//                           Algorithm Implementations
///////////////////////////////////////////////////////////////////////////////////

/**
 * Implements sine wave algorithm for oscillating values.
 * Generates values that oscillate in a sine wave pattern over time.
 * Parameters: base_value, amplitude, frequency, phase_offset (optional)
 * 
 * @param field Pointer to the field containing algorithm parameters
 * @param current_time Current simulation time in seconds
 * @return Calculated value based on sine wave function
 */
sim_value_t sim_algo_sine_wave(sim_field_t* field, float current_time) {
    sim_value_t result = {0};
    
    if (!field || !field->params) return result;
    
    // Get parameters
    cJSON* base_value = cJSON_GetObjectItem(field->params, "base_value");
    cJSON* amplitude = cJSON_GetObjectItem(field->params, "amplitude");
    cJSON* frequency = cJSON_GetObjectItem(field->params, "frequency");
    cJSON* phase_offset = cJSON_GetObjectItem(field->params, "phase_offset");
    
    float base = base_value && cJSON_IsNumber(base_value) ? (float)cJSON_GetNumberValue(base_value) : 0.0f;
    float amp = amplitude && cJSON_IsNumber(amplitude) ? (float)cJSON_GetNumberValue(amplitude) : 1.0f;
    float freq = frequency && cJSON_IsNumber(frequency) ? (float)cJSON_GetNumberValue(frequency) : 1.0f;
    float phase = phase_offset && cJSON_IsNumber(phase_offset) ? (float)cJSON_GetNumberValue(phase_offset) : 0.0f;
    
    // Calculate sine wave value
    float elapsed_time = current_time - field->start_time;
    float sine_value = base + amp * sinf((elapsed_time * freq) + phase);
    
    // Convert to appropriate type
    switch (field->type) {
        case SIM_TYPE_FLOAT:
            result.f = sine_value;
            break;
        case SIM_TYPE_INT:
            result.i = (int)roundf(sine_value);
            break;
        case SIM_TYPE_BOOL:
            result.b = sine_value > base;
            break;
    }
    
    return result;
}

/**
 * Implements linear decay algorithm for decreasing values over time.
 * Values decrease linearly from start_value to end_value over duration_seconds.
 * Parameters: start_value, end_value, duration_seconds
 * 
 * @param field Pointer to the field containing algorithm parameters
 * @param current_time Current simulation time in seconds
 * @return Calculated value based on linear interpolation
 */
sim_value_t sim_algo_linear_decay(sim_field_t* field, float current_time) {
    sim_value_t result = {0};
    
    if (!field || !field->params) return result;
    
    // Get parameters
    cJSON* start_value = cJSON_GetObjectItem(field->params, "start_value");
    cJSON* end_value = cJSON_GetObjectItem(field->params, "end_value");
    cJSON* duration = cJSON_GetObjectItem(field->params, "duration_seconds");
    
    float start_val = start_value && cJSON_IsNumber(start_value) ? (float)cJSON_GetNumberValue(start_value) : 100.0f;
    float end_val = end_value && cJSON_IsNumber(end_value) ? (float)cJSON_GetNumberValue(end_value) : 0.0f;
    float duration_sec = duration && cJSON_IsNumber(duration) ? (float)cJSON_GetNumberValue(duration) : 1.0f;
    
    // Calculate current progress (0.0 to 1.0)
    float elapsed_time = current_time - field->start_time;
    float progress = elapsed_time / duration_sec;
    
    // Clamp progress between 0 and 1
    if (progress < 0.0f) progress = 0.0f;
    if (progress > 1.0f) progress = 1.0f;
    
    // Linear interpolation from start to end
    float current_value = start_val + (end_val - start_val) * progress;
    
    // Convert to appropriate type
    switch (field->type) {
        case SIM_TYPE_FLOAT:
            result.f = current_value;
            break;
        case SIM_TYPE_INT:
            result.i = (int)roundf(current_value);
            break;
        case SIM_TYPE_BOOL:
            result.b = current_value > ((start_val + end_val) * 0.5f);
            break;
    }
    
    return result;
}

/**
 * Implements linear growth algorithm for increasing values over time.
 * Values increase linearly from start_value at growth_rate per second.
 * Parameters: start_value (optional, default 0), growth_rate, max_value (optional)
 * 
 * @param field Pointer to the field containing algorithm parameters
 * @param current_time Current simulation time in seconds
 * @return Calculated value based on linear growth
 */
sim_value_t sim_algo_linear_growth(sim_field_t* field, float current_time) {
    sim_value_t result = {0};
    
    if (!field || !field->params) return result;
    
    // Get parameters
    cJSON* start_value = cJSON_GetObjectItem(field->params, "start_value");
    cJSON* growth_rate = cJSON_GetObjectItem(field->params, "growth_rate");
    cJSON* max_value = cJSON_GetObjectItem(field->params, "max_value");
    
    float start_val = start_value && cJSON_IsNumber(start_value) ? (float)cJSON_GetNumberValue(start_value) : 0.0f;
    float rate = growth_rate && cJSON_IsNumber(growth_rate) ? (float)cJSON_GetNumberValue(growth_rate) : 1.0f;
    float max_val = max_value && cJSON_IsNumber(max_value) ? (float)cJSON_GetNumberValue(max_value) : INFINITY;
    
    // Calculate current value based on growth rate
    float elapsed_time = current_time - field->start_time;
    float current_value = start_val + (rate * elapsed_time);
    
    // Clamp to maximum value if specified
    if (current_value > max_val) {
        current_value = max_val;
    }
    
    // Convert to appropriate type
    switch (field->type) {
        case SIM_TYPE_FLOAT:
            result.f = current_value;
            break;
        case SIM_TYPE_INT:
            result.i = (int)roundf(current_value);
            break;
        case SIM_TYPE_BOOL:
            result.b = current_value > start_val;
            break;
    }
    
    return result;
}

/**
 * Implements dependent value algorithm for fields calculated from other fields.
 * Evaluates a mathematical formula using values from other fields as inputs.
 * Parameters: formula (string expression), depends_on (array of field names)
 * 
 * @param field Pointer to the field containing algorithm parameters
 * @param current_time Current simulation time in seconds (unused for dependent values)
 * @param engine Pointer to the simulation engine for accessing other field values
 * @return Calculated value based on formula evaluation
 */
sim_value_t sim_algo_dependent_value(sim_field_t* field, float current_time, sim_engine_t* engine) {
    sim_value_t result = {0};
    
    if (!field || !field->params || !engine) return result;
    
    // Get formula parameter
    cJSON* formula = cJSON_GetObjectItem(field->params, "formula");
    if (!formula || !cJSON_IsString(formula)) {
        printf("Warning: No formula specified for dependent field %s\n", field->field_name);
        return result;
    }
    
    const char* formula_str = cJSON_GetStringValue(formula);
    float calculated_value = sim_algo_evaluate_formula(formula_str, engine);
    
    // Convert to appropriate type
    switch (field->type) {
        case SIM_TYPE_FLOAT:
            result.f = calculated_value;
            break;
        case SIM_TYPE_INT:
            result.i = (int)roundf(calculated_value);
            break;
        case SIM_TYPE_BOOL:
            result.b = calculated_value > 0.0f;
            break;
    }
    
    return result;
}

///////////////////////////////////////////////////////////////////////////////////
//                          Algorithm Validation
///////////////////////////////////////////////////////////////////////////////////

/**
 * Validates parameters for the sine wave algorithm.
 * Checks that required base_value parameter is present and is a number.
 * 
 * @param params JSON object containing algorithm parameters
 * @return true if parameters are valid, false otherwise
 */
bool sim_algo_validate_sine_wave_params(cJSON* params) {
    if (!params) return false;
    
    cJSON* base_value = cJSON_GetObjectItem(params, "base_value");
    if (!base_value || !cJSON_IsNumber(base_value)) return false;
    
    // Other parameters are optional with defaults
    return true;
}

/**
 * Validates parameters for the linear decay algorithm.
 * Checks that required start_value, end_value, and duration_seconds parameters are present.
 * 
 * @param params JSON object containing algorithm parameters
 * @return true if parameters are valid, false otherwise
 */
bool sim_algo_validate_linear_decay_params(cJSON* params) {
    if (!params) return false;
    
    cJSON* start_value = cJSON_GetObjectItem(params, "start_value");
    cJSON* end_value = cJSON_GetObjectItem(params, "end_value");
    cJSON* duration = cJSON_GetObjectItem(params, "duration_seconds");
    
    if (!start_value || !cJSON_IsNumber(start_value)) return false;
    if (!end_value || !cJSON_IsNumber(end_value)) return false;
    if (!duration || !cJSON_IsNumber(duration)) return false;
    
    return true;
}

/**
 * Validates parameters for the linear growth algorithm.
 * Checks that required growth_rate parameter is present and is a number.
 * 
 * @param params JSON object containing algorithm parameters
 * @return true if parameters are valid, false otherwise
 */
bool sim_algo_validate_linear_growth_params(cJSON* params) {
    if (!params) return false;
    
    cJSON* growth_rate = cJSON_GetObjectItem(params, "growth_rate");
    if (!growth_rate || !cJSON_IsNumber(growth_rate)) return false;
    
    // start_value and max_value are optional
    return true;
}

/**
 * Validates parameters for the dependent value algorithm.
 * Checks that required formula parameter is present and is a string.
 * 
 * @param params JSON object containing algorithm parameters
 * @return true if parameters are valid, false otherwise
 */
bool sim_algo_validate_dependent_value_params(cJSON* params) {
    if (!params) return false;
    
    cJSON* formula = cJSON_GetObjectItem(params, "formula");
    if (!formula || !cJSON_IsString(formula)) return false;
    
    return true;
}

///////////////////////////////////////////////////////////////////////////////////
//                            Utility Functions
///////////////////////////////////////////////////////////////////////////////////

/**
 * Evaluates a mathematical formula string using current field values.
 * Supports basic arithmetic operations (+, -, *, /) with field name substitution.
 * Example: "heart_rate * 33.22114 - 2212.8225"
 * 
 * @param formula String containing the mathematical formula to evaluate
 * @param engine Pointer to the simulation engine for field value lookup
 * @return Calculated result of the formula evaluation
 */
float sim_algo_evaluate_formula(const char* formula, sim_engine_t* engine) {
    if (!formula || !engine) return 0.0f;
    
    // Simple formula evaluator - supports basic math with field references
    // Format: "field_name * constant + constant" or "field_name * constant - constant"
    
    char* formula_copy = strdup(formula);
    char* token;
    char* rest = formula_copy;
    
    float result = 0.0f;
    float current_value = 0.0f;
    char operation = '+';
    bool expecting_value = true;
    
    // Tokenize the formula
    while ((token = strtok_r(rest, " ", &rest))) {
        if (expecting_value) {
            // Check if token is a field name or a number
            if (isdigit(token[0]) || (token[0] == '-' && isdigit(token[1]))) {
                // It's a number
                current_value = atof(token);
            } else {
                // It's a field name - look up its value
                sim_value_t field_value = sim_engine_get_field_value(engine, token);
                current_value = field_value.f;  // Assume float for formula calculations
            }
            
            // Apply the operation
            switch (operation) {
                case '+':
                    result += current_value;
                    break;
                case '-':
                    result -= current_value;
                    break;
                case '*':
                    result *= current_value;
                    break;
                case '/':
                    if (current_value != 0.0f) {
                        result /= current_value;
                    }
                    break;
                default:
                    result = current_value;
                    break;
            }
            
            expecting_value = false;
        } else {
            // Token should be an operator
            if (strlen(token) == 1 && strchr("+-*/", token[0])) {
                operation = token[0];
                expecting_value = true;
            }
        }
    }
    
    free(formula_copy);
    return result;
}

/**
 * Parses an algorithm type string into its corresponding enum value.
 * Converts JSON algorithm type strings into internal algorithm type enums.
 * 
 * @param algo_string String name of the algorithm (e.g., "sine_wave", "linear_decay")
 * @return Corresponding algorithm type enum, or SIM_ALGO_SINE_WAVE if not recognized
 */
sim_algorithm_type_t sim_algo_parse_type_string(const char* algo_string) {
    if (!algo_string) return SIM_ALGO_SINE_WAVE;
    
    if (strcmp(algo_string, "sine_wave") == 0) {
        return SIM_ALGO_SINE_WAVE;
    } else if (strcmp(algo_string, "linear_decay") == 0) {
        return SIM_ALGO_LINEAR_DECAY;
    } else if (strcmp(algo_string, "linear_growth") == 0) {
        return SIM_ALGO_LINEAR_GROWTH;
    } else if (strcmp(algo_string, "dependent_value") == 0) {
        return SIM_ALGO_DEPENDENT_VALUE;
    }
    
    return SIM_ALGO_SINE_WAVE;  // Default
}

/**
 * Converts an algorithm type enum to its string representation.
 * Used for debugging and status display purposes.
 * 
 * @param type Algorithm type enum value
 * @return String representation of the algorithm type
 */
const char* sim_algo_type_to_string(sim_algorithm_type_t type) {
    switch (type) {
        case SIM_ALGO_SINE_WAVE:
            return "sine_wave";
        case SIM_ALGO_LINEAR_DECAY:
            return "linear_decay";
        case SIM_ALGO_LINEAR_GROWTH:
            return "linear_growth";
        case SIM_ALGO_DEPENDENT_VALUE:
            return "dependent_value";
        default:
            return "unknown";
    }
}