#include <stdlib.h>

#include "osm.h"

// http://www.cse.yorku.ca/~oz/hash.html
static uint64_t djb2(char *s) {
	unsigned long hash = 5381;
	for(char *c = s; *c; ++c) {
		hash = ((hash << 5) + hash) + *c;
	}
	return hash;
}

#define TAG_CMP(left, right) strcmp(left->key, right->key)
#define TAG_HASH(entry) djb2(entry->key)
DECLARE_HASHMAP(tag_map, TAG_CMP, TAG_HASH, free, realloc)
