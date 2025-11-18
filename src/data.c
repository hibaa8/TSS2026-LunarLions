#include "data.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

///////////////////////////////////////////////////////////////////////////////////
//                        Backend Lifecycle Management
///////////////////////////////////////////////////////////////////////////////////

/**
 * Initializes the backend data structure and simulation engine
 *
 * @return Pointer to the initialized backend data structure
 */
struct backend_data_t *init_backend() {
    // Allocate memory for backend
    struct backend_data_t *backend = malloc(sizeof(struct backend_data_t));
    memset(backend, 0, sizeof(struct backend_data_t));

    // Set initial timing information
    backend->start_time = time(NULL);
    backend->running_pr_sim = -1;
    backend->pr_sim_paused = false;

    // Initialize simulation engine
    backend->sim_engine = sim_engine_create();
    if (backend->sim_engine) {
        if (!sim_engine_load_predefined_configs(backend->sim_engine)) {
            printf("Warning: Failed to load simulation configurations\n");
        }

        if (!sim_engine_initialize(backend->sim_engine)) {
            printf("Warning: Failed to initialize simulation engine\n");
        }
    } else {
        printf("Warning: Failed to create simulation engine\n");
    }

    printf("Backend and simulation engine initialized successfully\n");

    return backend;
}

/**
 * Calls the simulation engine to update all telemetry data based on elapsed time
 *
 * @param backend Backend data structure containing all telemetry and simulation engines
 */
void increment_simulation(struct backend_data_t *backend) {
    // Increment server time
    int new_time = time(NULL) - backend->start_time;
    bool time_incremented = false;
    if (new_time != backend->server_up_time) {
        backend->server_up_time = new_time;
        time_incremented = true;
    }

    // Update simulation engine once per second
    if (time_incremented) {
        // Update simulation engine with elapsed time
        float delta_time = 1.0f;  // 1 second per update

        if (backend->sim_engine) {
            sim_engine_update(backend->sim_engine, delta_time);
        }

        // Update EVA station timing
        update_eva_station_timing();
    }
}

/**
 * Cleans up backend data structure and frees resources from memory, called in server.c
 * 
 * @param backend Backend data structure to cleanup
 */
void cleanup_backend(struct backend_data_t *backend) {
    if (!backend) return;

    // Cleanup simulation engine
    if (backend->sim_engine) {
        sim_engine_destroy(backend->sim_engine);
    }

    // Free backend data structure
    free(backend);
}


///////////////////////////////////////////////////////////////////////////////////
//                             UDP Request Handlers
///////////////////////////////////////////////////////////////////////////////////


/**
 * Handles UDP GET requests for data retrieval
 * 
 * @param command Command identifier for the GET request
 * @param data Response buffer to populate with requested data
 * @param backend Backend data structure containing all telemetry and simulation engines
 */
void handle_udp_get_request(unsigned int command, unsigned char* data, struct backend_data_t* backend) {
    // Handle different GET requests
    switch (command) {
        case 0: // ROVER telemetry
            printf("Getting ROVER telemetry data.\n");
            send_json_file("ROVER", data);
            break;
        case 1: // EVA telemetry
            printf("Getting EVA telemetry data.\n");
            send_json_file("EVA", data);
            break;
        case 2: // LTV data
            printf("Getting LTV telemetry data.\n");
            send_json_file("LTV", data);
            break;


        default:
            printf("Invalid GET command: %u\n", command);
            break;
    }
}

/**
 * Handles UDP POST requests for data updates using command-to-path mapping
 *
 * @param command Command identifier for the POST request
 * @param data Request buffer containing data to update
 * @param backend Backend data structure containing all telemetry and simulation engines
 * 
 * @return true if the update was successful, false otherwise
 */
