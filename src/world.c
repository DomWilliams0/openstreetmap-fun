#include "world.h"
#include "error.h"

int init_world(struct world *world) {
	vec_init(&world->roads);
	return CRACKING;
}

void free_world(struct world *world) {
	if (world->roads.data != NULL)
		vec_deinit(&world->roads);
}

