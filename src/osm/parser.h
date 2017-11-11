#ifndef OSM_PARSER
#define OSM_PARSER

#include <stdint.h>
#include "hashmap.h"
#include "vec.h"

typedef uint32_t coord;

typedef struct {
	coord x, y;
} point;

typedef vec_t(point) vec_point_t;

enum road_type {
	ROAD_MOTORWAY,
	ROAD_PRIMARY,
	ROAD_SECONDARY,
	ROAD_MINOR,
	ROAD_PEDESTRIAN
};

struct road {
	enum road_type type;
	vec_point_t segments;
	char *name;
};
typedef vec_t(struct road) vec_road_t;

struct world {
	coord bounds[2];
	vec_road_t roads;
};

int parse_osm(char *file_path, struct world *out);

void free_world(struct world *world);


#endif

