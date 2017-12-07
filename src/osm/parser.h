#ifndef OSM_PARSER
#define OSM_PARSER

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "vec.h"

extern FILE *err_stream;

struct world;

typedef uint32_t coord;

typedef struct {
	coord x, y;
} point;

typedef vec_t(point) vec_point_t;

int parse_osm_from_file(const char *path, struct world *out);
int parse_osm_from_buffer(void *buffer, size_t len, struct world *out);

void convert_latlon_to_pixel(double lat, double lon, point *out);
#endif

