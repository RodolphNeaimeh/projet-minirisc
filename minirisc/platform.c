#include <stdio.h>
#include <stdlib.h>
#include "platform.h"

#define RAM_SIZE_BYTES (32u * 1024u * 1024u)

platform_t* platform_new()
{
    /* Ici, on alloue la structure qui contient le pointeur vers la RAM */
    platform_t *platform = malloc(sizeof *platform);
    if (platform == NULL) {
        return NULL;
    }

    /* calloc alloue les 32 Mio du RAM et on les initialise a zero */
    platform->memory = calloc(RAM_SIZE_BYTES / sizeof *platform->memory, sizeof *platform->memory);

    if (platform->memory == NULL) {
        free(platform);
        return NULL;
    }

    return platform;
}

void platform_free(platform_t *platform)
{
    if (platform == NULL) {
        return;
    }

    // Ici on libere les 32 Mio reserves par calloc.
    free(platform->memory);

    // Ici on libere la structure réservée par malloc.
    free(platform);	
}


void platform_free(platform_t *platform)
{
	// TODO
}


int platform_load_program(platform_t *plt, const char *file_name)
{
	// TODO
}


int platform_read(platform_t *plt, platform_access_type_t access_type, uint32_t addr, uint32_t *data)
{
	// TODO
}


int platform_write(platform_t *plt, platform_access_type_t access_type, uint32_t addr, uint32_t data)
{
	// TODO
}


