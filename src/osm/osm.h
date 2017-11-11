#ifndef OSM_OSM
#define OSM_OSM

#include <stdint.h>
#include "hashmap.h"
#include "vec.h"
#include "parser.h"

DEFINE_HASHMAP(node_map, struct node)
DEFINE_HASHMAP(way_map, struct way)

typedef int64_t id;
typedef vec_t(id) vec_id_t;

struct node {
	id id;
	point pos;
};

enum way_type {
	WAY_UNKNOWN = 0,
	WAY_ROAD
};

enum road_type {
	ROAD_UNKNOWN = 0,
	ROAD_MOTORWAY,
	ROAD_PRIMARY,
	ROAD_SECONDARY,
	ROAD_MINOR,
	ROAD_PEDESTRIAN // TODO footpath is not a road!
};

const char *road_type_to_string(enum road_type rt);

struct road {
	enum road_type type;
	vec_point_t segments;
	char *name;
};


struct way {
	id id;
	vec_id_t nodes;

	enum way_type way_type;
	union {
		struct road road;
	} que;
};

#endif
