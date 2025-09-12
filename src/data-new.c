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
void simulate_backend(struct backend_data_t *backend) {
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
void handle_udp_get_request(unsigned int command, struct backend_data_t* backend) {
    // Handle different GET requests based on command
    switch (command) {
        case 1: // Rover data
            printf("Getting Rover data for all teams.\n");
            // Implement udp_get_rover for all teams
            break;

        case 2: // EVA telemetry
            printf("Getting EVA telemetry for all teams.\n");
            // Implement udp_get_eva_telemetry for all teams
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
 * @param filename Name of the JSON file to update (e.g., "EVA_TELEMETRY.json")
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
