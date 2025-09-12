/*
1. Function to handle UDP get requests for data


2. Function handle UDP post requests for data updates/changes

3. data lifecycle management (init, cleanup, reset)

4. Simulation setup (maybe within the data lifecycle management)

5. Functions to update JSON files when values change

6. Functions to update JSON values from the simulation engine. I think the JSON fields should match so this should able to be generalized in my opinion.

7. Helper functions for reversing the bytes and determining endianess.

Other todo:
- Convert all data to make it team specific
*/

#include "data-new.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

///////////////////////////////////////////////////////////////////////////////////
//                        Backend Lifecycle Management
///////////////////////////////////////////////////////////////////////////////////

/**
 * Initializes the backend data structure and simulation engines for each team
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

    // Initialize simulation engine for each team
    for (int i = 0; i < NUM_TEAMS; i++) {
        backend->sim_engine[i] = sim_engine_create();
        if (backend->sim_engine[i]) {
            if (!sim_engine_load_predefined_configs(backend->sim_engine[i])) {
                printf("Warning: Failed to load simulation configurations for team %d\n", i);
            }

            if (!sim_engine_initialize(backend->sim_engine[i])) {
                printf("Warning: Failed to initialize simulation engine for team %d\n", i);
            }
        } else {
            printf("Warning: Failed to create simulation engine for team %d\n", i);
        }
    }

    return backend;
}

/**
 * Calls the simulation engine to update all telemetry data based on elapsed time
 *
 * @param backend Backend data structure containing all telemetry and simulation engines
 */
void start_simulation(struct backend_data_t *backend) {
    // Increment server time
    // @TODO double check the timing logic here is correct
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

        for (int i = 0; i < NUM_TEAMS; i++) {
            if (backend->sim_engine[i]) {
                sim_engine_update(backend->sim_engine[i], delta_time);
            }
        }
    }
}

/**
 * Cleans up backend data structure and frees resources from memory, called in server.c
 * 
 * @param backend Backend data structure to cleanup
 */
