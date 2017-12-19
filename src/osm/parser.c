#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>
#include <error.h>
#include <math.h>
#include <limits.h>

#include "osm.h"
#include "world.h"
#include "parser.h"

#define NODE_CMP(left, right) left->id != right->id
#define NODE_HASH(entry) entry->id
DECLARE_HASHMAP(node_map, NODE_CMP, NODE_HASH, free, realloc)
DECLARE_HASHMAP(way_map, NODE_CMP, NODE_HASH, free, realloc)

enum tag_type {
	TAG_NODE,
	TAG_TAG,
	TAG_WAY,
	TAG_NODE_REF,
	TAG_RELATION,
	TAG_UNKNOWN
};

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
	tag_map current_tags;

	double lat_range[2];
	double lon_range[2];

	node_map nodes;
	way_map ways;

	struct world out;
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

// current_tags must have been init'd already
void clear_current(struct parse_ctx *ctx) {
	ctx->current_tag = TAG_UNKNOWN;
	memset(&ctx->que, 0, sizeof(ctx->que));

	struct tag *tag = NULL;
	HASHMAP_FOR_EACH(tag_map, tag, ctx->current_tags) {
		free(tag->key);
		free(tag->val);
	} HASHMAP_FOR_EACH_END
	tag_mapDestroy(&ctx->current_tags);
	tag_mapNew(&ctx->current_tags);
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

	int ret = node_mapPut(&ctx->nodes, &node, HMDR_FAIL) == HMPR_FAILED ? ERR_MEM : CRACKING;

	// unset current
	clear_current(ctx);

	return ret;
}

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
		if (!id_to_long(val, &((struct node *)data)->id)) {
			// fprintf(err_stream, "bad node id '%s'\n", val);
			return;
		}
	}

	else if (strcmp(key, "lat") == 0) {
		((struct node *)data)->pos.lat = strtold(val, NULL);
	}

	else if (strcmp(key, "lon") == 0) {
		((struct node *)data)->pos.lon = strtold(val, NULL);
	}
}

int parse_node_tag(struct parse_ctx *ctx, bool opening) {
	// closing
	if (!opening) {
		return add_node_to_context(ctx);
	}

	struct node *node = &ctx->que.node;

	visit_attributes(ctx->attr_start, node_visitor, node);

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
			((struct tag *)data)->key = strdup(val);
			break;
		case 'v':
			((struct tag *)data)->val = strdup(val);
			break;
	}
}

ATTR_VISITOR(node_ref_visitor) {
	if (strcmp(key, "ref") == 0) {
		if (!id_to_long(val, (id *)data)) {
			// fprintf(err_stream, "bad node ref id '%s'\n", val);
			return;
		}

	}
}
int parse_node_ref_tag(struct parse_ctx *ctx) {
	if (ctx->current_tag != TAG_WAY) {
		// fprintf(err_stream, "nd tag found inside non-way tag '%s'\n", tag_lookup[ctx->current_tag]);
		return ERR_OSM;
	}

	id id = 0;
	visit_attributes(ctx->attr_start, node_ref_visitor, &id);

	if (id == 0)
		return ERR_OSM;


	struct way *way = &ctx->que.way;
	return vec_push(&way->nodes, id) == 0 ? CRACKING : ERR_MEM;
}

static enum road_type parse_road_type(const char *s) {
	if (strcmp(s, "motorway") == 0) return ROAD_MOTORWAY;
	else if (strcmp(s, "motorway_link") == 0) return ROAD_MOTORWAY;
	else if (strcmp(s, "primary_link") == 0) return ROAD_PRIMARY;
	else if (strcmp(s, "primary") == 0) return ROAD_PRIMARY;
	else if (strcmp(s, "trunk") == 0) return ROAD_PRIMARY;
	else if (strcmp(s, "trunk_link") == 0) return ROAD_PRIMARY;

	else if (strcmp(s, "secondary_link") == 0) return ROAD_SECONDARY;
	else if (strcmp(s, "secondary") == 0) return ROAD_SECONDARY;
	else if (strcmp(s, "tertiary") == 0) return ROAD_SECONDARY;
	else if (strcmp(s, "tertiary_link") == 0) return ROAD_SECONDARY;

	else if (strcmp(s, "unclassified") == 0) return ROAD_MINOR;
	else if (strcmp(s, "minor") == 0) return ROAD_MINOR;


	else if (strcmp(s, "residential") == 0) return ROAD_RESIDENTIAL;
	else if (strcmp(s, "living_street") == 0) return ROAD_RESIDENTIAL;

