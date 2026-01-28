/* Wait for TaHoma's advertising
 *
 * From client-browse-services's souce code
 */

#include "TaHomaCtl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-common/simple-watch.h> 
#include <avahi-common/error.h>
#include <avahi-common/defs.h> 
#include <avahi-common/malloc.h>

AvahiProtocol avahiIP = AVAHI_PROTO_UNSPEC;

	/* The service type we are looking for */
#define SERVICE_TYPE "_kizboxdev._tcp"

static AvahiSimplePoll *simple_poll = NULL;

static void resolve_callback(
	AvahiServiceResolver *r,
	AvahiIfIndex interface,
	AvahiProtocol protocol,
	AvahiResolverEvent event,
	const char *name,
	const char *type,
	const char *domain,
	const char *host_name,
	const AvahiAddress *address,
	uint16_t aport,
	AvahiStringList *txt,
	AvahiLookupResultFlags flags,
	void* userdata){
	assert(r);

		/* Called whenever a service has been resolved successfully or timed out */
	switch (event) {
		case AVAHI_RESOLVER_FAILURE:
			fprintf(stderr, "*E* (Resolver) Failed to resolve service '%s' of type '%s' in domain '%s': %s\n", name, type, domain, avahi_strerror(avahi_client_errno(avahi_service_resolver_get_client(r))));
			break;
		case AVAHI_RESOLVER_FOUND: {
			char a[AVAHI_ADDRESS_STR_MAX], *t;
			if(verbose || debug)
				printf("*I* Service '%s' of type '%s' in domain '%s':\n", name, type, domain);
			avahi_address_snprint(a, sizeof(a), address);
			t = avahi_string_list_to_string(txt);
			if(verbose || debug)
				fprintf(stderr,
					"\t%s:%u (%s)\n"
					"\tTXT=%s\n"
					"\tcookie is %u\n"
					"\tis_local: %i\n"
					"\tour_own: %i\n"
					"\twide_area: %i\n"
					"\tmulticast: %i\n"
					"\tcached: %i\n",
					host_name, aport, a,
					t,
					avahi_string_list_get_service_cookie(txt),
					!!(flags & AVAHI_LOOKUP_RESULT_LOCAL),
					!!(flags & AVAHI_LOOKUP_RESULT_OUR_OWN),
					!!(flags & AVAHI_LOOKUP_RESULT_WIDE_AREA),
					!!(flags & AVAHI_LOOKUP_RESULT_MULTICAST),
					!!(flags & AVAHI_LOOKUP_RESULT_CACHED)
				);

			FreeAndSet(&tahoma, host_name);
			FreeAndSet(&ip, a);
			port = aport;

			avahi_free(t);
			avahi_simple_poll_quit(simple_poll);
			break;
		}
	}
	avahi_service_resolver_free(r);
}

static void browse_callback(
	AvahiServiceBrowser *b,
	AvahiIfIndex interface,
	AvahiProtocol protocol,
	AvahiBrowserEvent event,
	const char *name,
	const char *type,
	const char *domain,
	AvahiLookupResultFlags flags,
	void* userdata){
	AvahiClient *c = userdata;
	assert(b);

		/* Called whenever a new services becomes available on the LAN or is removed from the LAN */
	switch (event) {
		case AVAHI_BROWSER_FAILURE:
			fprintf(stderr, "*E* (Browser) %s\n", avahi_strerror(avahi_client_errno(avahi_service_browser_get_client(b))));
			avahi_simple_poll_quit(simple_poll);
			return;
		case AVAHI_BROWSER_NEW:
			if(debug)
				printf("*D* (Browser) NEW: service '%s' of type '%s' in domain '%s'\n", name, type, domain);
			/* We ignore the returned resolver object. In the callback
			   function we free it. If the server is terminated before
			   the callback function is called the server will free
			   the resolver for us. */
			if(!(avahi_service_resolver_new(c, interface, protocol, name, type, domain, AVAHI_PROTO_UNSPEC, 0, resolve_callback, c)))
				fprintf(stderr, "*E* Failed to resolve service '%s': %s\n", name, avahi_strerror(avahi_client_errno(c)));
			break;
		case AVAHI_BROWSER_REMOVE:
			if(debug)
				printf("*D* (Browser) REMOVE: service '%s' of type '%s' in domain '%s'\n", name, type, domain);
			break;
		case AVAHI_BROWSER_ALL_FOR_NOW:
		case AVAHI_BROWSER_CACHE_EXHAUSTED:
			if(debug)
				printf("*D* (Browser) %s\n", event == AVAHI_BROWSER_CACHE_EXHAUSTED ? "CACHE_EXHAUSTED" : "ALL_FOR_NOW");
			break;
	}
}

static void client_callback(AvahiClient *c, AvahiClientState state, AVAHI_GCC_UNUSED void * userdata) {
	assert(c);

		/* Called whenever the client or server state changes */
	if (state == AVAHI_CLIENT_FAILURE) {
		fprintf(stderr, "*E* Server connection failure: %s\n", avahi_strerror(avahi_client_errno(c)));
		avahi_simple_poll_quit(simple_poll);
	}
}

void func_scan(const char *){
	AvahiClient *client = NULL;
	AvahiServiceBrowser *sb = NULL;
	int error;

		/* Remove old references */
	clean(&tahoma);
	clean(&url);

		/* ***
		 * Avahi listener 
		 * ***/

		/* Allocate main loop object */
	if(!(simple_poll = avahi_simple_poll_new())){
		fprintf(stderr, "*E* Failed to create simple poll object.\n");
		goto cleanup;
	}

	client = avahi_client_new(avahi_simple_poll_get(simple_poll), 0, client_callback, NULL, &error);
	if(!client){
		fprintf(stderr, "*E* Failed to create client: %s\n", avahi_strerror(error));
		goto cleanup;
	}

		/* Create the service browser */
	if(!(sb = avahi_service_browser_new(client, AVAHI_IF_UNSPEC, avahiIP, "_kizboxdev._tcp", NULL, 0, browse_callback, client))) {
		fprintf(stderr, "*E* Failed to create service browser: %s\n", avahi_strerror(avahi_client_errno(client)));
		goto cleanup;
	}

		/* Wait for events */
	avahi_simple_poll_loop(simple_poll);

		/* The TaHoma may have been discovered : trying
		 * to build connection informations
		 */
	buildURL();

cleanup:
		/* Cleanup things */
	if (sb)
		avahi_service_browser_free(sb);

	if (client)
		avahi_client_free(client);

	if (simple_poll)
		avahi_simple_poll_free(simple_poll);
}
