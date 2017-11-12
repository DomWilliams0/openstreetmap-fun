#include "acutest.h"

void test_hiya() {
	TEST_CHECK(1 == 1);
}

TEST_LIST = {
	{ "nothing much", test_hiya },
	{ NULL, NULL }
};