	else if (strcmp(s, "pedestrian") == 0) return ROAD_PEDESTRIAN;
	else if (strcmp(s, "footway") == 0) return ROAD_PEDESTRIAN;
	else if (strcmp(s, "steps") == 0) return ROAD_PEDESTRIAN;
	else if (strcmp(s, "path") == 0) return ROAD_PEDESTRIAN;
	else if (strcmp(s, "cycleway") == 0) return ROAD_PEDESTRIAN;
	else if (strcmp(s, "bridleway") == 0) return ROAD_PEDESTRIAN;

	else return ROAD_UNKNOWN;
}

static enum land_use_type parse_landuse(const char *s) {

	if (strcmp(s, "residential") == 0) return LANDUSE_RESIDENTIAL;

	else if (strcmp(s, "commercial") == 0) return LANDUSE_COMMERCIAL;
	else if (strcmp(s, "retail") == 0) return LANDUSE_COMMERCIAL;

	else if (strcmp(s, "conservation") == 0) return LANDUSE_AGRICULTURE;
	else if (strcmp(s, "plant_nursery") == 0) return LANDUSE_AGRICULTURE;
	else if (strcmp(s, "aquaculture") == 0) return LANDUSE_AGRICULTURE;
	else if (strcmp(s, "farmland") == 0) return LANDUSE_AGRICULTURE;
	else if (strcmp(s, "farmyard") == 0) return LANDUSE_AGRICULTURE;
	else if (strcmp(s, "orchard") == 0) return LANDUSE_AGRICULTURE;
	else if (strcmp(s, "vineyard") == 0) return LANDUSE_AGRICULTURE;
	else if (strcmp(s, "greenhouse_horticulture") == 0) return LANDUSE_AGRICULTURE;
	else if (strcmp(s, "logging") == 0) return LANDUSE_AGRICULTURE;
	else if (strcmp(s, "farm") == 0) return LANDUSE_AGRICULTURE;
	else if (strcmp(s, "allotments") == 0) return LANDUSE_AGRICULTURE;

	else if (strcmp(s, "industrial") == 0) return LANDUSE_INDUSTRIAL;
	else if (strcmp(s, "quarry") == 0) return LANDUSE_INDUSTRIAL;
	else if (strcmp(s, "construction") == 0) return LANDUSE_INDUSTRIAL;

	else if (strcmp(s, "cemetery") == 0) return LANDUSE_GREEN;
	else if (strcmp(s, "forest") == 0) return LANDUSE_GREEN;
	else if (strcmp(s, "grass") == 0) return LANDUSE_GREEN;
	else if (strcmp(s, "meadow") == 0) return LANDUSE_GREEN;
	else if (strcmp(s, "village_green") == 0) return LANDUSE_GREEN;
	else if (strcmp(s, "recreation_ground") == 0) return LANDUSE_GREEN;
	else if (strcmp(s, "greenfield") == 0) return LANDUSE_GREEN;
	else if (strcmp(s, "field") == 0) return LANDUSE_GREEN;

	else if (strcmp(s, "reservoir") == 0) return LANDUSE_WATER;
	else if (strcmp(s, "basin") == 0) return LANDUSE_WATER;

	else return LANDUSE_UNKNOWN;
}

ATTR_VISITOR(way_visitor) {

	if (strcmp(key, "id") == 0) {
		if (!id_to_long(val, &((struct way *)data)->id)) {
			// fprintf(err_stream, "bad way id '%s'\n", val);
			return;
		}
	}
}

static enum way_type classify_way(struct parse_ctx *ctx, struct way *way) {
	struct tag tag = {0};
	struct tag *ptag = &tag;

	tag.key = "landuse";
	if (tag_mapFind(&ctx->current_tags, &ptag)) {
		enum land_use_type lu = parse_landuse(ptag->val);
		if (lu != LANDUSE_UNKNOWN) {
			way->way_type = WAY_LANDUSE;
			way->que.land_use.type = lu;
			return WAY_LANDUSE;
		}
	}



	tag.key = "highway";
	if (tag_mapFind(&ctx->current_tags, &ptag)) {
		enum road_type rt = parse_road_type(ptag->val);
		way->way_type = WAY_ROAD;
		way->que.road.type = rt;
		return WAY_ROAD;
	}

/*
	tag.key = "building";
	if (tag_mapFind(&ctx->current_tags, &ptag)) {
		enum building_type bt = parse_building_type(ptag->val);
		way->way_type = WAY_BUILDING;
		way->que.building.type = bt;
		return WAY_BUILDING;
	}
*/

	return way->way_type = WAY_UNKNOWN;
}

static int add_node_points(struct parse_ctx *ctx, struct way *way, vec_point_t *out) {
	int i = 0;
	id nid = 0;
	struct node node = {0};
	struct node *pnode = NULL;
	vec_foreach(&way->nodes, nid, i) {
		node.id = nid;
		pnode = &node;
		if (!node_mapFind(&ctx->nodes, &pnode)) {
			// fprintf(err_stream, "nonexistent node ref %ld\n", nid);
			return ERR_OSM;
		}

		if (vec_push(out, pnode->pos) != 0)
			return ERR_MEM;
	}

	return CRACKING;
}

