/*
 * Avahi Debug Browser + Resolver + Record Browser
 *
 * This program browses mDNS/Avahi services, resolves them, and prints
 * details including records and TXT data.  It has been extended to print
 * TTL (queried via the system resolver when possible), and human-readable
 * DNS class and type names, and readable flags.
 *
 * Compile with: gcc -o avahi_debug1 avahi_debug1.c -lavahi-client -lavahi-common -lresolv
 *
 * Mainly ChatGPT code but sightly debugged by myself.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <time.h>
#include <resolv.h>
#include <arpa/nameser.h>
#include <netinet/in.h>

#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-common/error.h>
#include <avahi-common/alternative.h>
#include <avahi-common/domain.h>
#include <avahi-common/malloc.h>
#include <avahi-common/timeval.h>

/* Forward declarations */
static void browse_callback(AvahiServiceBrowser *b,
							AvahiIfIndex interface,
							AvahiProtocol protocol,
							AvahiBrowserEvent event,
							const char *name,
							const char *type,
							const char *domain,
							AvahiLookupResultFlags flags,
							void *userdata);

static void resolve_callback(AvahiServiceResolver *r,
							 AvahiIfIndex interface,
							 AvahiProtocol protocol,
							 AvahiResolverEvent event,
							 const char *name,
							 const char *type,
							 const char *domain,
							 const char *host_name,
							 const AvahiAddress *address,
							 uint16_t port,
							 AvahiStringList *txt,
							 AvahiLookupResultFlags flags,
							 void *userdata);

static void record_callback(flags){
	/* Added: human-readable flags + support for multiple TTL extraction */

	// --- Human‑readable Avahi flags ---
	const char *flag_str = "";
	if(flags & AVAHI_LOOKUP_RESULT_LOCAL) flag_str = "LOCAL";
	if(flags & AVAHI_LOOKUP_RESULT_OUR_OWN) flag_str = "OUR_OWN";
	if(flags & AVAHI_LOOKUP_RESULT_CACHED) flag_str = "CACHED";

	printf("  Flags: %s", flag_str);

	// --- TTL handling: prefer Avahi TTL if available ---
	unsigned ttl_avahi = record->ttl;
	printf("  Avahi TTL: %u", ttl_avahi);

	if(ttl_avahi == 0)
		printf("  TTL non fourni par Avahi");
	free_dns_rr_list(list);
	puts("");
}

const char *dns_type_to_string(uint16_t type){
	switch(type){
	case T_A: return "A";
	case T_NS: return "NS";
	case T_CNAME: return "CNAME";
	case T_SOA: return "SOA";
	case T_PTR: return "PTR";
	case T_MX: return "MX";
	case T_TXT: return "TXT";
	case T_AAAA: return "AAAA";
	case T_SRV: return "SRV";
	default: return "TYPE_?";
	}
}

static char *timestamp_now(void){
	time_t t = time(NULL);
	struct tm tm;
	localtime_r(&t, &tm);
	char *buf = malloc(64);
	if(!buf) return NULL;
	strftime(buf, 64, "%F %T", &tm);
	return buf;
}

/* Convert Avahi lookup flags to human-readable string */
static void lookup_flags_to_string(AvahiLookupResultFlags flags, char *out, size_t outlen){
	out[0] = '';
	int first = 1;
#ifdef AVAHI_LOOKUP_RESULT_LOCAL
	if(flags & AVAHI_LOOKUP_RESULT_LOCAL){ strncat(out, "LOCAL", outlen - strlen(out) - 1); first = 0; }
#endif
#ifdef AVAHI_LOOKUP_RESULT_OUR_OWN
	if(flags & AVAHI_LOOKUP_RESULT_OUR_OWN){ if(!first) strncat(out, ",", outlen - strlen(out) -1); strncat(out, "OUR_OWN", outlen - strlen(out) - 1); first = 0; }
#endif
#ifdef AVAHI_LOOKUP_RESULT_CACHED
	if(flags & AVAHI_LOOKUP_RESULT_CACHED){ if(!first) strncat(out, ",", outlen - strlen(out) -1); strncat(out, "CACHED", outlen - strlen(out) - 1); first = 0; }
#endif
	if(first) strncat(out, "NONE", outlen - strlen(out) - 1);
}

