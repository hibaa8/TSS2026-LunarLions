#include "sim_engine.h"
#include "sim_algorithms.h"
#include <string.h>
#include <math.h>
#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#endif

///////////////////////////////////////////////////////////////////////////////////
//                            Engine Lifecycle
///////////////////////////////////////////////////////////////////////////////////

/**
 * Creates and initializes a new simulation engine instance.
 * 
 * @return Pointer to new simulation engine, or NULL if creation failed
 */
sim_engine_t* sim_engine_create(void) {
    sim_engine_t* engine = calloc(1, sizeof(sim_engine_t));
    if (!engine) return NULL;
    
    engine->initialized = false;
    
    return engine;
}

/**
 * Destroys a simulation engine instance and frees all associated memory.
 * 
 * @param engine Pointer to the simulation engine to destroy
 */
void sim_engine_destroy(sim_engine_t* engine) {
    if (!engine) return;
    
    // Free components and fields
    for (int i = 0; i < engine->component_count; i++) {
        sim_component_t* comp = &engine->components[i];
        
        for (int j = 0; j < comp->field_count; j++) {
            sim_field_t* field = &comp->fields[j];
            
            free(field->field_name);
            free(field->component_name);
            
            if (field->params) {
                cJSON_Delete(field->params);
            }
            
            // Free dependency array
            for (int k = 0; k < field->depends_count; k++) {
                free(field->depends_on[k]);
            }
            free(field->depends_on);
        }
        
        free(comp->component_name);
        free(comp->fields);
    }
    
    free(engine->components);
    free(engine->update_order);
    free(engine);
}

///////////////////////////////////////////////////////////////////////////////////
//                         Configuration Loading
///////////////////////////////////////////////////////////////////////////////////

/**
 * Loads predefined JSON simulation configuration files.
 * Uses a hardcoded list of configuration files instead of scanning directories.
 * 
 * @param engine Pointer to the simulation engine
 * @return true if all files were loaded successfully, false otherwise
 */
bool sim_engine_load_predefined_configs(sim_engine_t* engine) {
    if (!engine) return false;
    
    // Predefined list of configuration files to load
    // @TODO can generalize this more in some way?
    const char* config_files[] = {
        "src/lib/simulation/config/eva1.json",
        "src/lib/simulation/config/eva2.json",
        "src/lib/simulation/config/rover.json"
    };
    const int config_count = sizeof(config_files) / sizeof(config_files[0]);
    
    bool success = true;
    int loaded_count = 0;
    
    // Load each configuration file in a loop
    for (int i = 0; i < config_count; i++) {
        if (sim_engine_load_component(engine, config_files[i])) {
            loaded_count++;
        } else {
            printf("Warning: Failed to load component file: %s\n", config_files[i]);
            success = false;
        }
    }
    
    if (loaded_count == 0) {
        printf("Error: No configuration files were loaded successfully.\n");
        return false;
    }
    
    return success;
}

/**
 * Loads a single JSON simulation component configuration file.
 * Parses the JSON and adds the component and its fields to the engine.
 * 
 * @param engine Pointer to the simulation engine
 * @param json_file_path Path to the JSON configuration file to load
 * @return true if the file was loaded successfully, false otherwise
 */