bool handle_udp_post_request(unsigned int command, unsigned char* data, struct backend_data_t* backend) {
    // Find the mapping for this command
    const udp_command_mapping_t* mapping = NULL;
    for (int i = 0; udp_command_mappings[i].path != NULL; i++) {
        if (udp_command_mappings[i].command == command) {
            mapping = &udp_command_mappings[i];
            break;
        }
    }

    if (!mapping) {
        printf("Invalid UDP POST command: %u\n", command);
        return false;
    }
    
    // Extract value from UDP data
    char value_str[32];

    if (mapping->data_type == "bool") {
        // Special case for Unreal Engine commands where bool is sent as float
        float bool_float;
        memcpy(&bool_float, data, 4);
        bool value = bool_float != 0.0f;
        strcpy(value_str, value ? "true" : "false");
    } else if (mapping->data_type == "array<float>") {
        // Special case for array<float> type (like LIDAR), copies as an array of floats 
        float lidar_data[10];
        memcpy(lidar_data, data, sizeof(lidar_data));
    } else {
        // Other commands extract float value from packet format
        if (strcmp(mapping->data_type, "bool") == 0) {
            bool value = extract_bool_value(data);
            strcpy(value_str, value ? "true" : "false");
        } else if (strcmp(mapping->data_type, "float") == 0) {
            float value = extract_float_value(data);
            sprintf(value_str, "%.6f", value);
        }
    }

    // Create request content in the same format as HTML forms
    char request_content[256];
    sprintf(request_content, "%s=%s", mapping->path, value_str);

    // Reuse existing html_form_json_update function for all the JSON and simulation logic
    printf("Processing UDP command %u: %s = %s\n", command, mapping->path, value_str);

    bool result = html_form_json_update(request_content, backend);

    return result;
}

///////////////////////////////////////////////////////////////////////////////////
//                              Data Management
///////////////////////////////////////////////////////////////////////////////////

/**
 * Updates a field within the specified JSON file (supports both simple and nested field paths)
 * 
 * @param filename Name of the JSON file to update (e.g., "EVA")
 * @param section Section within the JSON file to update (e.g., "telemetry")
 * @param field_path Field path within the section to update (e.g., "batt_time_left" or "eva1.batt")
 * @param new_value New value to set for the specified field
 */
void update_json_file(const char* filename, const char* section, const char* field_path, char* new_value) {
    // Construct the file path
    char file_path[100];
    snprintf(file_path, sizeof(file_path), "data/%s.json", filename);

    // Read existing JSON file
    FILE *fp = fopen(file_path, "r");
    if (fp == NULL) {
        printf("Error: Unable to open the file %s for reading.\n", file_path);
        return;
    }

    fseek(fp, 0L, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);

    char *file_buffer = malloc(file_size + 1);
    fread(file_buffer, 1, file_size, fp);
    file_buffer[file_size] = '\0'; // Null-terminate the buffer
    fclose(fp);

    // Parse the JSON content
    cJSON *json = cJSON_Parse(file_buffer);
    free(file_buffer);

    if (json == NULL) {
        printf("Error: Failed to parse JSON from file %s.\n", file_path);
        return;
    }

    // Navigate to the specified section
    cJSON *section_json = cJSON_GetObjectItemCaseSensitive(json, section);
    if (section_json == NULL) {
        printf("Error: Section %s not found in JSON.\n", section);
        cJSON_Delete(json);
        return;
    }

    // Parse the field path and navigate through it (supports both simple and nested paths)
    char field_path_copy[256];
    strncpy(field_path_copy, field_path, sizeof(field_path_copy) - 1);
    field_path_copy[sizeof(field_path_copy) - 1] = '\0';
    
    cJSON *current_object = section_json;
    char* field_parts[10];
    int field_part_count = 0;
    
    // Split field path by dots
    char* token = strtok(field_path_copy, ".");
    while (token != NULL && field_part_count < 10) {
        field_parts[field_part_count++] = token;
        token = strtok(NULL, ".");
    }
    
    // Navigate through all but the last field part
    for (int i = 0; i < field_part_count - 1; i++) {
        cJSON *next_object = cJSON_GetObjectItemCaseSensitive(current_object, field_parts[i]);
        if (next_object == NULL) {
            printf("Error: Field path %s not found at level %s.\n", field_path, field_parts[i]);
            cJSON_Delete(json);
            return;
        }
        current_object = next_object;
    }
    
    // Update the final field
    const char* final_field = field_parts[field_part_count - 1];
    
    // Determine value type and create appropriate JSON value
    cJSON* new_json_value = NULL;
    if (strcmp(new_value, "true") == 0) {
        new_json_value = cJSON_CreateTrue();
    } else if (strcmp(new_value, "false") == 0) {
        new_json_value = cJSON_CreateFalse();
    } else {
        // Try to parse as number
        char* endptr;
        double num_value = strtod(new_value, &endptr);
        if (*endptr == '\0') {
            // It's a valid number
            new_json_value = cJSON_CreateNumber(num_value);
        } else {
            // Treat as string
            new_json_value = cJSON_CreateString(new_value);
        }
    }
    
    cJSON_ReplaceItemInObject(current_object, final_field, new_json_value);

    // Write updated JSON back to file
    char *json_str = cJSON_Print(json);
    if (json_str == NULL) {
        printf("Error: Failed to print JSON to string.\n");
        cJSON_Delete(json);
        return;
    }

    fp = fopen(file_path, "w");
    if (fp == NULL) {
        printf("Error: Unable to open the file %s for writing.\n", file_path);
        free(json_str);
        cJSON_Delete(json);
        return;
    }

    fputs(json_str, fp);
    fclose(fp);

    free(json_str);
    cJSON_Delete(json);
}