void cleanup_backend(struct backend_data_t *backend) {
    if (!backend) return;

    // Cleanup simulation engines for each team
    for (int i = 0; i < NUM_TEAMS; i++) {
        if (backend->sim_engine[i]) {
            sim_engine_destroy(backend->sim_engine[i]);
        }
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
    // Extract team number from data buffer if needed
    unsigned int team_number = 0;
    float fl;
    memcpy(&fl, data, 4);
    team_number = (unsigned int)fl;
    printf("Team number: %u\n", team_number);
    
    // Handle different GET requests - each command returns entire JSON file
    switch (command) {
        case 1: // ROVER TELEMETRY
            printf("Getting ROVER telemetry data.\n");
            send_json_file("ROVER_TELEMETRY", team_number, data);
            break;

        case 2: // EVA TELEMETRY
            printf("Getting EVA telemetry data.\n");
            send_json_file("EVA_TELEMETRY", team_number, data);
            break;
            
        case 3: // EVA STATUS
            printf("Getting EVA status data.\n");
            send_json_file("EVA_STATUS", team_number, data);
            break;
            
        case 4: // COMM
            printf("Getting COMM data.\n");
            send_json_file("COMM", team_number, data);
            break;
            
        case 5: // DCU
            printf("Getting DCU data.\n");
            send_json_file("DCU", team_number, data);
            break;
            
        case 6: // ERROR
            printf("Getting ERROR data.\n");
            send_json_file("ERROR", team_number, data);
            break;
            
        case 7: // UIA
            printf("Getting UIA data.\n");
            send_json_file("UIA", team_number, data);
            break;

        default:
            printf("Invalid GET command: %u\n", command);
            break;
    }
}

/**
 * Handles UDP POST requests for data updates like changing the state of the pressurized rover
 * 
 * @param command Command identifier for the POST request
 * @param data Request buffer containing data to update
 * @param backend Backend data structure containing all telemetry and simulation engines
 */
void handle_udp_post_request(unsigned int command, unsigned char* data, struct backend_data_t* backend) {
    // Handle different POST requests based on command
    switch (command) {
        // @TODO handle all UDP post requests here
        case 1011: // Update Rover brakes
            printf("Updating Rover brakes for all teams.\n");
            // Implement udp_post_update_rover_brakes for all teams
            break;
        
        default:
            printf("Invalid POST command: %u\n", command);
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////////
//                              Data Management
///////////////////////////////////////////////////////////////////////////////////

/**
 * Updates the specified JSON file and specified field name with a new value in the correct section
 * 
 * @param filename Name of the JSON file to update (e.g., "EVA_TELEMETRY")
 * @param team_number Team number to identify the correct team directory
 * @param section Section within the JSON file to update (e.g., "telemetry")
 * @param field Field name within the section to update (e.g., "batt_time_left")
 * @param new_value New value to set for the specified field
 */
void update_json_file(const char* filename, const int team_number, const char* section, const char* field, char* new_value) { // @TODO have to update this function signature
    // Construct the file path based on team number
    char file_path[100];
    snprintf(file_path, sizeof(file_path), "data/teams/%d/%s.json", team_number, filename);

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

    // Navigate to the specified section and update the field
    cJSON *section_json = cJSON_GetObjectItemCaseSensitive(json, section);
    if (section_json == NULL) {
        printf("Error: Section %s not found in JSON.\n", section);
        cJSON_Delete(json);
        return;
    }

    // Update the specified field with the new value
    cJSON_ReplaceItemInObject(section_json, field, new_value);

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
 * Loads and returns a cJSON object from the specified JSON file for a given team
 * 
 * @param filename Name of the JSON file to load (e.g., "EVA_TELEMETRY")
 * @param team_number Team number to identify the correct team directory
 * @return Pointer to the cJSON object representing the file content, or NULL on failure
 */
cJSON* get_json_file(const char* filename, const int team_number) {
    // Construct the file path based on team number
    char file_path[100];
    snprintf(file_path, sizeof(file_path), "data/teams/%d/%s.json", team_number, filename);

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
 * @param filename Name of the JSON file (e.g., "EVA_TELEMETRY")
 * @param team_number Team number (0 for global files like COMM, DCU, etc.)
 * @param data Response buffer to populate with JSON string
 */
void send_json_file(const char* filename, const int team_number, unsigned char* data) {
    cJSON* json = get_json_file(filename, team_number);
    if (json == NULL) {
        printf("Error: Could not load JSON file %s for team %d\n", filename, team_number);
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
 * Synchronizes the simulation engine data to the corresponding JSON file for a specific team
 * 
 * @param backend Backend data structure containing all telemetry and simulation engines
 * @param team_index Index of the team to synchronize (0 to NUM_TEAMS-1)
 */
void sync_simulation_to_json(struct backend_data_t* backend, int team_index) {
    if (team_index < 0 || team_index >= NUM_TEAMS) {
        printf("Error: Invalid team index %d for synchronization.\n", team_index);
        return;
    }

    sim_engine_t* engine = backend->sim_engine[team_index];

    if (engine == NULL) {
        printf("Error: Simulation engine for team %d is NULL.\n", team_index);
        return;
    }

    // Create the complete EVA_TELEMETRY structure
    cJSON* root = cJSON_CreateObject();
    cJSON* telemetry = cJSON_CreateObject();
    
    // Add all simulation fields directly to telemetry section
    for (int i = 0; i < engine->total_field_count; i++) {
        sim_field_t* field = engine->update_order[i];
        if (field != NULL) {
            cJSON_AddNumberToObject(telemetry, field->field_name, field->current_value);
        }
    }
    
    // Add telemetry to root
    cJSON_AddItemToObject(root, "telemetry", telemetry);
    
    // Write to file
    char filepath[100];
    snprintf(filepath, sizeof(filepath), "data/teams/%d/EVA_TELEMETRY.json", team_index);
    
    char* json_str = cJSON_Print(root);
    FILE* fp = fopen(filepath, "w");
    if (fp) {
        fputs(json_str, fp);
        fclose(fp);
    }
    
    free(json_str);
    cJSON_Delete(root);
}

