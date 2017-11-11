#include <stdio.h>

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

void debug_print(struct world *world) {
	printf("World bounds: %d, %d\n", world->bounds[0], world->bounds[1]);
	printf("%d roads:\n", world->roads.length);

	int i = 0;
	struct road r = {0};
	vec_foreach(&world->roads, r, i) {
		printf("\t%s with %d segments", road_type_to_string(r.type), r.segments.length);
		if (r.name != NULL)
			printf(" - %s", r.name);
		printf("\n");
	}
}
