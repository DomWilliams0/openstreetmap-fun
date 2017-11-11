#include "world.h"
#include "error.h"
#include "osm/osm.h"

int init_world(struct world *world) {
	vec_init(&world->roads);
	return CRACKING;
}

void free_world(struct world *world) {
	int i = 0;
	struct road r = {0};
	vec_foreach(&world->roads, r, i) {
		if (r.name != NULL)
			free(r.name);
	}

	if (world->roads.data != NULL)
		vec_deinit(&world->roads);
}

