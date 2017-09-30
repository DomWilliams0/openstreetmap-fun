#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>
#include <error.h>

#include "osm_parse.h"

enum tag_type {
	TAG_BOUNDS,
	TAG_NODE,
	TAG_TAG,
	TAG_WAY,
	TAG_UNKNOWN
};

struct xml_tag {
	enum tag_type type;
	bool opening;
};

const char *tag_lookup[] = {
	"bounds",
	"node",
	"tag",
	"way",
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

void add_node_to_context(struct parse_ctx *ctx) {
	struct node *node = &ctx->que.node;
	printf("adding node id '%lu'\n", node->id);
	// TODO actually add to context list and realloc if necessary

	ctx->current_tag = TAG_UNKNOWN;
}

ATTR_VISITOR(bounds_visitor) {
	double dval = strtold(val, NULL);
	int index = -1;

	if (strcmp(key, "minlat") == 0)
		index = 0;
	else if (strcmp(key, "maxlat") == 0)
		index = 1;
	else if (strcmp(key, "minlon") == 0)
		index = 2;
	else if (strcmp(key, "maxlon") == 0)
		index = 3;

	if (index != -1)
		((double *)data)[index] = dval;
}

int parse_bounds_tag(struct parse_ctx *ctx) {

	double lat_lon[4] = {0};
	visit_attributes(ctx->attr_start, bounds_visitor, &lat_lon);

	if (lat_lon[0] == 0 ||
		lat_lon[1] == 0 ||
		lat_lon[2] == 0 ||
		lat_lon[3] == 0) {
		printf("bad bounds\n");
		return ERR_OSM;
	}


	printf("%.7lf\n", lat_lon[0]);
	printf("%.7lf\n", lat_lon[1]);
	printf("%.7lf\n", lat_lon[2]);
	printf("%.7lf\n", lat_lon[3]);


	return CRACKING;
}

ATTR_VISITOR(node_visitor) {

	if (strcmp(key, "id") == 0) {
		char *str_end;
		long long_id = strtol(val, &str_end, 10);
		if (*str_end != '\0') {
			printf("bad node id '%s'\n", val);
			return;
		}
		printf("got an id '%ld'\n", long_id);
		((struct node *)data)->id = long_id;
	}

	else if (strcmp(key, "lat") == 0) {
		((struct node *)data)->lat = strtold(val, NULL);;
	}

	else if (strcmp(key, "lon") == 0) {
		((struct node *)data)->lon = strtold(val, NULL);;
	}
}

int parse_node_tag(struct parse_ctx *ctx, bool opening) {
	// closing
	if (!opening) {
		add_node_to_context(ctx);
		return CRACKING;
	}

	struct node *node = &ctx->que.node;
	memset(node, '\0', sizeof(struct node));
	visit_attributes(ctx->attr_start, node_visitor, node);
	// TODO lon and lat converted to coords using bounds

	// uh oh
	if (node->id == 0 || node->lat == 0 || node->lon == 0) {
		printf("bad node missing id/lat/lon\n");
		return ERR_OSM;
	}

	// single line
	if (line_ends_with_close_tag(ctx)) {
		add_node_to_context(ctx);

	} else {
		// has more lines, dont store yet
		ctx->current_tag = TAG_NODE;
		puts("oho carrying on");
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

int parse_tag_tag(struct parse_ctx *ctx) {
	if (ctx->current_tag != TAG_NODE) {
		printf("tag tag found inside non-node tag '%d'\n", ctx->current_tag);
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

int parse_xml(char *file_path, struct context *out) {
	struct parse_ctx ctx = {0};
	ctx.current_tag = TAG_UNKNOWN;

	int ret = CRACKING;

	// open file
	if ((ret = open_file(&ctx, file_path)) == CRACKING) {

		while (true) {
			if (read_line(&ctx) != CRACKING) break;

			struct xml_tag tag = parse_tag(ctx.tag_start);

			switch(tag.type) {
				case TAG_BOUNDS:
					if (parse_bounds_tag(&ctx) != CRACKING)
						goto end_scan;

					break;
				case TAG_NODE:
					parse_node_tag(&ctx, tag.opening);
					break;

				case TAG_TAG:
					parse_tag_tag(&ctx);
					break;

				default:
					continue;

			}

		}
end_scan:

		fclose(ctx.f);
		ctx.f = NULL;
	}


	return ret;
}
