#include <stdio.h>

#include "error.h"
#include "osm_parse.h"

int main() {

	struct context ctx;
	int ret = parse_xml("../xmls/place.xml", &ctx);
	printf("result: %s\n", error_get_message(ret));

	free_context(&ctx);
}