int add_way_to_context(struct parse_ctx *ctx) {
	struct way *way = &ctx->que.way;
	struct tag tag = {0};
	struct tag *ptag = &tag;

	// add all ways in case they're used in relations
	if (way_mapPut(&ctx->ways, &way, HMDR_FAIL) == HMPR_FAILED)
		return ERR_MEM;

	int ret = CRACKING;
	enum way_type type = classify_way(ctx, way);

	// road name and segments
	if (type == WAY_ROAD) {
		tag.key = "name";
		ptag = &tag;
		if (tag_mapFind(&ctx->current_tags, &ptag)) {
			if ((way->que.road.name = strdup(ptag->val)) == NULL)
				return ERR_MEM;
		}

		// add road segments
		if ((ret = add_node_points(ctx, way, &way->que.road.segments)) != CRACKING)
			return ret;

		way->que.road.id = way->id;

		ret = vec_push(&ctx->out.roads, way->que.road) == 0 ? CRACKING : ERR_MEM;
	}

	// land use
	else if (type == WAY_LANDUSE) {
		if ((ret = add_node_points(ctx, way, &way->que.land_use.points)) != CRACKING)
			return ret;

		way->que.land_use.id = way->id;
		ret = vec_push(&ctx->out.land_uses, way->que.land_use) == 0 ? CRACKING : ERR_MEM;
	}

	// building
/*
	else if (type == WAY_BUILDING) {
		if ((ret = add_node_points(ctx, way, &way->que.building.points)) != CRACKING)
			return ret;
		ret = vec_push(&ctx->out.buildings, way->que.building) == 0 ? CRACKING : ERR_MEM;
	}
*/

	// unset current
	clear_current(ctx);

	return ret;
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
		// fprintf(err_stream, "tag tag found inside non-node or way tag '%s'\n", tag_lookup[ctx->current_tag]);
		return ERR_OSM;
	}

	struct tag tag = {0};
	visit_attributes(ctx->attr_start, tag_visitor, &tag);
	if (tag.key == NULL || tag.val == NULL) {
		// fprintf(err_stream, "bad tag or memory error\n");
		return ERR_OSM;
	}

	struct tag *ptag = &tag;
	if (tag_mapPut(&ctx->current_tags, &ptag, HMDR_REPLACE) == HMPR_FAILED)
		return ERR_MEM;

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

struct osm_source {
	int is_file;
	union {
		const char *file_path;

		struct {
			void *buf;
			size_t n;
		};
	} u;
};

static int parse_osm(struct osm_source *src, struct world *out) {
	struct parse_ctx ctx = {0};
	ctx.current_tag = TAG_UNKNOWN;
	init_world(&ctx.out);

	int ret;
	if (src->is_file) {
		ret = (ctx.f = fopen(src->u.file_path, "r")) == NULL ? ERR_FILE_NOT_FOUND : CRACKING;
	} else {
        ret = (ctx.f = fmemopen(src->u.buf, src->u.n, "r")) == NULL ? ERR_IO : CRACKING;
	}

	if (ret == CRACKING) {
		while (true) {
			if (read_line(&ctx) != CRACKING) break;

			struct xml_tag tag = parse_tag(ctx.tag_start);

			switch(tag.type) {
				case TAG_NODE:
					if ((ret = parse_node_tag(&ctx, tag.opening)) != CRACKING)
					{}
						// fprintf(err_stream, "error processing node: %s\n", error_get_message(ret));
					break;

				case TAG_WAY:
					if ((ret = parse_way_tag(&ctx, tag.opening)) != CRACKING)
					{}
						// fprintf(err_stream, "error processing way: %s\n", error_get_message(ret));
					break;

				case TAG_NODE_REF:
					if ((ret = parse_node_ref_tag(&ctx)) != CRACKING)
					{}
						// fprintf(err_stream, "error processing node ref: %s\n", error_get_message(ret));
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
	}

	*out = ctx.out;
	free_context(&ctx);
	return ret;
}

int parse_osm_from_file(const char *path, struct world *out) {
    struct osm_source src = {
            .is_file = 1,
			.u.file_path = path
	};
	return parse_osm(&src, out);
}

int parse_osm_from_buffer(void *buffer, size_t len, struct world *out) {
	struct osm_source src = {
			.is_file = 0,
			.u.buf = buffer,
			.u.n = len
	};

	return parse_osm(&src, out);
}

const char *road_type_lookup[] = {
	"unknown",
	"motorway",
	"primary",
	"secondary",
	"minor",
	"pedestrian"
};

const char *road_type_to_string(enum road_type rt) {
	return road_type_lookup[rt];
}
