#ifndef OSM_ERROR_H
#define OSM_ERROR_H

#define CRACKING (0)

#define ERR_FILE_NOT_FOUND (0x1000)
#define ERR_IO             (0x1001)
#define ERR_OSM            (0x1002)

const char *error_get_message(int err);

#endif
