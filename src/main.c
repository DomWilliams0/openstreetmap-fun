#include <stdio.h>

#include "error.h"
#include "osm/parser.h"

int main() {

	struct world world;
	int ret = parse_osm("../xmls/place.xml", &world);
	printf("result: %s\n", error_get_message(ret));

	if (ret == CRACKING)
		free_world(&world);
}