/**
 * Loads and returns a cJSON object from the specified JSON file
 *
 * @param filename Name of the JSON file to load (e.g., "EVA")
 * @return Pointer to the cJSON object representing the file content, or NULL on failure
 */
cJSON* get_json_file(const char* filename) {
    // Construct the file path
    char file_path[100];
    snprintf(file_path, sizeof(file_path), "data/%s.json", filename);

    // Read existing JSON file
    FILE *fp = fopen(file_path, "r");
    if (fp == NULL) {
        printf("Error: Unable to open the file %s for reading.\n", file_path);
        return NULL;
    }

    fseek(fp, 0L, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);

    char *file_buffer = malloc(file_size + 1);
    fread(file_buffer, 1, file_size, fp);
    file_buffer[file_size] = '\0'; // Null-terminate the buffer
    fclose(fp);

    // Parse the JSON content
    cJSON *json = cJSON_Parse(file_buffer);
    free(file_buffer);

    if (json == NULL) {
        printf("Error: Failed to parse JSON from file %s.\n", file_path);
        return NULL;
    }

    return json;
}

/**
 * Sends the entire JSON file content as response data
 *
 * @param filename Name of the JSON file (e.g., "EVA")
 * @param data Response buffer to populate with JSON string
 */
void send_json_file(const char* filename, unsigned char* data) {
    cJSON* json = get_json_file(filename);
    if (json == NULL) {
        printf("Error: Could not load JSON file %s\n", filename);
        return;
    }
    
    // Convert JSON to string
    char* json_str = cJSON_Print(json);
    if (json_str == NULL) {
        printf("Error: Failed to convert JSON to string\n");
        cJSON_Delete(json);
        return;
    }
    
    // Copy JSON string to response data buffer
    size_t json_len = strlen(json_str);
    memcpy(data, json_str, json_len);
    data[json_len] = '\0'; // Null terminate
    
    // Cleanup
    free(json_str);
    cJSON_Delete(json);
}


/**
 * Synchronizes the simulation engine data to the corresponding JSON files
 *
 * @param backend Backend data structure containing telemetry and simulation engine
 */
