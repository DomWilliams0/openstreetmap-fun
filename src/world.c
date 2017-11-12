#include <stdio.h>

#include "pb_encode.h"
#include "world.h"
#include "error.h"
#include "osm/osm.h"
#include "world.pb.h"

int init_world(struct world *world) {
	vec_init(&world->roads);
	return CRACKING;
}

void free_world(struct world *world) {
	int i = 0;
	struct road r = {0};
	vec_foreach(&world->roads, r, i) {
		if (r.name != NULL)
			free(r.name);
		if (r.segments.data != NULL)
			vec_deinit(&r.segments);
	}

	if (world->roads.data != NULL)
		vec_deinit(&world->roads);

	struct building b = {0};
	vec_foreach(&world->buildings, b, i) {
		if (b.points.data != NULL)
			vec_deinit(&b.points);
	}

	if (world->buildings.data != NULL)
		vec_deinit(&world->buildings);
}

void debug_print(struct world *world) {
	printf("World bounds: %d, %d\n", world->bounds[0], world->bounds[1]);
	printf("%d roads:\n", world->roads.length);

	int i = 0;
	struct road r = {0};
	vec_foreach(&world->roads, r, i) {
		printf("\t%s with %d segments", road_type_to_string(r.type), r.segments.length);
		if (r.name != NULL)
			printf(" - %s", r.name);
		printf("\n");
	}

	printf("%d buildings:\n", world->buildings.length);

	struct building b = {0};
	vec_foreach(&world->buildings, b, i) {
		printf("\t%d points\n", b.points.length);
	}
}

static bool write_callback(pb_ostream_t *stream, const uint8_t *buf, size_t count) {
	return fwrite(buf, 1, count, stream->state) == count;
}

static bool encode_string(pb_ostream_t *stream, const pb_field_t *field, void * const *arg) {
	char *s = (char *)*arg;
	if (s == NULL)
		return true;

	if (!pb_encode_tag_for_field(stream, field))
		return false;

	return pb_encode_string(stream, s, strlen(s));
}

static bool encode_points(pb_ostream_t *stream, const pb_field_t *field, void * const *arg) {

	vec_point_t *vec = (vec_point_t *)*arg;
	int i = 0;
	point point = {0};
	vec_foreach(vec, point, i) {
		Point p = Point_init_zero;
		p.x = point.x;
		p.y = point.y;

		if (!pb_encode_tag_for_field(stream, field))
			return false;

		if (!pb_encode_submessage(stream, Point_fields, &p))
			return false;
	}

	return true;
}

static bool encode_roads(pb_ostream_t *stream, const pb_field_t *field, void * const *arg) {
	struct world *world = (struct world *)*arg;

	int i = 0;
	struct road road = {0};
	vec_foreach(&world->roads, road, i) {
		Road r = Road_init_zero;
		r.type = RoadType_MOTORWAY;
		r.name.funcs.encode = encode_string;
		r.name.arg = road.name;
		r.segments.funcs.encode = encode_points;
		r.segments.arg = &road.segments;

		if (!pb_encode_tag_for_field(stream, field))
			return false;

		if (!pb_encode_submessage(stream, Road_fields, &r))
			return false;
	}

	return true;
}

static bool dump_to_file_safe(struct world *world, FILE *file) {

	World msg = World_init_zero;
	msg.bounds_x = world->bounds[0];
	msg.bounds_y = world->bounds[1];

	msg.roads.funcs.encode = encode_roads;
	msg.roads.arg = world;

	// TODO buildings

	pb_ostream_t os;
	os.callback = write_callback;
	os.state = file;
	os.max_size = SIZE_MAX;

	return pb_encode(&os, World_fields, &msg);
}

bool dump_to_file(struct world *world, char *path) {
	FILE *file = fopen(path, "wb");
	if (file == NULL) {
		perror("fopen");
		return false;
	}

	bool ret = dump_to_file_safe(world, file);
	fflush(file);
	fclose(file);

	return ret;
}