bool sim_engine_load_component(sim_engine_t* engine, const char* json_file_path) {
    if (!engine || !json_file_path) return false;
    
    // Read JSON file
    FILE* file = fopen(json_file_path, "r");
    if (!file) {
        printf("Error: Cannot open file: %s\n", json_file_path);
        return false;
    }
    
    // Get the file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Read the entire JSON file into a string
    char* json_string = malloc(file_size + 1);
    fread(json_string, 1, file_size, file);
    json_string[file_size] = '\0';
    fclose(file);
    
    // Parse the string as JSON
    cJSON* root = cJSON_Parse(json_string);
    free(json_string);
    
    if (!root) {
        printf("Error: Invalid JSON in file: %s\n", json_file_path);
        return false;
    }
    
    // Extract component name I.E. "eva", "rover"
    cJSON* component_name_json = cJSON_GetObjectItem(root, "component_name");
    if (!component_name_json || !cJSON_IsString(component_name_json)) {
        printf("Error: Missing component_name in file: %s\n", json_file_path);
        cJSON_Delete(root);
        return false;
    }
    
    const char* component_name = cJSON_GetStringValue(component_name_json);
    
    // Get fields object
    cJSON* fields = cJSON_GetObjectItem(root, "fields");
    if (!fields || !cJSON_IsObject(fields)) {
        printf("Error: Missing or invalid fields object in file: %s\n", json_file_path);
        cJSON_Delete(root);
        return false;
    }
    
    // Count fields
    int field_count = cJSON_GetArraySize(fields);
    if (field_count == 0) {
        printf("Warning: No fields found in component: %s\n", component_name);
        cJSON_Delete(root);
        return true;
    }
    
    // Expand components array
    engine->components = realloc(engine->components, 
        (engine->component_count + 1) * sizeof(sim_component_t));
    
    sim_component_t* component = &engine->components[engine->component_count];
    component->component_name = strdup(component_name);
    component->field_count = field_count;
    component->fields = calloc(field_count, sizeof(sim_field_t));
    component->running = false;  // Start stopped by default
    component->simulation_time = 0.0f;  // Initialize component simulation time
    
    // Parse fields
    int field_idx = 0;
    cJSON* field_json = NULL;
    cJSON_ArrayForEach(field_json, fields) {
        sim_field_t* field = &component->fields[field_idx];
        
        field->field_name = strdup(field_json->string);
        field->component_name = strdup(component_name);
        
        // Parse algorithm
        cJSON* algorithm = cJSON_GetObjectItem(field_json, "algorithm");
        if (!algorithm || !cJSON_IsString(algorithm)) {
            printf("Error: Missing algorithm for field %s\n", field->field_name);
            continue;
        }
        
        field->algorithm = sim_algo_parse_type_string(cJSON_GetStringValue(algorithm));
        
        // Parse type (default to float if not specified)
        cJSON* type = cJSON_GetObjectItem(field_json, "type");
        if (type && cJSON_IsString(type)) {
            const char* type_str = cJSON_GetStringValue(type);
            if (strcmp(type_str, "float") == 0) {
                field->type = SIM_TYPE_FLOAT;
            } else if (strcmp(type_str, "int") == 0) {
                field->type = SIM_TYPE_INT;
            } else {
                field->type = SIM_TYPE_FLOAT;  // Default
            }
        } else {
            field->type = SIM_TYPE_FLOAT;  // Default
        }
        
        // Clone parameters for algorithm use
        field->params = cJSON_Duplicate(field_json, true);
        
        // Parse dependencies
        cJSON* depends_on = cJSON_GetObjectItem(field_json, "depends_on");
        if (depends_on && cJSON_IsArray(depends_on)) {
            field->depends_count = cJSON_GetArraySize(depends_on);
            field->depends_on = malloc(field->depends_count * sizeof(char*));
            
            for (int i = 0; i < field->depends_count; i++) {
                cJSON* dep = cJSON_GetArrayItem(depends_on, i);
                if (cJSON_IsString(dep)) {
                    field->depends_on[i] = strdup(cJSON_GetStringValue(dep));
                }
            }
        } else {
            field->depends_count = 0;
            field->depends_on = NULL;
        }

        field->initialized = false;
        field_idx++;
    }
    
    engine->component_count++;
    engine->total_field_count += field_count;
    
    cJSON_Delete(root);
    return true;
}

///////////////////////////////////////////////////////////////////////////////////
//                            Dependency Sorting
///////////////////////////////////////////////////////////////////////////////////

/**
 * Checks if a field has any unresolved dependencies that do not exist in the config files.
 * 
 * @param field The field to check for dependencies
 * @param resolved_fields Array of fields that have already been resolved
 * @param resolved_count Number of fields in the resolved_fields array
 * @return true if the field has unresolved dependencies, false otherwise
 */