void sync_simulation_to_json(struct backend_data_t* backend) {
    sim_engine_t* engine = backend->sim_engine;

    if (engine == NULL) {
        printf("Error: Simulation engine is NULL.\n");
        return;
    }

    // Load EVA JSON file
    cJSON* root = get_json_file("EVA");
    if (root == NULL) {
        printf("Error: Could not load EVA.json\n");
        return;
    }
    
    // Get or create the status section and update existing started field
    cJSON* status = cJSON_GetObjectItemCaseSensitive(root, "status");
    if (status == NULL) {
        status = cJSON_CreateObject();
        cJSON_AddItemToObject(root, "status", status);
    }

    // Update existing started field (check if eva1 OR eva2 is running)
    bool eva_running = sim_engine_is_component_running(engine, "eva1") || sim_engine_is_component_running(engine, "eva2");
    cJSON* started_field = cJSON_GetObjectItemCaseSensitive(status, "started");
    if (started_field != NULL) {
        cJSON_SetBoolValue(started_field, eva_running);
    } else {
        cJSON_AddBoolToObject(status, "started", eva_running);
    }

    // Get or create the telemetry section
    cJSON* telemetry = cJSON_GetObjectItemCaseSensitive(root, "telemetry");
    if (telemetry == NULL) {
        telemetry = cJSON_CreateObject();
        cJSON_AddItemToObject(root, "telemetry", telemetry);
    }
    
    // Get or create eva1 and eva2 sections under telemetry
    cJSON* eva1_section = cJSON_GetObjectItemCaseSensitive(telemetry, "eva1");
    if (eva1_section == NULL) {
        eva1_section = cJSON_CreateObject();
        cJSON_AddItemToObject(telemetry, "eva1", eva1_section);
    }
    
    cJSON* eva2_section = cJSON_GetObjectItemCaseSensitive(telemetry, "eva2");
    if (eva2_section == NULL) {
        eva2_section = cJSON_CreateObject();
        cJSON_AddItemToObject(telemetry, "eva2", eva2_section);
    }
    
    // Update simulation fields in their respective sections
    for (int i = 0; i < engine->total_field_count; i++) {
        sim_field_t* field = engine->update_order[i];
        if (field != NULL) {
            double value = (field->type == SIM_TYPE_FLOAT) ? field->current_value.f : (double)field->current_value.i;
            
            // Determine target section based on component name
            cJSON* target_section = NULL;
            if (strcmp(field->component_name, "eva1") == 0) {
                target_section = eva1_section;
            } else if (strcmp(field->component_name, "eva2") == 0) {
                target_section = eva2_section;
            }
    
            if (target_section != NULL) {
                // Check if field already exists and replace it, otherwise add new
                cJSON* existing_field = cJSON_GetObjectItemCaseSensitive(target_section, field->field_name);
                if (existing_field != NULL) {
                    cJSON_SetNumberValue(existing_field, value);
                } else {
                    cJSON_AddNumberToObject(target_section, field->field_name, value);
                }
            }
        }
    }
    
    // Write EVA file
    char filepath[100];
    snprintf(filepath, sizeof(filepath), "data/EVA.json");
    
    char* json_str = cJSON_Print(root);
    FILE* fp = fopen(filepath, "w");
    if (fp) {
        fputs(json_str, fp);
        fclose(fp);
    }
    
    free(json_str);
    cJSON_Delete(root);
    
    // Now sync rover data to ROVER.json
    cJSON* rover_root = get_json_file("ROVER");
    if (rover_root == NULL) {
        printf("Error: Could not load ROVER.json\n");
        return;
    }
    
    // Get or create the pr_telemetry section
    cJSON* pr_telemetry = cJSON_GetObjectItemCaseSensitive(rover_root, "pr_telemetry");
    if (pr_telemetry == NULL) {
        pr_telemetry = cJSON_CreateObject();
        cJSON_AddItemToObject(rover_root, "pr_telemetry", pr_telemetry);
    }

    // Update simulation running status in pr_telemetry (check if rover is running)
    bool rover_running = sim_engine_is_component_running(engine, "rover");
    cJSON* rover_sim_running_field = cJSON_GetObjectItemCaseSensitive(pr_telemetry, "sim_running");
    if (rover_sim_running_field != NULL) {
        cJSON_SetBoolValue(rover_sim_running_field, rover_running);
    } else {
        cJSON_AddBoolToObject(pr_telemetry, "sim_running", rover_running);
    }
    
    // Update rover simulation fields (skip external_value fields as they are inputs, not outputs)
    for (int i = 0; i < engine->total_field_count; i++) {
        sim_field_t* field = engine->update_order[i];
        if (field != NULL && strcmp(field->component_name, "rover") == 0) {
            // Skip external_value fields - these are inputs from JSON, not outputs to JSON
            if (field->algorithm == SIM_ALGO_EXTERNAL_VALUE) {
                continue;
            }

            double value = (field->type == SIM_TYPE_FLOAT) ? field->current_value.f : (double)field->current_value.i;

            // Check if field already exists and replace it, otherwise add new
            cJSON* existing_field = cJSON_GetObjectItemCaseSensitive(pr_telemetry, field->field_name);
            if (existing_field != NULL) {
                cJSON_SetNumberValue(existing_field, value);
            } else {
                cJSON_AddNumberToObject(pr_telemetry, field->field_name, value);
            }
        }
    }
    
    // Write ROVER file
    snprintf(filepath, sizeof(filepath), "data/ROVER.json");
    
    char* rover_json_str = cJSON_Print(rover_root);
    FILE* rover_fp = fopen(filepath, "w");
    if (rover_fp) {
        fputs(rover_json_str, rover_fp);
        fclose(rover_fp);
    }
    
    free(rover_json_str);
    cJSON_Delete(rover_root);
}

