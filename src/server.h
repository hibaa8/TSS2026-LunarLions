#ifndef SERVER_H
#define SERVER_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib/cjson/cJSON.h"
#include "network.h"
#include "data-new.h"

// Unreal Engine Communication Commands
#define TSS_TO_UNREAL_BRAKES_COMMAND 2000
#define TSS_TO_UNREAL_LIGHTS_COMMAND 2001
#define TSS_TO_UNREAL_STEERING_COMMAND 2002
#define TSS_TO_UNREAL_THROTTLE_COMMAND 2003

// Server Configuration
#define UNREAL_UPDATE_INTERVAL_SEC 0.2

extern struct profile_context_t profile_context;

#endif // SERVER_H