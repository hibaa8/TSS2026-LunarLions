#include "sim_algorithms.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>

///////////////////////////////////////////////////////////////////////////////////
//                           Algorithm Implementations
///////////////////////////////////////////////////////////////////////////////////

/**
 * Sine wave algorithm for oscillating values.
 * 
 * @param field Pointer to the field containing algorithm parameters
 * @param current_time Current simulation time in seconds
 * @return Calculated value based on sine wave function
 */
sim_value_t sim_algo_sine_wave(sim_field_t* field, float current_time) {
    sim_value_t result = {0};
    
    if (!field || !field->params) return result;
    
    // Get parameters for the sine wave calculation
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

    result.f = sine_value;

    return result;
}


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

    result.f = current_value;

    return result;
}

/** 
* Linear decay algorithm for rapid decreasing values over time.
 * Values decrease linearly from start_value to end_value over duration_seconds.
 * 
 * @param field Pointer to the field containing algorithm parameters
 * @param current_time Current simulation time in seconds
 * @return Calculated value based on rapid linear decay
*/
sim_value_t sim_algo_rapid_linear_decay(sim_field_t* field, float current_time) {
    sim_value_t result = {0};
    
    if (!field || !field->params) return result;
    
    // Get parameters
    static float start_val = 0.0f; 

    if(!field->rapid_algo_initialized) {
        start_val = field->current_value.f; 
        field->rapid_algo_initialized = true;
    }

    cJSON* end_value = cJSON_GetObjectItem(field->params, "end_value");
    cJSON* rapid_duration = cJSON_GetObjectItem(field->params, "rapid_duration_seconds");

    float end_val = end_value && cJSON_IsNumber(end_value) ? (float)cJSON_GetNumberValue(end_value) : 0.0f;
    float rapid_duration_sec = rapid_duration && cJSON_IsNumber(rapid_duration) ? (float)cJSON_GetNumberValue(rapid_duration) : 1.0f;
    
    // Calculate current progress (0.0 to 1.0)
    float elapsed_time = current_time - field->start_time;
    printf("current_time: %.2f, start_time: %.2f, elapsed_time: %.2f\n", current_time, field->start_time, elapsed_time);
    float progress = elapsed_time / rapid_duration_sec;
    
    // Clamp progress between 0 and 1
    if (progress < 0.0f) progress = 0.0f;
    if (progress > 1.0f) progress = 1.0f;
    
    // Linear interpolation from start to end
    float current_value = start_val + (end_val - start_val) * progress;
    printf("Rapid linear decay - start_val: %.2f, end_val: %.2f, elapsed_time: %.2f, progress: %.2f, current_value: %.2f\n\n", start_val, end_val, elapsed_time, progress, current_value);
    result.f = current_value;

    return result;
}

/** 
* Linear growth algorithm for rapid increasing values over time.
 * Values increase linearly from start_value at growth_rate per second.
 * 
 * @param field Pointer to the field containing algorithm parameters
 * @param current_time Current simulation time in seconds
 * @return Calculated value based on rapid linear growth
*/
sim_value_t sim_algo_rapid_linear_growth(sim_field_t* field, float current_time) {
    sim_value_t result = {0};
    
    if (!field || !field->params) return result;
    
    // Get parameters
    static float start_val = 0.0f; 

    if(!field->rapid_algo_initialized) {
        start_val = field->current_value.f; 
        field->rapid_algo_initialized = true;
    }

    cJSON* rapid_growth_rate = cJSON_GetObjectItem(field->params, "rapid_growth_rate");
    cJSON* max_value = cJSON_GetObjectItem(field->params, "max_value");

    float rapid_rate = rapid_growth_rate && cJSON_IsNumber(rapid_growth_rate) ? (float)cJSON_GetNumberValue(rapid_growth_rate) : 1.0f;
    float max_val = max_value && cJSON_IsNumber(max_value) ? (float)cJSON_GetNumberValue(max_value) : INFINITY;

    // Calculate current value based on growth rate
    float elapsed_time = current_time - field->start_time;
    printf("current_time: %.2f, start_time: %.2f, elapsed_time: %.2f\n", current_time, field->start_time, elapsed_time);
    float current_value = start_val + (rapid_rate * elapsed_time);
    printf("Rapid linear growth - start_val: %.2f, rapid_rate: %.2f, elapsed_time: %.2f, current_value: %.2f\n\n", start_val, rapid_rate, elapsed_time, current_value);
    
    // Clamp to maximum value if specified
    if (current_value > max_val) {
        current_value = max_val;
    }

    result.f = current_value;

    return result;
}

