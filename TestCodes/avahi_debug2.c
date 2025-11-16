/*
 * avahi_debug.c
 *
 * Simple Avahi (mDNS) browser + resolver for debugging.
 * Compatible with Avahi 0.8 (Arch Linux).
 *
 * Usage:
 * ./avahi_debug <service-type> <domain>
 * Example:
 * ./avahi_debug _http._tcp local
 * Compilation : 
 * gcc -std=c11 -Wall -O2 avahi_debug.c -o avahi_debug $(pkg-config --cflags --libs avahi-client)
 *
 *	From ChatGPT + Gemini code but sightly debugged and improved by myself
 *	(both AI generated craps !!!!)
 *
 *	GPLv3
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <avahi-common/defs.h>
#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/strlst.h>
#include <avahi-common/address.h>
#include <avahi-common/error.h>

/* Convert Avahi lookup flags to human-readable string (CORRECTED) */
static void lookup_flags_to_string(AvahiLookupResultFlags flags, char *out, size_t outlen){
	out[0] = '\0'; 
	size_t current_len = 0;
	int first = 1;

	if(flags & AVAHI_LOOKUP_RESULT_CACHED){ 
        if(!first) strncat(out + current_len, ",", outlen - current_len - 1);
        strncat(out + current_len, "CACHED", outlen - current_len - 1); 
        current_len = strlen(out);
        first = 0; 
    }

	if(flags & AVAHI_LOOKUP_RESULT_WIDE_AREA){ 
        if(!first) strncat(out + current_len, ",", outlen - current_len - 1);
        strncat(out + current_len, "WIDE_AREA", outlen - current_len - 1); 
        current_len = strlen(out);
        first = 0; 
    }

    if(flags & AVAHI_LOOKUP_RESULT_MULTICAST){ 
        if(!first) strncat(out + current_len, ",", outlen - current_len - 1);
        strncat(out + current_len, "MULTICAST", outlen - current_len - 1); 
        current_len = strlen(out);
        first = 0; 
    }

	if(flags & AVAHI_LOOKUP_RESULT_LOCAL){ 
        if(!first) strncat(out + current_len, ",", outlen - current_len - 1);
        strncat(out + current_len, "LOCAL", outlen - current_len - 1); 
        current_len = strlen(out);
        first = 0; 
    }

	if(flags & AVAHI_LOOKUP_RESULT_OUR_OWN){ 
        if(!first) strncat(out + current_len, ",", outlen - current_len - 1);
        strncat(out + current_len, "OUR_OWN", outlen - current_len - 1); 
        current_len = strlen(out);
        first = 0; 
    }

    if(flags & AVAHI_LOOKUP_RESULT_STATIC){ 
        if(!first) strncat(out + current_len, ",", outlen - current_len - 1);
        strncat(out + current_len, "STATIC", outlen - current_len - 1); 
        current_len = strlen(out);
        first = 0; 
    }

	if(first) {
        strncat(out, "NONE", outlen - 1);
    }
    
    // Si des drapeaux non reconnus restent, vous pouvez les ajouter ici,
    // mais dans la majorité des cas, cette implémentation suffit pour les flags mDNS.
}

/* Timestamp HH:MM:SS:ms */
static void timestamp_now(char *buf, size_t len) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm tm;
    localtime_r(&ts.tv_sec, &tm);
    snprintf(buf, len, "%02d:%02d:%02d:%03ld",
             tm.tm_hour, tm.tm_min, tm.tm_sec,
             ts.tv_nsec / 1000000);
}

/* TXT record to string */
static char* txt_to_string(AvahiStringList *txt) {
    if (!txt) return NULL;
    AvahiStringList *p = txt;
    char *s = NULL;
    size_t len = 0;
    while (p) {
        char *key = NULL;
        char *value = NULL;
        size_t value_len = 0;
        avahi_string_list_get_pair(p, &key, &value, &value_len);
        if (key) {
            size_t need = strlen(key) + (value ? value_len + 1 : 0) + 4;
            char *tmp = realloc(s, len + need);
            if (!tmp) { free(key); if(value) free(value); free(s); return NULL; }
            s = tmp;
            if(len==0) s[0]='\0'; else { s[len++]=';'; s[len]='\0'; }
            strcat(s,key); len=strlen(s);
            if(value && value_len>0) { s[len++]='='; memcpy(s+len,value,value_len); len+=value_len; s[len]='\0'; }
            free(key); if(value) free(value);
        }
        p = avahi_string_list_get_next(p);
    }
    return s;
}

/* Record Browser callback */
static void record_callback(
    AvahiRecordBrowser *b,
    AvahiIfIndex interface,
    AvahiProtocol protocol,
    AvahiBrowserEvent event,
    const char *name,
    uint16_t cls,
    uint16_t type,
    const void *rdata,
    size_t size,
    AvahiLookupResultFlags flags,
    void* userdata)
{
    (void)b; (void)interface; (void)protocol;
    (void)rdata; (void)size; (void)userdata;

    char ts[16];
    timestamp_now(ts,sizeof(ts));
    char flagstr[128];
    lookup_flags_to_string(flags, flagstr, sizeof(flagstr)); // Remplissage de la chaîne des flags

    switch(event) {
        case AVAHI_BROWSER_NEW:
            printf("%s RECORD_FOUND: name='%s' class=%u type=%u flags=0x%x (%s)\n", ts, name, cls, type, flags, flagstr);
            break;
        case AVAHI_BROWSER_REMOVE:
            printf("%s RECORD_REMOVED: name='%s' class=%u type=%u flags=0x%x (%s)\n", ts, name, cls, type, flags, flagstr);
            break;
        default: break;
    }
}

