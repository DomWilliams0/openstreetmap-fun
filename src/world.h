#ifndef OSM_WORLD
#define OSM_WORLD

#include "osm/parser.h"

typedef vec_t(struct road) vec_road_t;
typedef vec_t(struct land_use) vec_land_use_t;
struct world {
	vec_road_t roads;
	vec_land_use_t land_uses;
};

int init_world(struct world *world);

void free_world(struct world *world);

void debug_print(struct world *world);

bool dump_to_file(struct world *world, char *path);


#endif

