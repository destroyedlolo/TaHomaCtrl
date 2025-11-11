/* Wait for TaHoma's advertising
 */

#include "TaHomaCtl.h"

#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-common/simple-watch.h> 
#include <avahi-common/error.h>
#include <avahi-common/defs.h> 

// The service type we are looking for
#define SERVICE_TYPE "_kizboxdev._tcp"

static AvahiSimplePoll *simple_poll = NULL;
static AvahiServiceBrowser *browser = NULL;

void func_scan(const char *){
		/* Remove old references */
	clean(&tahoma);
	clean(&token);
	clean(&url);

		/* Avahi listener */
	AvahiClient *client = NULL;
	int error;
	int ret = 1;

	for(;;){	// Simulate try/catch
		if(!(simple_poll = avahi_simple_poll_new())){
		}
	}
}
