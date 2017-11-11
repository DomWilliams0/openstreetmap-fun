#ifndef OSM_WORLD
#define OSM_WORLD

#include "osm/parser.h"

typedef vec_t(struct road) vec_road_t;
struct world {
	coord bounds[2];
	vec_road_t roads;
};

int init_world(struct world *world);

void free_world(struct world *world);


#endif

