#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>
#include <error.h>
#include <math.h>
#include <limits.h>

#include "osm_parse.h"

#ifdef DEBUG
#define LOG printf
#else
#define LOG //
#endif

DEFINE_HASHMAP(node_map, struct node);
DEFINE_HASHMAP(way_map, struct way);

typedef int64_t id;
typedef vec_t(id) vec_id_t;

struct node {
	id id;
	point pos;
};

struct way {
	id id;
	vec_id_t nodes;
};

#define NODE_CMP(left, right) left->id != right->id
#define NODE_HASH(entry) entry->id
DECLARE_HASHMAP(node_map, NODE_CMP, NODE_HASH, free, realloc);
DECLARE_HASHMAP(way_map, NODE_CMP, NODE_HASH, free, realloc);

enum tag_type {
	TAG_NODE,
	TAG_TAG,
	TAG_WAY,
	TAG_NODE_REF,
	TAG_RELATION,
	TAG_UNKNOWN
};

struct xml_tag {
	enum tag_type type;
	bool opening;
};

const char *tag_lookup[] = {
	"node",
	"tag",
	"way",
	"nd",
	"relation",
};

struct xml_tag parse_tag(char *tag_in) {
	struct xml_tag out = {
		.type = TAG_UNKNOWN
	};

	if (tag_in[0] == '/') {
		out.opening = false;
		tag_in++;
	} else {
		out.opening = true;
	}

	for (int i = 0; i < TAG_UNKNOWN; i++) {
		if (strcmp(tag_lookup[i], tag_in) == 0) {
			out.type = (enum tag_type) i;
			break;
		}
	}

	return out;
}

struct parse_ctx {
	FILE *f;
	size_t n;
	char *full_line;
	char *line_end;
	char *tag_start;
	char *attr_start;

	enum tag_type current_tag;
	union {
		struct node node;
		struct way way;
	} que;

	double lat_range[2];
	double lon_range[2];

	node_map nodes;
	way_map ways;

	struct world out;
};

int open_file(struct parse_ctx *ctx, char *file_path) {
	ctx->f = fopen(file_path, "r");
	return ctx->f != NULL ? CRACKING : ERR_FILE_NOT_FOUND;
}

int find_char(char *s, char c) {
	int i = 0;
	while (*s) {
		if (*s == c)
			return i;
		i++;
		s++;
	}
	return -1;
}

bool line_ends_with_close_tag(struct parse_ctx *ctx) {
	int len = (int) (ctx->line_end - ctx->full_line);
	char *line = ctx->full_line;

	for (unsigned long i = len; i >= 1; i--)
		if (line[i] == '>' && line[i - 1] == '/')
			return true;

	return false;
}

int find_tag(char *line, char **tag_start) {
	int tag = find_char(line, '<');
	if (tag == -1)
		return ERR_OSM;

	int end_a = find_char(line + tag, ' ');
	int end_b = find_char(line + tag, '>');
	if (end_a == -1 && end_b == -1)
		return ERR_OSM;

	int end;
	if (end_a == -1)
		end = end_b;
	else if (end_b == -1)
		end = end_a;
	else
		end = end_a < end_b ? end_a : end_b;


	line[end + tag] = '\0';
	*tag_start = line + tag + 1;

	return CRACKING;
}

int read_line(struct parse_ctx *ctx) {
	while (true) {
		int read = getline(&ctx->full_line, &ctx->n, ctx->f);

		// possibly at the end too
		if (read == -1) {
			free(ctx->full_line);
			ctx->full_line = NULL;
			ctx->line_end = NULL;
			ctx->tag_start = NULL;
			ctx->attr_start = NULL;
			return ERR_IO;
		}

		ctx->line_end = ctx->full_line + strlen(ctx->full_line);

		if (find_tag(ctx->full_line, &ctx->tag_start) == CRACKING) {
			ctx->attr_start = ctx->tag_start + strlen(ctx->tag_start) + 1;
			return CRACKING;
		}
	}

	return ERR_IO; // never gonna get here
}

typedef void attr_visitor(char *key, char *val, void *data);
#define ATTR_VISITOR(name) void name(char *key, char *val, void *data)

