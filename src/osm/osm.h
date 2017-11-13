#ifndef OSM_OSM
#define OSM_OSM

#include <stdint.h>
#include "hashmap.h"
#include "vec.h"
#include "parser.h"

DEFINE_HASHMAP(node_map, struct node)
DEFINE_HASHMAP(way_map, struct way)

struct tag {
	char *key;
	char *val;
};
DEFINE_HASHMAP(tag_map, struct tag)

typedef int64_t id;
typedef vec_t(id) vec_id_t;

struct node {
	id id;
	point pos;
};

enum way_type {
	WAY_UNKNOWN = 0,
	WAY_ROAD,
	WAY_BUILDING,
	WAY_LANDUSE
};

enum road_type {
	ROAD_UNKNOWN = 0,
	ROAD_MOTORWAY,
	ROAD_PRIMARY,
	ROAD_SECONDARY,
	ROAD_MINOR,
	ROAD_RESIDENTIAL,
	ROAD_PEDESTRIAN
};

enum building_type {
	BUILDING_UNKNOWN = 0,
	BUILDING_ACCOMODATION,
	BUILDING_COMMERCIAL,
	BUILDING_CIVIC,
	BUILDING_OTHER
};

enum land_use_type {
	LANDUSE_UNKNOWN = 0,
	LANDUSE_RESIDENTIAL,
	LANDUSE_COMMERCIAL,
	LANDUSE_AGRICULTURE,
	LANDUSE_INDUSTRIAL,
	LANDUSE_GREEN,
	LANDUSE_WATER
};

const char *road_type_to_string(enum road_type rt);

struct road {
	enum road_type type;
	vec_point_t segments;
	char *name;
};

struct building {
	enum building_type type;
	vec_point_t points;
};

struct land_use {
	enum land_use_type type;
	vec_point_t points;
};

struct way {
	id id;
	vec_id_t nodes;

	enum way_type way_type;
	union {
		struct road road;
		struct building building;
		struct land_use land_use;
	} que;
};

#endif
