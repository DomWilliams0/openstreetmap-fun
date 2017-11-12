#include <stdio.h>

#include "error.h"
#include "osm/parser.h"
#include "world.h"

int main() {

	struct world world;
	int ret = parse_osm("../xmls/place.xml", &world);

	if (ret != CRACKING) {
		printf("error: %s\n", error_get_message(ret));
		return 1;
	}

	debug_print(&world);
	if (!dump_to_file(&world, "world.bin"))
		fprintf(stderr, "failed to dump world to file\n");
	free_world(&world);
}
