#ifndef JSON_2_DATA_H
#define JSON_2_DATA_H

#include"cJSON.h"
#include "stdio.h"

const char * j2d_get_string(const cJSON *data, const char * field_name);
int j2d_get_int(const cJSON *data, const char * field_name, int * out_val);


#endif