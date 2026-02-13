#ifndef SERVER_H
#define SERVER_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "lib/cjson/cJSON.h"
#include "network.h"
#include "data.h"

// Unreal Engine Communication Commands
#define TSS_TO_UNREAL_BRAKES_COMMAND 2000
#define TSS_TO_UNREAL_LIGHTS_COMMAND 2001
#define TSS_TO_UNREAL_STEERING_COMMAND 2002
#define TSS_TO_UNREAL_THROTTLE_COMMAND 2003
#define TSS_TO_UNREAL_PING_COMMAND 2005

// Server Configuration
#define UNREAL_UPDATE_INTERVAL_SEC 1.0

#define LIDAR_NUM_POINTS 17 // number of LiDAR points expected from the DUST sim to allocate in memory

extern struct profile_context_t profile_context;

#endif // SERVER_H