/**
 * Updates a field in a JSON file based on a route-style request (for example, "eva.error.fan_error=true") from a HTML form submission
 * The request content is parsed and matched to the appropriate JSON file and field.
 * 
 * @example request_content: "eva.error.fan_error=true" -> EVA.json, section "error", field "fan_error", value true
 * @param request_content String containing the route-based update request
 * @param backend Backend data structure
 * @return true if update was successful, false otherwise
 */
bool html_form_json_update(char* request_content, struct backend_data_t* backend) {
    // Parse URL-encoded data: "route=value"
    char* route = NULL;
    char* value = NULL;

    // Create a copy to work with since strtok modifies the string
    char request_copy[1024];
    strncpy(request_copy, request_content, sizeof(request_copy) - 1);
    request_copy[sizeof(request_copy) - 1] = '\0';

    // Parse URL-encoded parameters
    char* param = strtok(request_copy, "&");
    while (param != NULL) {
        char* equals_pos = strchr(param, '=');
        if (equals_pos != NULL) {
            *equals_pos = '\0';  // Split parameter name and value
            char* param_name = param;
            char* param_value = equals_pos + 1;

            // route parameter e.g. "eva.error.fan_error"
            route = param_name;
            value = param_value;
            break;  // Take the first parameter as the route
        }
        param = strtok(NULL, "&");
    }

    if (route == NULL || value == NULL) {
        printf("Error: Invalid format, missing route or value in request: %s\n", request_content);
        return false;
    }

    //printf("Processing route update: %s = %s\n", route, value);


    // Parse the route (split by dots)
    char route_copy[256];
    strncpy(route_copy, route, sizeof(route_copy) - 1);
    route_copy[sizeof(route_copy) - 1] = '\0';
    
    // Split route into parts
    char* route_parts[10];  // Support up to 10 levels deep
    int part_count = 0;
    
    char* token = strtok(route_copy, ".");
    while (token != NULL && part_count < 10) {
        route_parts[part_count++] = token;
        token = strtok(NULL, ".");
    }
    
    if (part_count < 2) {
        printf("Error: Route must have at least 2 parts (file.section): %s\n", route);
        return false;
    }
    
    // Determine file type and construct JSON path
    const char* filename = NULL;
    if (strcmp(route_parts[0], "eva") == 0) {
        filename = "EVA";
    } else if (strcmp(route_parts[0], "rover") == 0) {
        filename = "ROVER";
    } else if (strcmp(route_parts[0], "ltv") == 0) {
        filename = "LTV";
    } else {
        printf("Error: Unsupported file type '%s'. Use 'eva', 'rover', or 'ltv'\n", route_parts[0]);
        return false;
    }
    
    // Build the JSON path from remaining route parts

    if (part_count == 3) {
        // Simple case: file.section.field
        const char* section = route_parts[1];
        const char* field = route_parts[2];
        
        update_json_file(filename, section, field, value);

        // Handle simulation control for specific fields
        if (strcmp(filename, "ROVER") == 0 && strcmp(section, "pr_telemetry") == 0 && strcmp(field, "sim_running") == 0) {
            if (backend->sim_engine) {
                if (strcmp(value, "true") == 0) {
                    sim_engine_start_component(backend->sim_engine, "rover");
                    printf("Started rover simulation\n");
                } else {
                    sim_engine_reset_component(backend->sim_engine, "rover");
                    printf("Reset rover simulation\n");
                }
            }
        }

        if (strcmp(filename, "EVA") == 0 && strcmp(section, "status") == 0 && strcmp(field, "started") == 0) {
            if (backend->sim_engine) {
                if (strcmp(value, "true") == 0) {
                    sim_engine_start_component(backend->sim_engine, "eva1");
                    sim_engine_start_component(backend->sim_engine, "eva2");
                    printf("Started EVA simulation\n");
                } else {
                    sim_engine_reset_component(backend->sim_engine, "eva1");
                    sim_engine_reset_component(backend->sim_engine, "eva2");
                    reset_eva_station_timing();
                    printf("Reset EVA simulation\n");
                }
            }
        }

        return true;
    } else if (part_count > 3) {
        // Nested case: file.section.subsection.field or deeper
        const char* section = route_parts[1];
        
        // Rebuild the nested field path from remaining parts
        char nested_field[256] = "";
        for (int i = 2; i < part_count; i++) {
            if (i > 2) {
                strcat(nested_field, ".");
            }
            strcat(nested_field, route_parts[i]);
        }
        
        // For now, handle nested updates by directly updating the JSON
        // This requires extending update_json_file to handle nested paths
        update_json_file(filename, section, nested_field, value);

        // Handle simulation control for specific nested fields
        if (strcmp(filename, "ROVER") == 0 && strcmp(section, "pr_telemetry") == 0 && strcmp(nested_field, "sim_running") == 0) {
            if (backend->sim_engine) {
                if (strcmp(value, "true") == 0) {
                    sim_engine_start_component(backend->sim_engine, "rover");
                    printf("Started rover simulation\n");
                } else {
                    sim_engine_reset_component(backend->sim_engine, "rover");
                    printf("Reset rover simulation\n");
                }
            }
        }

        return true;
    } else {
        printf("Error: Invalid route format: %s\n", route);
        return false;
    }
}