void visit_attributes(char *line, attr_visitor *visitor, void *data) {
	while (true) {
		// assume attribute starts at the front
		char *key_start = line;

		// find equals
		char *eq = index(key_start, '=');
		if (eq == NULL)
			return;

		// split on equals
		*eq = '\0';
		char *val_start = eq + 2; // ="
		char *val_end = index(val_start, '"');
		if (val_end == NULL)
			return;

		*val_end = '\0';

		// wahey!
		visitor(key_start, val_start, data);
		// TODO allow early termination

		// move on
		line = val_end + 1;
		while (*line && isblank(*line))
			line++;
	}
}

int add_node_to_context(struct parse_ctx *ctx) {
	struct node *node = &ctx->que.node;

	// unset current
	ctx->current_tag = TAG_UNKNOWN;

	LOG("adding node '%lu'\n", node->id);
	HashMapPutResult res = node_mapPut(&ctx->nodes, &node, HMDR_REPLACE);
	if (res == HMPR_FAILED)
		return ERR_MEM;

	return CRACKING;
}

void convert_to_pixels(double lat, double lon, point *out) {
	const int ZOOM = 23;
	const double N = 1 << ZOOM;
	const double PI = 3.14159265359;
	const double RAD = PI / 180.0;

	double lat_rad = lat * RAD;
	out->x = (coord)((lon + 180.0) / 360.0 * N);
	out->y = (coord)((1.0 - log(tan(lat_rad) + (1.0 / cos(lat_rad))) / PI) / 2.0 * N);
}

void make_coords_relative(struct parse_ctx *ctx) {
	point min, max;

	convert_to_pixels(ctx->lat_range[1], ctx->lon_range[0], &min);
	convert_to_pixels(ctx->lat_range[0], ctx->lon_range[1], &max);

	struct node *node = NULL;
	HASHMAP_FOR_EACH(node_map, node, ctx->nodes) {
		node->pos.x -= min.x;
		node->pos.y -= min.y;
	} HASHMAP_FOR_EACH_END
}

struct node_lat {
	id id;
	double lon, lat;
};

bool id_to_long(char *s, long *out) {
	char *str_end;
	long long_id = strtol(s, &str_end, 10);
	if (*str_end != '\0')
		return false;
	*out = long_id;
	return true;
}

ATTR_VISITOR(node_visitor) {

	if (strcmp(key, "id") == 0) {
		if (!id_to_long(val, &((struct node_lat *)data)->id)) {
			printf("bad node id '%s'\n", val);
			return;
		}
	}

	else if (strcmp(key, "lat") == 0) {
		((struct node_lat *)data)->lat = strtold(val, NULL);;
	}

	else if (strcmp(key, "lon") == 0) {
		((struct node_lat *)data)->lon = strtold(val, NULL);;
	}
}

int parse_node_tag(struct parse_ctx *ctx, bool opening) {
	// closing
	if (!opening) {
		return add_node_to_context(ctx);
	}

	struct node *node = &ctx->que.node;

	struct node_lat node_lat = {0};
	visit_attributes(ctx->attr_start, node_visitor, &node_lat);
	node->id = node_lat.id;

	const coord INVALID_COORD = UINT_MAX;
	node->pos.x = INVALID_COORD;
	node->pos.y = INVALID_COORD;

	convert_to_pixels(node_lat.lat, node_lat.lon, &node->pos);

	// uh oh
	if (node->id == 0 || node->pos.x == INVALID_COORD || node->pos.y == INVALID_COORD) {
		printf("bad node missing id/lat/lon\n");
		return ERR_OSM;
	}

	// TODO could move this operation to the end to make the most of cache?
	ctx->lat_range[0] = fmin(ctx->lat_range[0], node_lat.lat);
	ctx->lat_range[1] = fmax(ctx->lat_range[1], node_lat.lat);
	ctx->lon_range[0] = fmin(ctx->lon_range[0], node_lat.lon);
	ctx->lon_range[1] = fmax(ctx->lon_range[1], node_lat.lon);

	// single line
	if (line_ends_with_close_tag(ctx)) {
		return add_node_to_context(ctx);

	} else {
		// has more lines, dont store yet
		ctx->current_tag = TAG_NODE;
	}

	return CRACKING;
}

ATTR_VISITOR(tag_visitor) {
	switch(key[0]) {
		case 'k':
			((char **)data)[0] = val;
			break;
		case 'v':
			((char **)data)[1] = val;
			break;
	}
}

