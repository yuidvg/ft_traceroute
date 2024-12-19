#pragma once
#include "external.h"

#define NUMBER_OF_ITEMS(array) (sizeof(array) / sizeof(array[0]))

#define DEFAULT_HOPS_MAX 30
#define DEFAULT_PROBES_PER_HOP 3
#define DEFAULT_RTT_MAX_MS 1000
#define DEFAULT_PACKET_SIZE_BYTES 60
#define DEFAULT_SIMALTANIOUS_PROBES 16
#define DEFAULT_EXPIRATION_TIME                                                                                        \
    (struct timeval)                                                                                                   \
    {                                                                                                                  \
        .tv_sec = 5, .tv_usec = 0                                                                                      \
    }

#define ERROR_STRING_SIZE_MAX 16