/* Resolver callback */
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
    uint16_t port,
    AvahiStringList *txt,
    AvahiLookupResultFlags flags,
    void* userdata)
{
    (void)userdata; (void)interface; (void)protocol;

    char ts[16];
    timestamp_now(ts,sizeof(ts));
    char flagstr[128];
    lookup_flags_to_string(flags, flagstr, sizeof(flagstr)); // Remplissage de la chaîne des flags

    AvahiClient *client = avahi_service_resolver_get_client(r);

    switch(event) {
        case AVAHI_RESOLVER_FOUND: {
            char addr[AVAHI_ADDRESS_STR_MAX];
            avahi_address_snprint(addr,sizeof(addr),address);
            char *txt_str = txt_to_string(txt);
            printf("%s RESOLVED: name='%s' type='%s' domain='%s' flags=0x%x (%s)\n",
                   ts,name,type,domain,flags,flagstr);
            printf("    host='%s' addr=%s port=%u\n", host_name, addr, port);
            if(txt_str){ printf("    txt='%s'\n",txt_str); free(txt_str); }

            /* Launch record browsers to debug records */
            avahi_record_browser_new(client, interface, protocol, name, AVAHI_DNS_CLASS_IN, AVAHI_DNS_TYPE_SRV, 0, record_callback, NULL);
            avahi_record_browser_new(client, interface, protocol, name, AVAHI_DNS_CLASS_IN, AVAHI_DNS_TYPE_TXT, 0, record_callback, NULL);
            avahi_record_browser_new(client, interface, protocol, host_name, AVAHI_DNS_CLASS_IN, AVAHI_DNS_TYPE_A, 0, record_callback, NULL);
            avahi_record_browser_new(client, interface, protocol, host_name, AVAHI_DNS_CLASS_IN, AVAHI_DNS_TYPE_AAAA, 0, record_callback, NULL);

            break;
        }
        case AVAHI_RESOLVER_FAILURE: {
            int err = avahi_client_errno(client);
            const char *errstr = avahi_strerror(err);
            printf("%s RESOLVE_FAILED: name='%s' type='%s' domain='%s' flags=0x%x (%s) -> %s\n",
                   ts,
                   name?name:"(null)",
                   type?type:"(null)",
                   domain?domain:"(null)",
                   flags, flagstr,
                   errstr?errstr:"(unknown error)");
            break;
        }
    }
    avahi_service_resolver_free(r);
}

/* Browser callback */
static void browse_callback(
    AvahiServiceBrowser *b,
    AvahiIfIndex interface,
    AvahiProtocol protocol,
    AvahiBrowserEvent event,
    const char *name,
    const char *type,
    const char *domain,
    AvahiLookupResultFlags flags,
    void *userdata)
{
    (void)b;
    char ts[16];
    timestamp_now(ts,sizeof(ts));
    AvahiClient *client=(AvahiClient*)userdata;
    char flagstr[128];
    lookup_flags_to_string(flags, flagstr, sizeof(flagstr)); // Remplissage de la chaîne des flags

    switch(event) {
        case AVAHI_BROWSER_NEW:
            printf("%s NEW: '%s' type='%s' domain='%s' flags=0x%x (%s)\n", ts,name,type,domain,flags,flagstr);
            avahi_service_resolver_new(client, interface, protocol, name, type, domain, AVAHI_PROTO_UNSPEC, 0, resolve_callback, NULL);
            break;
        case AVAHI_BROWSER_REMOVE:
            printf("%s REMOVE: '%s' type='%s' domain='%s' flags=0x%x (%s)\n", ts,name,type,domain,flags,flagstr);
            break;
        case AVAHI_BROWSER_CACHE_EXHAUSTED:
            printf("%s CACHE_EXHAUSTED\n", ts);
            break;
        case AVAHI_BROWSER_ALL_FOR_NOW:
            printf("%s ALL_FOR_NOW\n", ts);
            break;
        case AVAHI_BROWSER_FAILURE:
            printf("%s BROWSER_FAILURE\n", ts);
            break;
        default: break;
    }
}

/* Client callback */
static void client_callback(AvahiClient *c, AvahiClientState state, void *userdata) {
    (void)c; (void)userdata;
    char ts[16];
    timestamp_now(ts,sizeof(ts));

    switch(state) {
        case AVAHI_CLIENT_CONNECTING:
            printf("%s CLIENT: connecting\n", ts); break;
        case AVAHI_CLIENT_FAILURE:
            printf("%s CLIENT-ERROR\n", ts); break;
        case AVAHI_CLIENT_S_RUNNING:
            printf("%s CLIENT: running\n", ts); break;
        default: break;
    }
}

/* Main */
int main(int argc,char*argv[]) {
    if(argc<3){ fprintf(stderr,"Usage: %s <service_type> <domain>\n",argv[0]); return 1; }
    const char *stype=argv[1]; const char *domain=argv[2];

    AvahiSimplePoll *poll = avahi_simple_poll_new();
    if(!poll){ fprintf(stderr,"Failed to create simple poll\n"); return 1; }

    AvahiClient *client = avahi_client_new(avahi_simple_poll_get(poll), 0, client_callback, NULL, NULL);
    if(!client){ fprintf(stderr,"Failed to create Avahi client\n"); return 1; }

    AvahiServiceBrowser *browser = avahi_service_browser_new(client, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC,
                                                            stype, domain, 0, browse_callback, client);
    if(!browser){ fprintf(stderr,"Failed to create Avahi browser\n"); return 1; }

    avahi_simple_poll_loop(poll);
    return 0;
}
