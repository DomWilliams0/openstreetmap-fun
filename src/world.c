#include <stdio.h>

#include "pb_encode.h"
#include "world.h"
#include "error.h"
#include "osm/osm.h"

#ifndef NO_PROTOBUF
#include "world.pb.h"
#endif

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

	struct land_use l = {0};
	vec_foreach(&world->land_uses, l, i) {
		if (l.points.data != NULL)
			vec_deinit(&l.points);
	}
	if (world->land_uses.data != NULL)
		vec_deinit(&world->land_uses);
}

void debug_print(struct world *world) {
	printf("%d roads:\n", world->roads.length);

	int i = 0;
	struct road r = {0};
	vec_foreach(&world->roads, r, i) {
		printf("\t%ld - %s with %d segments", r.id, road_type_to_string(r.type), r.segments.length);
		if (r.name != NULL)
			printf(" - %s", r.name);
		printf("\n");
	}

	printf("%d land_uses:\n", world->land_uses.length);

	struct land_use l = {0};
	vec_foreach(&world->land_uses, l, i) {
		printf("\t%ld - %d points\n", l.id, l.points.length);
	}
}

#ifndef NO_PROTOBUF
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
		p.lat = point.lat;
		p.lon = point.lon;

		if (!pb_encode_tag_for_field(stream, field))
			return false;

		if (!pb_encode_submessage(stream, Point_fields, &p))
			return false;
	}

	return true;
}

static RoadType convert_road_type(enum road_type rt) {
	switch (rt) {
		case ROAD_MOTORWAY:
			return RoadType_R_MOTORWAY;
		case ROAD_PRIMARY:
			return RoadType_R_PRIMARY;
		case ROAD_SECONDARY:
			return RoadType_R_SECONDARY;
		case ROAD_MINOR:
			return RoadType_R_MINOR;
		case ROAD_RESIDENTIAL:
			return RoadType_R_RESIDENTIAL;
		case ROAD_PEDESTRIAN:
			return RoadType_R_PEDESTRIAN;
		case ROAD_UNKNOWN:
		default:
			return RoadType_R_UNKNOWN;
	}
}

static LandUseType convert_land_use_type(enum land_use_type lu) {
	switch (lu) {
		case LANDUSE_RESIDENTIAL:
			return LandUseType_LU_RESIDENTIAL;
		case LANDUSE_COMMERCIAL:
			return LandUseType_LU_COMMERCIAL;
		case LANDUSE_AGRICULTURE:
			return LandUseType_LU_AGRICULTURE;
		case LANDUSE_INDUSTRIAL:
			return LandUseType_LU_INDUSTRIAL;
		case LANDUSE_GREEN:
			return LandUseType_LU_GREEN;
		case LANDUSE_WATER:
			return LandUseType_LU_WATER;
		case LANDUSE_UNKNOWN:
		default:
			return LandUseType_LU_UNKNOWN;
	}
}

static bool encode_roads(pb_ostream_t *stream, const pb_field_t *field, void * const *arg) {
	struct world *world = (struct world *)*arg;

	int i = 0;
	struct road road = {0};
	vec_foreach(&world->roads, road, i) {
		Road r = Road_init_zero;
		r.id = road.id;
		r.type = convert_road_type(road.type);
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

static bool encode_land_uses(pb_ostream_t *stream, const pb_field_t *field, void * const *arg) {
	struct world *world = (struct world *)*arg;

	int i = 0;
	struct land_use land_use = {0};
	vec_foreach(&world->land_uses, land_use, i) {
		LandUse l = LandUse_init_zero;

		l.id = land_use.id;
		l.type = convert_land_use_type(land_use.type);
		l.points.funcs.encode = encode_points;
		l.points.arg = &land_use.points;

		if (!pb_encode_tag_for_field(stream, field))
			return false;

		if (!pb_encode_submessage(stream, LandUse_fields, &l))
			return false;
	}

	return true;
}

static bool dump_to_file_safe(struct world *world, FILE *file) {

	World msg = World_init_zero;
	msg.roads.funcs.encode = encode_roads;
	msg.roads.arg = world;

	msg.land_uses.funcs.encode = encode_land_uses;
	msg.land_uses.arg = world;

	pb_ostream_t os;
	os.callback = write_callback;
	os.state = file;
	os.max_size = SIZE_MAX;

	return pb_encode(&os, World_fields, &msg);
}
#endif

bool dump_to_file(struct world *world, char *path) {

#ifndef NO_PROTOBUF
	FILE *file = fopen(path, "wb");
	if (file == NULL) {
		perror("fopen");
		return false;
	}

	bool ret = dump_to_file_safe(world, file);
	fflush(file);
	fclose(file);

	return ret;
#else
	(void)(world);
	(void)(path);
    return false;
#endif
}