ATTR_VISITOR(node_ref_visitor) {
	if (strcmp(key, "ref") == 0) {
		if (!id_to_long(val, (id *)data)) {
			printf("bad node ref id '%s'\n", val);
			return;
		}

	}
}
int parse_node_ref_tag(struct parse_ctx *ctx) {
	if (ctx->current_tag != TAG_WAY) {
		printf("nd tag found inside non-way tag '%s'\n", tag_lookup[ctx->current_tag]);
		return ERR_OSM;
	}

	id id = 0;
	visit_attributes(ctx->attr_start, node_ref_visitor, &id);

	if (id == 0)
		return ERR_OSM;


	struct way *way = &ctx->que.way;
	return vec_push(&way->nodes, id) == 0 ? CRACKING : ERR_MEM;
}

ATTR_VISITOR(way_visitor) {

	if (strcmp(key, "id") == 0) {
		if (!id_to_long(val, &((struct way *)data)->id)) {
			printf("bad way id '%s'\n", val);
			return;
		}
	}
}
int add_way_to_context(struct parse_ctx *ctx) {
	struct way *way = &ctx->que.way;

	// unset current
	ctx->current_tag = TAG_UNKNOWN;

	LOG("adding way '%lu'\n", way->id);

	HashMapPutResult res = way_mapPut(&ctx->ways, &way, HMDR_FAIL);
	if (res == HMPR_FAILED)
		return ERR_MEM;

	return CRACKING;
}


int parse_way_tag(struct parse_ctx *ctx, bool opening) {
	// closing
	if (!opening) {
		return add_way_to_context(ctx);
	}

	struct way *way = &ctx->que.way;

	visit_attributes(ctx->attr_start, way_visitor, way);
	vec_init(&way->nodes);

	// single line
	if (line_ends_with_close_tag(ctx)) {
		return add_way_to_context(ctx);

	} else {
		// has more lines, dont store yet
		ctx->current_tag = TAG_WAY;
	}

	return CRACKING;
}

int parse_tag_tag(struct parse_ctx *ctx) {
	if (ctx->current_tag != TAG_NODE && ctx->current_tag != TAG_WAY) {
		printf("tag tag found inside non-node or way tag '%s'\n", tag_lookup[ctx->current_tag]);
		return ERR_OSM;
	}

	char *key_val[2] = {0};
	visit_attributes(ctx->attr_start, tag_visitor, &key_val);
	if (key_val[0] == NULL || key_val[1] == NULL) {
		printf("bad tag\n");
		return ERR_OSM;
	}
	// TODO actually use the tag
	return CRACKING;
}

void free_context(struct parse_ctx *ctx) {
	node_mapDestroy(&ctx->nodes);

	struct way *way = NULL;
	HASHMAP_FOR_EACH(way_map, way, ctx->ways) {
		vec_deinit(&way->nodes);
	} HASHMAP_FOR_EACH_END

	way_mapDestroy(&ctx->ways);
}

void init_world(struct world *world) {
	vec_init(&world->roads);
}

int parse_osm(char *file_path, struct world *out) {
	struct parse_ctx ctx = {0};
	ctx.current_tag = TAG_UNKNOWN;
	init_world(&ctx.out);

	int ret = CRACKING;

	// open file
	if ((ret = open_file(&ctx, file_path)) == CRACKING) {

		while (true) {
			if (read_line(&ctx) != CRACKING) break;

			struct xml_tag tag = parse_tag(ctx.tag_start);

			switch(tag.type) {
				case TAG_NODE:
					if ((ret = parse_node_tag(&ctx, tag.opening)) != CRACKING)
						printf("error processing node: %s\n", error_get_message(ret));
					break;

				case TAG_WAY:
					if ((ret = parse_way_tag(&ctx, tag.opening)) != CRACKING)
						printf("error processing way: %s\n", error_get_message(ret));
					break;

				case TAG_NODE_REF:
					if ((ret = parse_node_ref_tag(&ctx)) != CRACKING)
						printf("error processing node ref: %s\n", error_get_message(ret));
					break;

				case TAG_TAG:
					parse_tag_tag(&ctx);
					break;

				default:
					continue;

			}

		}

		fclose(ctx.f);
		ctx.f = NULL;

		make_coords_relative(&ctx);

		struct way *w = NULL;
		HASHMAP_FOR_EACH(way_map, w, ctx.ways) {
			LOG("way %lu has %d\n", w->id,  w->nodes.length);
		} HASHMAP_FOR_EACH_END
	}

	*out = ctx.out;
	free_context(&ctx);
	return ret;
}

void free_world(struct world *world) {
	if (world->roads.data != NULL)
		vec_deinit(&world->roads);
}
