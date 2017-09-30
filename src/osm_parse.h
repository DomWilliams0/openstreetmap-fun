#ifndef OSM_OSM_PARSE
#define OSM_OSM_PARSE

#include <stdint.h>
#include "hashmap.h"

typedef int64_t id;
typedef uint32_t coord;

DEFINE_HASHMAP(node_map, struct node);

struct node {
	id id;
	coord pos[2];
};

struct way {
	id id;
	struct node *nodes;
};

struct context {
	node_map nodes;
	struct way *ways;
	coord bounds[2];
};

int parse_xml(char *file_path, struct context *out);

void free_context(struct context *ctx);


#endif

