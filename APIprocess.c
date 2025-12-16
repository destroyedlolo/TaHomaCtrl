/* Process APIs
 */

#include "TaHomaCtl.h"

#include <json-c/json.h>

#define OBJPATH(...) (const char*[]){ __VA_ARGS__ }

struct json_object *getObj(struct json_object *parent, const char *path[]){
	struct json_object *obj = parent;

	for(int i=0; path[i]; ++i){
		if(debug)
			printf("*D* %d: '%s'\n", i, path[i]);

		if(!obj){
			if(debug)
				fprintf(stderr, "*E* Broken path at %dth\n", i);
			return NULL;
		}
		obj = json_object_object_get(obj, path[i]);
	}

	return obj;
}

const char *getObjString(struct json_object *parent, const char *path[]){
	struct json_object *obj = getObj(parent, path);
	if(!obj)
		return NULL;

	if(json_object_is_type(obj, json_type_string))
		return json_object_get_string(obj);
	else if(debug)
		fputs("*E* Not a string\n", stderr);

	return NULL;
}

int getObjInt(struct json_object *parent, const char *path[]){
	struct json_object *obj = getObj(parent, path);
	if(!obj)
		return 0;

	if(json_object_is_type(obj, json_type_int))
		return json_object_get_int(obj);
	else if(debug)
		fputs("*E* Not an integer\n", stderr);

	return 0;
}

bool getObjBool(struct json_object *parent, const char *path[]){
	struct json_object *obj = getObj(parent, path);
	if(!obj)
		return false;

	if(json_object_is_type(obj, json_type_boolean))
		return json_object_get_boolean(obj);
	else if(debug)
		fputs("*E* Not a boolean\n", stderr);

	return false;
}

static const char *affString(const char *v){
	if(v)
		return v;
	else
		return "Not found";
}

void func_Tgw(const char *){
	struct ResponseBuffer buff = {NULL};

	callAPI("setup/gateways", &buff);
	if(debug)
		printf("*D* Resp: '%s'\n", buff.memory ? buff.memory : "NULL data");

		/* Display result */
	if(buff.memory){
		struct json_object *parsed_json = json_tokener_parse(buff.memory);

		if(json_object_is_type(parsed_json, json_type_array)){	/* 1st object is an array */
			struct json_object *first_object = json_object_array_get_idx(parsed_json, 0);
			if(first_object){
				printf("gatewayId : %s\n", affString(getObjString(first_object, OBJPATH( "gatewayId", NULL ) )));
				printf("Connected : %s\n", affString(getObjString(first_object, OBJPATH( "connectivity", "status", NULL ) )));
				printf("protocolVersion : %s\n", affString(getObjString(first_object, OBJPATH( "connectivity", "protocolVersion", NULL ) )));

			} else
				fputs("*E* Empty or unexpected object returned", stderr);
		} else
			fputs("*E* Returned object is not an array", stderr);

		json_object_put(parsed_json);
	}
	
	freeResponse(&buff);
}

void func_Devs(const char *){
	struct ResponseBuffer buff = {NULL};

	callAPI("setup/devices", &buff);
	if(debug)
		printf("*D* Resp: '%s'\n", buff.memory ? buff.memory : "NULL data");

		/* Process result */
	if(buff.memory){
		struct json_object *res= json_tokener_parse(buff.memory);

		if(json_object_is_type(res, json_type_array)){	/* 1st object is an array */
			size_t nbr = json_object_array_length(res);
			if(debug || verbose)
				printf("*I* %ld devices\n", nbr);

			for(size_t idx=0; idx < nbr; ++idx){
				struct json_object *obj = json_object_array_get_idx(res, idx);

				if(obj){
					printf("%s [%s]\n", 
						affString(getObjString(obj, OBJPATH( "label", NULL ) )),
						affString(getObjString(obj, OBJPATH( "controllableName", NULL ) ))
					);

					printf("\tURL : %s\n", 
						affString(getObjString(obj, OBJPATH( "deviceURL", NULL ) ))
					);

					printf("\tType : %d, subsystemId : %d\n", 
						getObjInt(obj, OBJPATH( "type", NULL ) ),
						getObjInt(obj, OBJPATH( "subsystemId", NULL ) )
					);

					printf("\t%ssynced, %senabled, %savailable\n",
						getObjBool(obj, OBJPATH( "synced", NULL ) ) ? "":"Not ",
						getObjBool(obj, OBJPATH( "enabled", NULL ) ) ? "":"Not ",
						getObjBool(obj, OBJPATH( "available", NULL ) ) ? "":"Not "
					);

					printf("\t\tType: %s\n",
						affString(getObjString(obj, OBJPATH( "definition", "type", NULL ) ))
					);
						
				} else
					fprintf(stderr, "*E* Can't get %ld\n", idx);
			}
		} else
			fputs("*E* Returned object is not an array", stderr);

		json_object_put(res);
	}
	freeResponse(&buff);
}
