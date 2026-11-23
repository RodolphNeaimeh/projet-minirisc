#include <stdio.h>
#include <stdlib.h>
#include "platform.h"
#include <inttypes.h>


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


#define RAM_BASE    UINT32_C(0x80000000)
#define CHAROUT_CHR UINT32_C(0x10000000)
#define CHAROUT_DEC UINT32_C(0x10000004)
#define CHAROUT_HEX UINT32_C(0x10000008)

/* Les valeurs de l'énumération ne sont pas les tailles en octets. */
/* Convertit le type d'accès en nombre d'octets : 1, 2 ou 4.  Renvoie 0 si le type d'accès est inconnu.*/
static unsigned int access_size(platform_access_type_t access_type)
{
    switch (access_type) {
    case PLATFORM_ACCESS_BYTE:
        return 1;
    case PLATFORM_ACCESS_HALF:
        return 2;
    case PLATFORM_ACCESS_WORD:
        return 4;
    default:
        return 0;
    }
}

int platform_load_program(platform_t *plt, const char *file_name)
{
    FILE *file = fopen(file_name, "rb");
    if (file == NULL) {
        return -1;
    }

    /* Mesure le fichier avant de copier ses octets dans la RAM. */
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return -1;
    }

    long length = ftell(file);
    if (length < 0 || length > (long)RAM_SIZE_BYTES ||
        fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return -1;
    }

    size_t bytes_read = fread(plt->memory, 1, (size_t)length, file);
    int close_result = fclose(file);

    if (bytes_read != (size_t)length || close_result != 0) {
        return -1;
    }

    return 0;
}

int platform_read(platform_t *plt, platform_access_type_t access_type,
                  uint32_t addr, uint32_t *data)
{
    unsigned int size = access_size(access_type);
    if (size == 0 || addr % size != 0) {
        return -1;
    }

    if (size == 4 &&
        (addr == CHAROUT_CHR || addr == CHAROUT_DEC || addr == CHAROUT_HEX)) {
        *data = 0;
        return 0;
    }

    if (addr < RAM_BASE || addr - RAM_BASE > RAM_SIZE_BYTES - size) {
        return -1;
    }

    const unsigned char *ram = (const unsigned char *)plt->memory;
    uint32_t offset = addr - RAM_BASE;
    uint32_t value = 0;

    /* l'octet de poids faible est à l'adresse la plus basse. */
    for (unsigned int i = 0; i < size; ++i) {
        value |= (uint32_t)ram[offset + i] << (8u * i);
    }

    *data = value;
    return 0;
}

int platform_write(platform_t *plt, platform_access_type_t access_type,
                   uint32_t addr, uint32_t data)
{
    unsigned int size = access_size(access_type);
    if (size == 0 || addr % size != 0) {
        return -1;
    }

    if (size == 4) {
        switch (addr) {
        case CHAROUT_CHR:
            return putchar((unsigned char)data) == EOF ? -1 : 0;
        case CHAROUT_DEC:
            return printf("%" PRId32, (int32_t)data) < 0 ? -1 : 0;
        case CHAROUT_HEX:
            return printf("%" PRIx32, data) < 0 ? -1 : 0;
        }
    }

    if (addr < RAM_BASE || addr - RAM_BASE > RAM_SIZE_BYTES - size) {
        return -1;
    }

    unsigned char *ram = (unsigned char *)plt->memory;
    uint32_t offset = addr - RAM_BASE;

    /* Écrit seulement les octets concernés par le store. */
    for (unsigned int i = 0; i < size; ++i) {
        ram[offset + i] = (unsigned char)(data >> (8u * i));
    }

    return 0;
}