static void print_txt(AvahiStringList *txt){
	AvahiStringList *l;
	for (l = txt; l; l = l->next){
		char *s = avahi_string_list_to_string(l);
		if(s){
			printf("	TXT: %s\n", s);
			avahi_free(s);
		}
	}
}

/* Query system resolver for a specific record type and return the TTL of the first matching answer.
 * Returns 0 on failure or if no matching RR found. Requires linking with -lresolv.
 */
static unsigned int query_ttl_via_resolv(const char *name, int qtype){
	unsigned char answer[NS_PACKETSZ];
	int len = res_query(name, C_IN, qtype, answer, sizeof(answer));
	if(len < 0) return 0;
	ns_msg msg;
	if(ns_initparse(answer, len, &msg) < 0) return 0;
	int ancount = ns_msg_count(msg, ns_s_an);
	for(int i = 0; i < ancount; ++i){
		ns_rr rr;
		if(ns_parserr(&msg, ns_s_an, i, &rr) == 0){
			if((int)ns_rr_type(rr) == qtype)
				return ns_rr_ttl(rr);
		}
	}
	return 0;
}

static void browse_callback(AvahiServiceBrowser *b,
							AvahiIfIndex interface,
							AvahiProtocol protocol,
							AvahiBrowserEvent event,
							const char *name,
							const char *type,
							const char *domain,
							AvahiLookupResultFlags flags,
							void *userdata){
	char *ts = timestamp_now();
	switch (event){
	case AVAHI_BROWSER_NEW:
		printf("%s BROWSE: NEW: name='%s' type='%s' domain='%s'
", ts, name, type, domain);
		break;
	case AVAHI_BROWSER_REMOVE:
		printf("%s BROWSE: REMOVE: name='%s' type='%s' domain='%s'
", ts, name, type, domain);
		break;
	case AVAHI_BROWSER_CACHE_EXHAUSTED:
		printf("%s BROWSE: CACHE_EXHAUSTED
", ts);
		break;
	case AVAHI_BROWSER_ALL_FOR_NOW:
		printf("%s BROWSE: ALL_FOR_NOW
", ts);
		break;
	default:
		printf("%s BROWSE: EVENT %d\n", ts, event);
	}
	free(ts);
}

static void resolve_callback(AvahiServiceResolver *r,
							 AvahiIfIndex interface,
							 AvahiProtocol protocol,
							 AvahiResolverEvent event,
							 const char *name,
							 const char *type,
							 const char *domain,
							 const char *host_name,
							 const AvahiAddress *address,
							 uint16_t port,
							 AvahiStringList *txt,
							 AvahiLookupResultFlags flags,
							 void *userdata){
	char *ts = timestamp_now();
	char flagstr[128];
	lookup_flags_to_string(flags, flagstr, sizeof(flagstr));

	if(event == AVAHI_RESOLVER_FOUND){
		char addrbuf[AVAHI_ADDRESS_STR_MAX];
		avahi_address_snprint(addrbuf, sizeof(addrbuf), address);
		printf("%s RESOLVER_FOUND: name='%s' type='%s' domain='%s' host='%s' address='%s' port=%u flags=0x%x (%s)\n",
			   ts, name, type, domain, host_name, addrbuf, port, flags, flagstr);

		print_txt(txt);

		/* Try to query TTLs via the system resolver: */
		unsigned int txt_ttl = query_ttl_via_resolv(name, T_TXT);
		unsigned int srv_ttl = query_ttl_via_resolv(name, T_SRV);
		unsigned int addr_ttl = 0;
		if(address->proto == AVAHI_PROTO_INET)
			addr_ttl = query_ttl_via_resolv(host_name, T_A);
		else if(address->proto == AVAHI_PROTO_INET6)
			addr_ttl = query_ttl_via_resolv(host_name, T_AAAA);

		if(txt_ttl)
			printf("	TXT TTL: %u\n", txt_ttl);
		else
			printf("	TXT TTL: (unknown)\n");

		if(srv_ttl)
			printf("	SRV TTL: %u\n", srv_ttl);
		else
			printf("	SRV TTL: (unknown)\n");

		if(addr_ttl)
			printf("	Address TTL: %u\n", addr_ttl);
		else
			printf("	Address TTL: (unknown)");
	} else
		printf("%s RESOLVER_FAILURE: name='%s' type='%s' domain='%s'
", ts, name ? name : "(null)", type ? type : "(null)", domain ? domain : "(null)");
	free(ts);
}

static void record_callback(AvahiEntryGroup *g,
							AvahiIfIndex interface,
							AvahiProtocol protocol,
							AvahiLookupResultFlags flags,
							AvahiRecordType type,
							const char *name,
							const void *rdata,
							size_t size,
							uint32_t ttl,
							void *userdata){
	char *ts = timestamp_now();
	printf("%s RECORD_FOUND: name='%s' type=%u (%s) ttl=%u size=%zu flags=0x%x
",
		   ts, name, type, dns_type_to_string(type), ttl, size, flags);
	/* For TXT, print string contents */
	if(type == AVAHI_RECORD_TXT){
		AvahiStringList *l = (AvahiStringList *)rdata;
		for (; l; l = l->next){
			char *s = avahi_string_list_to_string(l);
			if(s){
				printf("	TXT_RECORD: %s\n", s);
				avahi_free(s);
			}
		}
	} else if(type == AVAHI_RECORD_A){
		char addrbuf[AVAHI_ADDRESS_STR_MAX];
		AvahiAddress a;
		memcpy(&a.v4, rdata, sizeof(a.v4));
		a.in_family = AVAHI_ADDRESS_IPV4;
		avahi_address_snprint(addrbuf, sizeof(addrbuf), &a);
		printf("	A_RECORD: %s\n", addrbuf);
	} else if(type == AVAHI_RECORD_AAAA){
		char addrbuf[AVAHI_ADDRESS_STR_MAX];
		AvahiAddress a;
		memcpy(&a.v6, rdata, sizeof(a.v6));
		a.in_family = AVAHI_ADDRESS_IPV6;
		avahi_address_snprint(addrbuf, sizeof(addrbuf), &a);
		printf("	AAAA_RECORD: %s\n", addrbuf);
	} else if(type == AVAHI_RECORD_SRV){
		/* SRV: rdata contains priority, weight, port, and target */
		const uint8_t *p = rdata;
		uint16_t priority = (p[0] << 8) | p[1];
		uint16_t weight = (p[2] << 8) | p[3];
		uint16_t portn = (p[4] << 8) | p[5];
		const char *target = (const char *)(p + 6);
		printf("	SRV: priority=%u weight=%u port=%u target=%s\n", priority, weight, portn, target);
	}
	free(ts);
}

static void client_callback(AvahiClient *c, AvahiClientState state, void *userdata){
	switch (state){
	case AVAHI_CLIENT_S_RUNNING:
		puts("Avahi client running");
		break;
	case AVAHI_CLIENT_FAILURE:
		fprintf(stderr, "Avahi client failure: %s\n", avahi_strerror(avahi_client_errno(c)));
		break;
	default:
		break;
	}
}

int main(int argc, char *argv[]){
	AvahiClient *client = NULL;
	AvahiServiceBrowser *sb = NULL;
	int error;

	/* Initialize resolver state for libresolv */
	res_init();

	client = avahi_client_new(avahi_threaded_poll_get(NULL), 0, client_callback, NULL, &error);
	if(!client){
		fprintf(stderr, "Failed to create Avahi client: %s\n", avahi_strerror(error));
		return 1;
	}

	/* Browse for all services */
	sb = avahi_service_browser_new(client, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC,
								   "_services._dns-sd._udp", NULL, 0, browse_callback, NULL);
	if(!sb){
		fprintf(stderr, "Failed to create service browser: %s\n", avahi_strerror(avahi_client_errno(client)));
		avahi_client_free(client);
		return 1;
	}

	/* The main loop - Avahi threaded poll should be running in real example.
	 * For simplicity, we sleep here and let callbacks run if poll is configured.
	 */
	puts("Browsing for services...");
	while(1)
		sleep(5);

	/* Cleanup (never reached) */
	if(sb) avahi_service_browser_free(sb);
	if(client) avahi_client_free(client);

	return 0;
}

