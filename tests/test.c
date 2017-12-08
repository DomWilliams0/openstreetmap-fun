#include <osm/parser.h>
#include "acutest.h"
#include "error.h"
#include "world.h"
#include "osm/parser.h"
#include "osm/osm.h"

int create_test_world(struct world *out) {
	err_stream = fopen("/dev/null", "w");
	return parse_osm_from_file("tests/example.osm", out);
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
		point cmp = {54.090175, 12.248263};
		TEST_CHECK(r.segments.data[0].lat == cmp.lat && r.segments.data[0].lon == cmp.lon);
	}
	{
		point cmp = {54.090631, 12.244192};
		TEST_CHECK(r.segments.data[1].lat == cmp.lat && r.segments.data[1].lon == cmp.lon);
	}
	free_world(&w);
}

TEST_LIST = {
	{ "road discovery", test_roads },
	{ NULL, NULL }
};
