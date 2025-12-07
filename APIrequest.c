/* Call Tahoma's API
 */

#include "TaHomaCtl.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

CURL *curl = NULL;
static struct curl_slist *global_resolve_list = NULL;	/* forced resolver */
static struct curl_slist *global_headers = NULL;				/* Headers */

void curl_cleanup(void){
		/* internally protected against NULL pointer */
	curl_easy_cleanup(curl);	
	curl_slist_free_all(global_resolve_list);
	curl_slist_free_all(global_headers);
	curl_global_cleanup();
}

void buildURL(void){
	if(!tahoma || !ip || !port || !token)	/* Some information are missing */
		return;

		/* Build target base URL */
	if(url){
		free(url);
		url = NULL;
	}

	url_len = strlen("https://:/enduser-mobile-web/1/enduserAPI/");
	url_len += strlen(tahoma);
	url_len += 5; /* port: 65535 */

	url = malloc(url_len + 1);
	assert(url);

	sprintf(url, "https://%s:%u/enduser-mobile-web/1/enduserAPI/", tahoma, port);
	if(debug)
		printf("*D* url: '%s'\n", url);

		/* Build DNS overwriting */
	if(global_resolve_list){
		curl_slist_free_all(global_resolve_list);
		global_resolve_list = NULL;
	}

	char resolve_entry[strlen(tahoma) + strlen(ip) + 3]; /* host:port:ip */
	sprintf(resolve_entry, "%s:%u:%s", tahoma, port, ip);
	if(debug)
		printf("*D* Resolution: '%s'\n", resolve_entry);

	if(!(global_resolve_list = curl_slist_append(NULL, resolve_entry))){
		fputs("*E* Failed to force DNS resolution", stderr);
		return;
	}

	curl_easy_setopt(curl, CURLOPT_RESOLVE, global_resolve_list);

		/* Authorization header string */
	if(global_headers){
		curl_slist_free_all(global_headers);
		global_headers = NULL;
	}
	
	char auth_header[strlen("Authorization: Bearer ") + strlen(token) + 1];
	strcpy(auth_header, "Authorization: Bearer ");
	strcat(auth_header, token);

	if(debug)
		printf("*D* Auth header : '%s'\n", auth_header);
	
	if(!(global_headers = curl_slist_append(NULL, token))){
		fputs("*E* Failed to set header", stderr);
		return;
	}
	
	/* Other headers if needed */
	global_headers = curl_slist_append(global_headers, "Content-Type: application/json");	

	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, global_headers);
}
