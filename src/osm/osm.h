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

struct way {
	id id;
	vec_id_t nodes;

	enum way_type way_type;
	union {
		struct road road;
	} que;
};

#endif
