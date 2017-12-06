#ifndef OSM_PARSER
#define OSM_PARSER

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "vec.h"

extern FILE *err_stream;

struct world;

typedef uint32_t coord;

typedef struct {
	coord x, y;
} point;

typedef vec_t(point) vec_point_t;

struct osm_source {
	int is_file;
    union {
        char *file_path;

        struct {
			char *buf;
			size_t n;
		};
	} u;
};
int parse_osm(struct osm_source *src, struct world *out);

#endif

