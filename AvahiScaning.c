/* Wait for TaHoma's advertising
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

AvahiProtocol avahiIP = AVAHI_PROTO_UNSPEC;

// The service type we are looking for
#define SERVICE_TYPE "_kizboxdev._tcp"

static AvahiSimplePoll *simple_poll = NULL;
static AvahiServiceBrowser *browser = NULL;

/**
 * @brief Set a setting after clearing it if needed
 * @details Settings are dynamically allocated
 */
static void store(char **s, const char *v){
	if(*s)	// Already feed
		free(*s);

	*s = strdup(v);
	assert(*s);
}

/**
 * @brief Service Resolver Callback.
 * @details Called when detailed service information (address, port) is available.
 */
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
	void *userdata)
{
	assert(r);

	switch(event) {
	case AVAHI_RESOLVER_FAILURE: // Handle resolution failure
		fprintf(stderr, "*W* [Resolver] Failed to resolve service '%s': %s\n", 
			name, avahi_strerror(avahi_client_errno((AvahiClient*)userdata)));
		break;
	case AVAHI_RESOLVER_FOUND: {
			char a[AVAHI_ADDRESS_STR_MAX];
			avahi_address_snprint(a, sizeof(a), address);	// Convert AvahiAddress to human-readable string

			if(verbose){
				printf("*I* Service '%s' found and resolved:\n", name);
				printf("*I*\tType: %s, Domain: %s\n", type, domain);
				printf("*I*\tHost: '%s', Address: '%s', Port: %u\n", host_name, a, aport);
			}
			store(&tahoma, host_name);
			store(&ip, a);
			port = aport;

			if(simple_poll)
				avahi_simple_poll_quit(simple_poll);
			break;
		}
	}

	avahi_service_resolver_free(r); // Free the resolver after use
}


/**
 * @brief Service Browser Callback.
 * @details Called when a service is added or removed from the network.
 */
static void browse_callback(
	AvahiServiceBrowser *b,
	AvahiIfIndex interface,
	AvahiProtocol protocol,
	AvahiBrowserEvent event,
	const char *name,
	const char *type,
	const char *domain,
	AvahiLookupResultFlags flags,
	void* userdata)
{
	AvahiClient *c = userdata;
	assert(b);

	switch(event){
	case AVAHI_BROWSER_FAILURE: // Handle browser failure
		fprintf(stderr, "*E* (Browser) Failure: %s\n", avahi_strerror(avahi_client_errno(c)));
		avahi_simple_poll_quit(simple_poll);
		break;
	case AVAHI_BROWSER_NEW:
		if(debug)
			printf("*D* New service found: '%s' of type '%s' in domain '%s'\n", name, type, domain);
			
		// Start resolution to get address and port
		if(!(avahi_service_resolver_new(c, interface, protocol, name, type, domain, avahiIP, 0, resolve_callback, c))){
			fprintf(stderr, "*E* Failed to create resolver for '%s': %s\n", name, avahi_strerror(avahi_client_errno(c)));
		}
		break;
	case AVAHI_BROWSER_REMOVE:
		if(debug)
			printf("*D* **Service removed:** '%s' of type '%s' in domain '%s'\n", name, type, domain);
		break;
	case AVAHI_BROWSER_ALL_FOR_NOW:
		if(debug)
			printf("*D* (Browser) Initial browsing complete. Waiting for new services...\n");
		break;
	case AVAHI_BROWSER_CACHE_EXHAUSTED:
		if(debug)
			printf("*D* (Browser) Cache exhausted (initial pass complete or error)... Waiting for new services...\n");
		break;
	}
}


/**
 * @brief Avahi Client State Callback.
 * @details Called when the connection state to the Avahi daemon changes.
 */
static void client_callback(AvahiClient *c, AvahiClientState state, AVAHI_GCC_UNUSED void * userdata) {
	assert(c);

	switch (state) {
	case AVAHI_CLIENT_S_RUNNING: // Connection established, start service browsing
		if(!(browser = avahi_service_browser_new(c, AVAHI_IF_UNSPEC, avahiIP, SERVICE_TYPE, "local", 0, browse_callback, c))){ // Create a new service browser instance
			fprintf(stderr, "*E* Failed to create service browser: %s\n", avahi_strerror(avahi_client_errno(c)));
			avahi_simple_poll_quit(simple_poll);
		}
		break;
	case AVAHI_CLIENT_FAILURE:
		fprintf(stderr, "*E* Connection to Avahi server failed: %s\n", avahi_strerror(avahi_client_errno(c)));
		avahi_simple_poll_quit(simple_poll);
		break;
	case AVAHI_CLIENT_S_REGISTERING:
	case AVAHI_CLIENT_S_COLLISION:
	case AVAHI_CLIENT_CONNECTING:
			// Other states, do nothing for simple browsing
		break;
	}
}

void func_scan(const char *){
		/* Remove old references */
	clean(&tahoma);
	clean(&token);
	clean(&url);

		/* Avahi listener */
	AvahiClient *client = NULL;
	int error;

	if(!(simple_poll = avahi_simple_poll_new())){
		fputs("*E* Failed to create AvahiSimplePoll object.\n", stderr);
		return;
	}

	client = avahi_client_new(avahi_simple_poll_get(simple_poll), 0, client_callback, NULL, &error);
	if(!client){
		fprintf(stderr, "*E* Failed to create Avahi client: %s\n", avahi_strerror(error));
		goto cleanup;
	}

	if(verbose)
		puts("*I* Waiting for Avahi advertisement");
	avahi_simple_poll_loop(simple_poll);

cleanup:
	if(browser)
		avahi_service_browser_free(browser);

	if(client)
		avahi_client_free(client);

	if(simple_poll)
		avahi_simple_poll_free(simple_poll);
}
