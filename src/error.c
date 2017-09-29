#include "error.h"

const char *error_get_message(int err)
{
	switch(err)
	{
		case CRACKING:
			return "No error here, success!";
		case ERR_FILE_NOT_FOUND:
			return "File not found";
		case ERR_IO:
			return "IO error";
		case ERR_OSM:
			return "OSM format error";
		default:
			return "Unknown error code";
	}
}