static bool has_unresolved_dependencies(sim_field_t* field, sim_field_t** resolved_fields, int resolved_count) {
    for (int i = 0; i < field->depends_count; i++) {
        bool found = false;
        for (int j = 0; j < resolved_count; j++) {
            if (strcmp(field->depends_on[i], resolved_fields[j]->field_name) == 0) {
                found = true;
                break;
            }
        }
        if (!found) return true;
    }
    return false;
}

/**
 * Sorts all fields in the engine by their dependencies using topological sorting.
 * Fields with no dependencies are placed first, followed by fields that depend on them.
 * This ensures fields are updated in the correct order during simulation.
 * 
 * @param engine Pointer to the simulation engine
 * @return true if sorting was successful, false if circular dependencies were detected
 */
static bool sort_fields_by_dependencies(sim_engine_t* engine) {
    engine->update_order = malloc(engine->total_field_count * sizeof(sim_field_t*));
    sim_field_t** all_fields = malloc(engine->total_field_count * sizeof(sim_field_t*));
    
    // Collect all fields
    int field_idx = 0;
    for (int i = 0; i < engine->component_count; i++) {
        for (int j = 0; j < engine->components[i].field_count; j++) {
            all_fields[field_idx++] = &engine->components[i].fields[j];
        }
    }
    
    // Topological sort
    int resolved_count = 0;
    int iterations = 0;
    const int max_iterations = engine->total_field_count * 2;
    
    while (resolved_count < engine->total_field_count && iterations < max_iterations) {
        bool progress = false;
        
        for (int i = 0; i < engine->total_field_count; i++) {
            sim_field_t* field = all_fields[i];
            
            // Skip already resolved fields
            bool already_resolved = false;
            for (int j = 0; j < resolved_count; j++) {
                if (engine->update_order[j] == field) {
                    already_resolved = true;
                    break;
                }
            }
            if (already_resolved) continue;
            
            // Check if all dependencies are resolved
            if (!has_unresolved_dependencies(field, engine->update_order, resolved_count)) {
                engine->update_order[resolved_count++] = field;
                progress = true;
            }
        }
        
        if (!progress) {
            printf("Error: Circular dependency detected in simulation fields\n");
            free(all_fields);
            return false;
        }
        
        iterations++;
    }
    
    free(all_fields);
    return resolved_count == engine->total_field_count;
}

///////////////////////////////////////////////////////////////////////////////////
//                           Simulation Control
///////////////////////////////////////////////////////////////////////////////////

/**
 * Initializes the simulation engine after all components have been loaded.
 * Sorts fields by dependencies, sets initial values, and prepares for simulation.
 * Must be called after loading all configuration files and before updating.
 * 
 * @param engine Pointer to the simulation engine
 * @return true if initialization was successful, false otherwise
 */
bool sim_engine_initialize(sim_engine_t* engine) {
    if (!engine || engine->initialized) return false;
    
    // Sort fields by dependencies
    if (!sort_fields_by_dependencies(engine)) {
        return false;
    }
    
    // Initialize all fields
    for (int i = 0; i < engine->total_field_count; i++) {
        sim_field_t* field = engine->update_order[i];

        // Find the component this field belongs to and use its simulation time
        sim_component_t* component = NULL;
        for (int j = 0; j < engine->component_count; j++) {
            if (strcmp(engine->components[j].component_name, field->component_name) == 0) {
                component = &engine->components[j];
                break;
            }
        }

        field->start_time = component ? component->simulation_time : 0.0f;
        field->initialized = true;
        
        // Set initial values based on algorithm
        switch (field->algorithm) {
            case SIM_ALGO_SINE_WAVE: {
                cJSON* base_value = cJSON_GetObjectItem(field->params, "base_value");
                if (base_value && cJSON_IsNumber(base_value)) {
                    field->current_value.f = (float)cJSON_GetNumberValue(base_value);
                }
                break;
            }
            case SIM_ALGO_LINEAR_DECAY: {
                cJSON* start_value = cJSON_GetObjectItem(field->params, "start_value");
                if (start_value && cJSON_IsNumber(start_value)) {
                    field->current_value.f = (float)cJSON_GetNumberValue(start_value);
                }
                break;
            }
            case SIM_ALGO_LINEAR_GROWTH: {
                cJSON* start_value = cJSON_GetObjectItem(field->params, "start_value");
                if (start_value && cJSON_IsNumber(start_value)) {
                    field->current_value.f = (float)cJSON_GetNumberValue(start_value);
                } else {
                    field->current_value.f = 0.0f;
                }
                break;
            }
            case SIM_ALGO_DEPENDENT_VALUE: {
                // Will be calculated during first update
                field->current_value.f = 0.0f;
                break;
            }
        }
        
        field->previous_value = field->current_value;
    }
    
    engine->initialized = true;
    return true;
}

