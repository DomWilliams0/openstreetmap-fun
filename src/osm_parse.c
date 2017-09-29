#include <stdio.h>
#include <stdbool.h>
#include <error.h>

#include "osm_parse.h"

struct parse_ctx {
	FILE *f;
};

int open_file(struct parse_ctx *ctx, char *file_path) {
	ctx->f = fopen(file_path, "r");
	return ctx->f != NULL ? CRACKING : ERR_FILE_NOT_FOUND;
}

int parse_xml(char *file_path, struct context *out) {
	struct parse_ctx ctx = {0};

	int opened = open_file(&ctx, file_path);
	if (opened != CRACKING)
		return opened;

	fclose(ctx.f);

	return CRACKING;
}
