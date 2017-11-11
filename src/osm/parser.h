#ifndef OSM_PARSER
#define OSM_PARSER

#include <stdint.h>
#include "hashmap.h"
#include "vec.h"

struct world;

typedef uint32_t coord;

typedef struct {
	coord x, y;
} point;

typedef vec_t(point) vec_point_t;

int parse_osm(char *file_path, struct world *out);

#endif

