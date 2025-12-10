/* Process APIs
 */

#include "TaHomaCtl.h"

#include <json-c/json.h>

void func_Tgw(const char *){
	struct ResponseBuffer buff = {NULL};

	callAPI("setup/gateways", &buff);

	if(debug)
		printf("*D* Resp: '%s'\n", buff.memory ? buff.memory : "NULL data");

		/* Display result */
	if(buff.memory){
		struct json_object *parsed_json = json_tokener_parse(buff.memory);
		struct json_object *obj;

		if(json_object_is_type(parsed_json, json_type_array)){	/* 1st object is an array */
			struct json_object *first_object = json_object_array_get_idx(parsed_json, 0);
			if(first_object && json_object_is_type(first_object, json_type_object)){
				if(json_object_object_get_ex(first_object, "gatewayId", &obj)){
					if(json_object_is_type(obj, json_type_string))
						printf("gatewayId : %s\n", json_object_get_string(obj));
				} else
					fputs("*E* gatewayId not found ", stderr);
			} else
				fputs("*E* Empty or unexpected object returned", stderr);
		} else
			fputs("*E* Returned object is not an array", stderr);

		json_object_put(parsed_json);
	}
	
	freeResponse(&buff);
}


