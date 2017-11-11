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

// TODO move to way.h
struct way {
	id id;
	vec_id_t nodes;
};

#endif