/**
 * Linear growth algorithm for increasing values over time.
 * Values increase linearly from start_value at growth_rate per second.
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

    result.f = current_value;

    return result;
}

/**
 * Implements dependent value algorithm for fields calculated from other fields.
 * Evaluates a mathematical formula using values from other fields as inputs.
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

    result.f = calculated_value;

    return result;
}

/**
 * Implements error value algorithm for 
 * Evaluates a mathematical formula using values from other fields as inputs.
 * 
 * @param field Pointer to the field containing algorithm parameters
 * @param current_time Current simulation time in seconds (unused for dependent values)
 * @param engine Pointer to the simulation engine for accessing other field values
 * @return Calculated value based on formula evaluation
 */

/**
 * External value algorithm for fetching values from data JSON files.
 * Reads the specified field from data/{file_path}.
 *
 * @param field Pointer to the field containing algorithm parameters
 * @param current_time Current simulation time (unused for external values)
 * @param engine Pointer to the simulation engine
 * @return Value fetched from external JSON file
 */
sim_value_t sim_algo_external_value(sim_field_t* field, float current_time, sim_engine_t* engine) {
    sim_value_t result = {0};

    if (!field || !field->params || !engine) return result;

    // Get parameters
    cJSON* file_path_param = cJSON_GetObjectItem(field->params, "file_path");
    cJSON* field_path_param = cJSON_GetObjectItem(field->params, "field_path");

    if (!file_path_param || !cJSON_IsString(file_path_param)) {
        printf("Warning: No file_path specified for external_value field %s\n", field->field_name);
        return result;
    }

    if (!field_path_param || !cJSON_IsString(field_path_param)) {
        printf("Warning: No field_path specified for external_value field %s\n", field->field_name);
        return result;
    }

    const char* file_path = cJSON_GetStringValue(file_path_param);
    const char* field_path = cJSON_GetStringValue(field_path_param);

    // Construct full file path: data/{file_path}
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "data/%s", file_path);

    // Read and parse JSON file
    FILE* file = fopen(full_path, "r");
    if (!file) {
        printf("Warning: Cannot open external data file: %s\n", full_path);
        return result;
    }

    // Get file size and read content
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* json_string = malloc(file_size + 1);
    if (!json_string) {
        fclose(file);
        return result;
    }

    fread(json_string, 1, file_size, file);
    json_string[file_size] = '\0';
    fclose(file);

    // Parse JSON
    cJSON* root = cJSON_Parse(json_string);
    free(json_string);

    if (!root) {
        printf("Warning: Invalid JSON in external data file: %s\n", full_path);
        return result;
    }

    // Navigate through field path using dot notation (e.g., "telemetry.eva1.temperature")
    cJSON* current_obj = root;
    char* path_copy = strdup(field_path);
    char* token = strtok(path_copy, ".");

    while (token && current_obj) {
        current_obj = cJSON_GetObjectItem(current_obj, token);
        token = strtok(NULL, ".");
    }

    // Extract value
    if (current_obj) {
        if (cJSON_IsNumber(current_obj)) {
            double value = cJSON_GetNumberValue(current_obj);
            result.f = (float)value;
        } else if (cJSON_IsBool(current_obj)) {
            // Convert boolean to number (true = 1.0, false = 0.0)
            double value = cJSON_IsTrue(current_obj) ? 1.0 : 0.0;
            result.f = (float)value;
        } else {
            printf("Warning: Field '%s' in %s is not a number or boolean\n", field_path, full_path);
        }
    } else {
        printf("Warning: Could not find field '%s' in %s\n", field_path, full_path);
    }

    free(path_copy);
    cJSON_Delete(root);
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
 * Returns operator precedence level for order of operations.
 * Higher values indicate higher precedence.
 *
 * @param op Operator character (+, -, *, /)
 * @return Precedence level (3 for * /, 2 for + -, 0 for others)
 */
static int get_precedence(char op) {
    switch(op) {
        case '*':
        case '/':
            return 3;
        case '+':
        case '-':
            return 2;
        default:
            return 0;
    }
}