/**
 * Gets a field value from a JSON file using a dot-separated path
 *
 * @param filename Name of the JSON file (e.g., "ROVER", "EVA")
 * @param field_path Dot-separated path to the field (e.g., "pr_telemetry.brakes" or "telemetry.eva1.batt")
 * @param default_value Default value to return if field is not found or invalid
 * @return Field value as double, or default_value if not found
 */
double get_field_from_json(const char* filename, const char* field_path, double default_value) {
    cJSON* json = get_json_file(filename);
    if (json == NULL) {
        return default_value;
    }
    
    // Parse the field path
    char path_copy[256];
    strncpy(path_copy, field_path, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';
    
    cJSON* current_object = json;
    char* field_parts[10];
    int field_part_count = 0;
    
    // Split field path by dots
    char* token = strtok(path_copy, ".");
    while (token != NULL && field_part_count < 10) {
        field_parts[field_part_count++] = token;
        token = strtok(NULL, ".");
    }
    
    // Navigate through the JSON path
    for (int i = 0; i < field_part_count; i++) {
        cJSON* next_object = cJSON_GetObjectItemCaseSensitive(current_object, field_parts[i]);
        if (next_object == NULL) {
            cJSON_Delete(json);
            return default_value;
        }
        current_object = next_object;
    }
    
    // Extract the value based on type
    double result = default_value;
    if (cJSON_IsNumber(current_object)) {
        result = cJSON_GetNumberValue(current_object);
    } else if (cJSON_IsBool(current_object)) {
        result = cJSON_IsTrue(current_object) ? 1.0 : 0.0;
    } else if (cJSON_IsString(current_object)) {
        // Try to parse string as number
        char* endptr;
        double parsed_value = strtod(cJSON_GetStringValue(current_object), &endptr);
        if (*endptr == '\0') {
            result = parsed_value;
        }
    }
    
    cJSON_Delete(json);
    return result;
}

/**
 * Updates EVA station timing based on started states
 * Increments time for stations that are started and marks completed when stopped
 */
void update_eva_station_timing(void) {
    // Load current EVA JSON data
    cJSON* eva_json = get_json_file("EVA");
    if (eva_json == NULL) {
        return;
    }

    // Get the status section
    cJSON* status = cJSON_GetObjectItemCaseSensitive(eva_json, "status");
    if (status == NULL) {
        cJSON_Delete(eva_json);
        return;
    }

    bool json_modified = false;
    const char* stations[] = {"uia", "dcu", "spec"};
    int num_stations = sizeof(stations) / sizeof(stations[0]);

    // Process each station
    for (int i = 0; i < num_stations; i++) {
        const char* station_name = stations[i];

        // Get station object
        cJSON* station = cJSON_GetObjectItemCaseSensitive(status, station_name);
        if (station == NULL) {
            continue;
        }

        // Get started and time fields
        cJSON* started_field = cJSON_GetObjectItemCaseSensitive(station, "started");
        cJSON* time_field = cJSON_GetObjectItemCaseSensitive(station, "time");
        cJSON* completed_field = cJSON_GetObjectItemCaseSensitive(station, "completed");

        if (started_field == NULL || time_field == NULL) {
            continue;
        }

        bool is_started = cJSON_IsTrue(started_field);
        double current_time = cJSON_GetNumberValue(time_field);

        // If station is started, increment time
        if (is_started) {
            cJSON_SetNumberValue(time_field, current_time + 1.0);
            json_modified = true;
        }

        // Check if station was just stopped (completed should be set when started changes from true to false)
        // This is handled by the frontend toggle logic, but we can ensure completed status is consistent
        if (!is_started && completed_field != NULL && cJSON_IsFalse(completed_field) && current_time > 0) {
            cJSON_SetBoolValue(completed_field, true);
            json_modified = true;
        }
    }

    // Save changes if JSON was modified
    if (json_modified) {
        char filepath[100];
        snprintf(filepath, sizeof(filepath), "data/EVA.json");

        char* json_str = cJSON_Print(eva_json);
        FILE* fp = fopen(filepath, "w");
        if (fp) {
            fputs(json_str, fp);
            fclose(fp);
        }
        free(json_str);
    }

    cJSON_Delete(eva_json);
}

/**
 * Resets EVA station timing by setting all station times to 0 and completed status to false
 */
void reset_eva_station_timing(void) {
    // Load current EVA JSON data
    cJSON* eva_json = get_json_file("EVA");
    if (eva_json == NULL) {
        return;
    }

    // Get the status section
    cJSON* status = cJSON_GetObjectItemCaseSensitive(eva_json, "status");
    if (status == NULL) {
        cJSON_Delete(eva_json);
        return;
    }

    bool json_modified = false;
    const char* stations[] = {"uia", "dcu", "spec"};
    int num_stations = sizeof(stations) / sizeof(stations[0]);

    // Reset each station
    for (int i = 0; i < num_stations; i++) {
        const char* station_name = stations[i];

        // Get station object
        cJSON* station = cJSON_GetObjectItemCaseSensitive(status, station_name);
        if (station == NULL) {
            continue;
        }

        // Get time and completed fields
        cJSON* time_field = cJSON_GetObjectItemCaseSensitive(station, "time");
        cJSON* completed_field = cJSON_GetObjectItemCaseSensitive(station, "completed");

        // Reset time to 0
        if (time_field != NULL) {
            cJSON_SetNumberValue(time_field, 0.0);
            json_modified = true;
        }

        // Reset completed status to false
        if (completed_field != NULL) {
            cJSON_SetBoolValue(completed_field, false);
            json_modified = true;
        }
    }

    // Save changes if JSON was modified
    if (json_modified) {
        char filepath[100];
        snprintf(filepath, sizeof(filepath), "data/EVA.json");

        char* json_str = cJSON_Print(eva_json);
        FILE* fp = fopen(filepath, "w");
        if (fp) {
            fputs(json_str, fp);
            fclose(fp);
        }
        free(json_str);
    }

    cJSON_Delete(eva_json);
}

///////////////////////////////////////////////////////////////////////////////////
//                              Helper Functions
///////////////////////////////////////////////////////////////////////////////////

/**
 * Reverses the byte order of a 4-byte value for endianness conversion
 * 
 * @param bytes Pointer to 4-byte array to reverse
 */
void reverse_bytes(unsigned char *bytes) {
    // expects 4 bytes to be flipped
    char temp;
    for (int i = 0; i < 2; i++) {
        temp = bytes[i];
        bytes[i] = bytes[3 - i];
        bytes[3 - i] = temp;
    }
}

/**
 * Determines if the system uses big-endian byte ordering
 *
 * @return true if system is big-endian, false if little-endian
 */
bool big_endian() {
    unsigned int i = 1;
    unsigned char temp[4];

    memcpy(temp, &i, 4);

    if (temp[0] == 1) {
        // System is little-endian
        return false;
    } else {
        // System is big-endian
        return true;
    }
}

/**
 * Extracts boolean value from UDP data (bytes 4-7 as float, non-zero = true)
 *
 * @param data UDP data buffer
 * @return Boolean value
 */
bool extract_bool_value(unsigned char* data) {
    float bool_float;
    memcpy(&bool_float, data, 4);
    return bool_float != 0.0f;
}

/**
 * Extracts float value from UDP data (bytes 4-7)
 *
 * @param data UDP data buffer
 * @return Float value
 */
float extract_float_value(unsigned char* data) {
    float value;
    memcpy(&value, data, 4);
    return value;
}