/**
 * Updates the simulation by one time step.
 * Advances simulation time and updates all fields in dependency order.
 * Call this regularly to advance the simulation.
 * 
 * @param engine Pointer to the simulation engine
 * @param delta_time Time elapsed since last update (in seconds)
 */
void sim_engine_update(sim_engine_t* engine, float delta_time) {
    if (!engine || !engine->initialized) return;

    // First, advance simulation time for all running components
    for (int i = 0; i < engine->component_count; i++) {
        if (engine->components[i].running) {
            engine->components[i].simulation_time += delta_time;
        }
    }

    // Update all fields in dependency order (only for running components)
    for (int i = 0; i < engine->total_field_count; i++) {
        sim_field_t* field = engine->update_order[i];

        // Find the component this field belongs to
        sim_component_t* component = NULL;
        for (int j = 0; j < engine->component_count; j++) {
            if (strcmp(engine->components[j].component_name, field->component_name) == 0) {
                component = &engine->components[j];
                break;
            }
        }

        // Only update if component is running
        if (!component || !component->running) continue;

        field->previous_value = field->current_value;

        switch (field->algorithm) {
            case SIM_ALGO_SINE_WAVE:
                field->current_value = sim_algo_sine_wave(field, component->simulation_time);
                break;
            case SIM_ALGO_LINEAR_DECAY:
                field->current_value = sim_algo_linear_decay(field, component->simulation_time);
                break;
            case SIM_ALGO_LINEAR_GROWTH:
                field->current_value = sim_algo_linear_growth(field, component->simulation_time);
                break;
            case SIM_ALGO_DEPENDENT_VALUE:
                field->current_value = sim_algo_dependent_value(field, component->simulation_time, engine);
                break;
        }
    }
}

/**
 * Starts simulation updates for a specific component.
 *
 * @param engine Pointer to the simulation engine
 * @param component_name Name of the component to start (e.g., "rover", "eva1", "eva2")
 */
void sim_engine_start_component(sim_engine_t* engine, const char* component_name) {
    if (!engine || !engine->initialized || !component_name) return;

    for (int i = 0; i < engine->component_count; i++) {
        if (strcmp(engine->components[i].component_name, component_name) == 0) {
            engine->components[i].running = true;
            printf("Started component '%s' simulation\n", component_name);
            return;
        }
    }

    printf("Warning: Component '%s' not found\n", component_name);
}

/**
 * Stops simulation updates for a specific component.
 *
 * @param engine Pointer to the simulation engine
 * @param component_name Name of the component to stop (e.g., "rover", "eva1", "eva2")
 */
void sim_engine_stop_component(sim_engine_t* engine, const char* component_name) {
    if (!engine || !component_name) return;

    for (int i = 0; i < engine->component_count; i++) {
        if (strcmp(engine->components[i].component_name, component_name) == 0) {
            engine->components[i].running = false;
            printf("Stopped component '%s' simulation\n", component_name);
            return;
        }
    }

    printf("Warning: Component '%s' not found\n", component_name);
}

/**
 * Resets all fields of a specific component to their initial state and stops the component.
 *
 * @param engine Pointer to the simulation engine
 * @param component_name Name of the component to reset (e.g., "rover", "eva1", "eva2")
 */
