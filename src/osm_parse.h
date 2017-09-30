#ifndef OSM_OSM_PARSE
#define OSM_OSM_PARSE

#include <stdint.h>

typedef int64_t id;
typedef uint32_t coord;

struct node {
	id id;
	// coord pos[2];
	double lat, lon;
	// TODO convert to coordinates
};

struct way {
	id id;
	struct node *nodes;
};

struct context {
	// TODO map ids->nodes
	struct way *ways;
	coord bounds[2];
};

int parse_xml(char *file_path, struct context *out);


#endif

