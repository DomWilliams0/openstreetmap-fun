#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <strings.h>
#include <error.h>

#include "osm_parse.h"

const int LINE_PREFIX_LEN = 32;

enum tag_type {
	TAG_BOUNDS,
	TAG_NODE,
	TAG_TAG,
	TAG_WAY,
	TAG_UNKNOWN
};

struct tag {
	enum tag_type type;
	bool opening;
};

const char *tag_lookup[] = {
	"bounds",
	"node",
	"tag",
	"way",
};

struct tag parse_tag(char *tag_in) {
	struct tag out = {
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
	printf("len is %d and n %lu\n", len, ctx->n);
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
		printf("getline %ld\n", ctx->n);
		int read = getline(&ctx->full_line, &ctx->n, ctx->f);

		// possibly at the end too
		if (read == -1) {
			free(ctx->full_line);
			ctx->full_line = NULL;
			ctx->line_end = NULL;
			ctx->tag_start = NULL;
			return ERR_IO;
		}

		ctx->line_end = ctx->full_line + strlen(ctx->full_line);

		if (find_tag(ctx->full_line, &ctx->tag_start) == CRACKING)
			return CRACKING;
	}

	return ERR_IO; // never gonna get here
}

char *parse_attribute(char *line, const char *key) {
	char *str = strstr(line, key);
	if (str == NULL)
		return "";

	char *start = str + strlen(key) + 2; // ="
	int len = find_char(start, '"');
	if (len < 0)
		return "";

	start[len] = '\0';
	return start;
}

void add_node_to_context(struct parse_ctx *ctx) {
	struct node *node = &ctx->que.node;
	printf("adding node id '%lu'\n", node->id);
	// TODO actually add to context list and realloc if necessary

	ctx->current_tag = TAG_UNKNOWN;
}

int parse_node_tag(struct parse_ctx *ctx, bool opening) {
	// closing
	if (!opening) {
		add_node_to_context(ctx);
		return CRACKING;
	}

	char *line = ctx->tag_start + strlen(ctx->tag_start) + 1;

	// extract id
	char *str_id = parse_attribute(line, "id");
	char *str_end;
	long long_id = strtol(str_id, &str_end, 10);
	if (*str_end != '\0') {
		printf("bad node id '%s'\n", str_id);
		return ERR_OSM;
	}
	ctx->que.node.id = long_id;

	// TODO lon and lat converted to coords using bounds

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

int parse_tag_tag(struct parse_ctx *ctx) {
	if (ctx->current_tag != TAG_NODE) {
		printf("tag tag found inside non-node tag '%d'\n", ctx->current_tag);
		return ERR_OSM;
	}
	// TODO parse key/val and put into map
	printf("adding tag\n");
	return CRACKING;
}

int parse_xml(char *file_path, struct context *out) {
	struct parse_ctx ctx = {0};
	ctx.current_tag = TAG_UNKNOWN;

	int ret = CRACKING;

	// open file
	if ((ret = open_file(&ctx, file_path)) == CRACKING) {

		while (true) {
			int ret = read_line(&ctx);
			if (ret != CRACKING) break;

			struct tag tag = parse_tag(ctx.tag_start);

			switch(tag.type) {
				case TAG_NODE:
					ret = parse_node_tag(&ctx, tag.opening);
					break;

				case TAG_TAG:
					ret = parse_tag_tag(&ctx);
					break;

				default:
					continue;

			}
			// TODO how use return codes?

			// do something

		}

		fclose(ctx.f);
		ctx.f = NULL;
	}


	return ret;
}