void sim_engine_reset_component(sim_engine_t* engine, const char* component_name) {
    if (!engine || !engine->initialized || !component_name) return;

    // Find and stop the component
    sim_component_t* target_component = NULL;
    for (int i = 0; i < engine->component_count; i++) {
        if (strcmp(engine->components[i].component_name, component_name) == 0) {
            target_component = &engine->components[i];
            target_component->running = false;
            target_component->simulation_time = 0.0f;  // Reset component simulation time
            break;
        }
    }

    if (!target_component) {
        printf("Warning: Component '%s' not found\n", component_name);
        return;
    }

    // Reset all fields of this component
    for (int i = 0; i < engine->total_field_count; i++) {
        sim_field_t* field = engine->update_order[i];
        if (field && strcmp(field->component_name, component_name) == 0) {
            // Reset field timing to component time (which is now 0)
            field->start_time = target_component->simulation_time;

            // Set initial values based on algorithm
            switch (field->algorithm) {
                case SIM_ALGO_SINE_WAVE: {
                    cJSON* base_value = cJSON_GetObjectItem(field->params, "base_value");
                    if (base_value && cJSON_IsNumber(base_value)) {
                        field->current_value.f = (float)cJSON_GetNumberValue(base_value);
                    }
                    break;
                }
                case SIM_ALGO_LINEAR_DECAY: {
                    cJSON* start_value = cJSON_GetObjectItem(field->params, "start_value");
                    if (start_value && cJSON_IsNumber(start_value)) {
                        field->current_value.f = (float)cJSON_GetNumberValue(start_value);
                    }
                    break;
                }
                case SIM_ALGO_LINEAR_GROWTH: {
                    cJSON* start_value = cJSON_GetObjectItem(field->params, "start_value");
                    if (start_value && cJSON_IsNumber(start_value)) {
                        field->current_value.f = (float)cJSON_GetNumberValue(start_value);
                    } else {
                        field->current_value.f = 0.0f;
                    }
                    break;
                }
                case SIM_ALGO_DEPENDENT_VALUE: {
                    // Will be calculated during first update
                    field->current_value.f = 0.0f;
                    break;
                }
            }

            field->previous_value = field->current_value;
        }
    }

    printf("Reset component '%s' simulation\n", component_name);
}

///////////////////////////////////////////////////////////////////////////////////
//                              Field Access
///////////////////////////////////////////////////////////////////////////////////

/**
 * Finds a field by name across all components.
 * Searches through all loaded components to find a field with the given name.
 * 
 * @param engine Pointer to the simulation engine
 * @param field_name Name of the field to find
 * @return Pointer to the field if found, NULL otherwise
 */
sim_field_t* sim_engine_find_field(sim_engine_t* engine, const char* field_name) {
    if (!engine || !field_name) return NULL;
    
    for (int i = 0; i < engine->component_count; i++) {
        for (int j = 0; j < engine->components[i].field_count; j++) {
            if (strcmp(engine->components[i].fields[j].field_name, field_name) == 0) {
                return &engine->components[i].fields[j];
            }
        }
    }
    
    return NULL;
}

/**
 * Gets the current value of a field by name.
 * 
 * @param engine Pointer to the simulation engine
 * @param field_name Name of the field to get the value for
 * @return The current value of the field, or a zero-initialized value if not found
 */
sim_value_t sim_engine_get_field_value(sim_engine_t* engine, const char* field_name) {
    sim_value_t empty = {0};
    
    sim_field_t* field = sim_engine_find_field(engine, field_name);
    if (!field) return empty;
    
    return field->current_value;
}

/**
 * Checks if a specific component is currently running.
 *
 * @param engine Pointer to the simulation engine
 * @param component_name Name of the component to check
 * @return true if the component is running, false otherwise
 */
bool sim_engine_is_component_running(sim_engine_t* engine, const char* component_name) {
    if (!engine || !component_name) return false;

    for (int i = 0; i < engine->component_count; i++) {
        if (strcmp(engine->components[i].component_name, component_name) == 0) {
            return engine->components[i].running;
        }
    }
    return false;
}