#include "json_to_data.h"

const char * j2d_get_string(const cJSON *data, const char * field_name){
    const cJSON * field_obj = cJSON_GetObjectItem(data, field_name);
    if (field_obj == NULL){
        printf("%s not available\n", field_name);
        return NULL;
    }
    if (!cJSON_IsString(field_obj)){
        printf("%s is not a String\n", field_name);
        return NULL;
    }
    return field_obj->valuestring;
}
int j2d_get_int(const cJSON *data, const char * field_name, int * out_val){
    const cJSON * field_obj = cJSON_GetObjectItem(data, field_name);
    if (field_obj == NULL){
        printf("%s not available\n", field_name);
        return -1;
    }
    if (!cJSON_IsNumber(field_obj)){
        printf("%s is not a Valid Number\n", field_name);
        return -1;
    }
    *out_val = field_obj->valueint;
    return 0;
}

