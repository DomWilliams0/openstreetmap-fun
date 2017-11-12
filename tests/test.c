#include "acutest.h"
#include "error.h"
#include "world.h"
#include "osm/osm.h"
#include "osm/parser.h"

int create_test_world(struct world *out) {
	return parse_osm("tests/example.osm", out);
}

void test_roads() {
	struct world w;
	TEST_CHECK(create_test_world(&w) == CRACKING);

	TEST_CHECK(w.roads.length == 1);
	struct road r = w.roads.data[0];
	TEST_CHECK(r.name != NULL);
	TEST_CHECK(strcmp(r.name, "Pastower Straße") == 0);

	TEST_CHECK(r.segments.length == 2);

	{
		point cmp = {95, 18};
		TEST_CHECK(memcmp(r.segments.data + 0, &cmp, sizeof(point)) == 0);
	}
	{
		point cmp = {0, 0};
		TEST_CHECK(memcmp(r.segments.data + 1, &cmp, sizeof(point)) == 0);
	}
	free_world(&w);
}

void test_buildings() {
	struct world w;
	TEST_CHECK(create_test_world(&w) == CRACKING);

	TEST_CHECK(w.buildings.length == 1);
	struct building b = w.buildings.data[0];
	TEST_CHECK(b.points.length == 2);

	{
		point cmp = {174, 19};
		TEST_CHECK(memcmp(b.points.data + 0, &cmp, sizeof(point)) == 0);
	}
	{
		point cmp = {179, 23};
		TEST_CHECK(memcmp(b.points.data + 1, &cmp, sizeof(point)) == 0);
	}
	free_world(&w);
}

void test_bounds() {
	struct world w;
	TEST_CHECK(create_test_world(&w) == CRACKING);

	TEST_CHECK(w.bounds[0] == 227);
	TEST_CHECK(w.bounds[1] == 23);

	free_world(&w);
}

TEST_LIST = {
	{ "bounds", test_bounds },
	{ "road discovery", test_roads },
	{ "building discovery", test_buildings },
	{ NULL, NULL }
};