/**
 * Applies a binary operator to two operands
 *
 * @param op Operator character (+, -, *, /)
 * @param a Left operand
 * @param b Right operand
 * @return Result of applying operator to operands
 */
static float apply_operator(char op, float a, float b) {
    switch(op) {
        case '+':
            return a + b;
        case '-':
            return a - b;
        case '*':
            return a * b;
        case '/':
            return (b != 0.0f) ? a / b : 0.0f;
        default:
            return 0.0f;
    }
}

/**
 * Parses a token as either a numeric literal or field name lookup
 *
 * @param token Token string to parse
 * @param engine Simulation engine for field value lookup
 * @return Numeric value of token
 */
static float parse_token_value(const char* token, sim_engine_t* engine) {
    if (isdigit(token[0]) || (token[0] == '-' && isdigit(token[1]))) {
        return atof(token);
    }
    sim_value_t field_value = sim_engine_get_field_value(engine, token);
    return field_value.f;
}

/**
 * Evaluates a mathematical formula string using current field values.
 * Supports arithmetic operations (+, -, *, /) and parentheses.
 * Implements proper operator precedence (* / before + -) and parentheses grouping.
 * Example: "90.0 + (temperature - 21.1) * 0.36"
 *
 * @param formula String containing the mathematical formula to evaluate
 * @param engine Pointer to the simulation engine for field value lookup
 * @return Calculated result of the formula evaluation
 */
float sim_algo_evaluate_formula(const char* formula, sim_engine_t* engine) {
    if (!formula || !engine) return 0.0f;

    // Stacks for two-stack algorithm
    float value_stack[64];
    int value_top = -1;

    char op_stack[64];
    int op_top = -1;

    // Tokenize and process
    char* formula_copy = strdup(formula);
    char* token;
    char* rest = formula_copy;

    while ((token = strtok_r(rest, " ", &rest))) {
        // Skip commas
        if (strcmp(token, ",") == 0) {
            continue;
        }

        // Handle opening parenthesis
        if (token[0] == '(' && token[1] == '\0') {
            op_stack[++op_top] = '(';
            continue;
        }

        // Handle closing parenthesis
        if (token[0] == ')' && token[1] == '\0') {
            // Pop operators until '('
            while (op_top >= 0 && op_stack[op_top] != '(') {
                char op = op_stack[op_top--];
                if (value_top < 1) break;  // Safety check
                float b = value_stack[value_top--];
                float a = value_stack[value_top--];
                value_stack[++value_top] = apply_operator(op, a, b);
            }

            // Remove '(' from stack
            if (op_top >= 0 && op_stack[op_top] == '(') {
                op_top--;
            }
            continue;
        }

        // Handle operators
        if (strlen(token) == 1 && strchr("+-*/", token[0])) {
            char op = token[0];
            int prec = get_precedence(op);

            // Pop higher or equal precedence operators
            while (op_top >= 0 && op_stack[op_top] != '(' &&
                   get_precedence(op_stack[op_top]) >= prec) {
                char prev_op = op_stack[op_top--];
                if (value_top < 1) break;  // Safety check
                float b = value_stack[value_top--];
                float a = value_stack[value_top--];
                value_stack[++value_top] = apply_operator(prev_op, a, b);
            }

            op_stack[++op_top] = op;
            continue;
        }

        // Handle numbers and field names
        float value = parse_token_value(token, engine);
        value_stack[++value_top] = value;
    }

    // Pop remaining operators
    while (op_top >= 0) {
        char op = op_stack[op_top--];
        if (op == '(') continue;  // Skip unmatched parens
        if (value_top < 1) break;  // Safety check
        float b = value_stack[value_top--];
        float a = value_stack[value_top--];
        value_stack[++value_top] = apply_operator(op, a, b);
    }

    free(formula_copy);
    return value_top >= 0 ? value_stack[value_top] : 0.0f;
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
    } else if (strcmp(algo_string, "external_value") == 0) {
        return SIM_ALGO_EXTERNAL_VALUE;
    }
    
    return SIM_ALGO_SINE_WAVE;  // Default algorithm
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
        case SIM_ALGO_EXTERNAL_VALUE:
            return "external_value";
        default:
            return "unknown";
    }
}