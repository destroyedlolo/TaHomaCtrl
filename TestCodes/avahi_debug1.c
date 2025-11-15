/* Trace Avahi messages
 *
 * Compilation : 
gcc avahi_debug.c -o avahi_debug $(pkg-config --cflags --libs avahi-client glib-2.0) -lc
 */

/*
 * avahi_debug.c
 *
 * Simple Avahi (mDNS) browser + resolver for debugging.
 * Compatible with Avahi 0.8 (Arch Linux).
 *
 * Usage:
 *   ./avahi_debug <service-type> <domain>
 * Example:
 *   ./avahi_debug _http._tcp local
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-common/error.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/alternative.h>
#include <avahi-common/malloc.h>
#include <avahi-common/strlst.h>
#include <avahi-common/address.h>

static AvahiSimplePoll *simple_poll = NULL;
static AvahiClient *client = NULL;

/* Return current time formatted as HH:MM:SS:ms */
static void timestamp_now(char *buf, size_t buflen)
{
    struct timespec ts;
    struct tm tm;

    clock_gettime(CLOCK_REALTIME, &ts);
    localtime_r(&ts.tv_sec, &tm);

    int ms = (int)(ts.tv_nsec / 1000000);

    snprintf(buf, buflen, "%02d:%02d:%02d:%03d",
             tm.tm_hour, tm.tm_min, tm.tm_sec, ms);
}

/* Convert TXT record list to a readable string */
static char* txt_to_string(AvahiStringList *txt)
{
    if (!txt) return NULL;

    AvahiStringList *p = txt;
    char *s = NULL;
    size_t len = 0;

    while (p) {
        char *key = NULL;
        char *value = NULL;
        size_t value_len = 0;

        /* Avahi 0.8 API */
        avahi_string_list_get_pair(p, &key, &value, &value_len);

        if (key) {
            size_t need = strlen(key)
                        + (value ? value_len + 1 : 0)
                        + 4;

            char *tmp = realloc(s, len + need);
            if (!tmp) {
                avahi_free(key);
                if (value) avahi_free(value);
                free(s);
                return NULL;
            }

            s = tmp;

            if (len == 0)
                s[0] = '\0';
            else {
                s[len++] = ';';
                s[len] = '\0';
            }

            strcat(s, key);
            len = strlen(s);

            if (value && value_len > 0) {
                s[len++] = '=';
                memcpy(s + len, value, value_len);
                len += value_len;
                s[len] = '\0';
            }

            avahi_free(key);
            if (value) avahi_free(value);
        }

        p = avahi_string_list_get_next(p);
    }

    return s;
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
    (void)interface; (void)protocol; (void)flags; (void)userdata;

    char ts[16];
    timestamp_now(ts, sizeof(ts));

    if (event == AVAHI_RESOLVER_FOUND) {
        char addr_str[AVAHI_ADDRESS_STR_MAX];
        avahi_address_snprint(addr_str, sizeof(addr_str), address);

        char *txts = txt_to_string(txt);

        printf("%s RESOLVED: '%s' type='%s' domain='%s'\n",
               ts, name, type, domain);
        printf("    host='%s' addr=%s port=%u\n", host_name, addr_str, port);
        printf("    txt='%s'\n", txts ? txts : "(none)");

        free(txts);
    }
    else {
        printf("%s RESOLVE_FAILED: name='%s' type='%s'\n",
               ts, name, type);
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
    void* userdata)
{
    (void)b; (void)interface; (void)protocol; (void)flags; (void)userdata;

    char ts[16];
    timestamp_now(ts, sizeof(ts));

    switch (event) {

    case AVAHI_BROWSER_NEW:
        printf("%s NEW: '%s' type='%s' domain='%s'\n",
               ts, name, type, domain);

        if (!avahi_service_resolver_new(
                client, interface, protocol,
                name, type, domain,
                AVAHI_PROTO_UNSPEC, 0,
                resolve_callback, NULL)) {

            printf("%s ERROR: resolver creation failed: %s\n",
                   ts, avahi_strerror(avahi_client_errno(client)));
        }
        break;

    case AVAHI_BROWSER_REMOVE:
        printf("%s REMOVE: '%s' type='%s' domain='%s'\n",
               ts, name, type, domain);
        break;

    case AVAHI_BROWSER_CACHE_EXHAUSTED:
        printf("%s CACHE_EXHAUSTED\n", ts);
        break;

    case AVAHI_BROWSER_ALL_FOR_NOW:
        printf("%s ALL_FOR_NOW\n", ts);
        break;

    case AVAHI_BROWSER_FAILURE:
        fprintf(stderr, "%s BROWSER FAILURE: %s\n",
                ts, avahi_strerror(avahi_client_errno(client)));
        avahi_simple_poll_quit(simple_poll);
        break;

    default:
        printf("%s UNKNOWN_EVENT %d\n", ts, event);
        break;
    }
}

/* Client state callback */
static void client_callback(AvahiClient *c, AvahiClientState state, void *userdata)
{
    (void)userdata;
    client = c;

    char ts[16];
    timestamp_now(ts, sizeof(ts));

    switch (state) {
    case AVAHI_CLIENT_S_RUNNING:
        printf("%s Avahi client running.\n", ts);
        break;

    case AVAHI_CLIENT_S_REGISTERING:
    case AVAHI_CLIENT_S_COLLISION:
        printf("%s Avahi registering/collision.\n", ts);
        break;

    case AVAHI_CLIENT_CONNECTING:
        printf("%s Avahi connecting.\n", ts);
        break;

    case AVAHI_CLIENT_FAILURE:
        fprintf(stderr, "%s Avahi client failure: %s\n",
                ts, avahi_strerror(avahi_client_errno(client)));
        avahi_simple_poll_quit(simple_poll);
        break;

    default:
        printf("%s Avahi client state=%d\n", ts, state);
        break;
    }
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr,
            "Usage: %s <service-type> <domain>\n"
            "Example: %s _http._tcp local\n",
            argv[0], argv[0]);
        return 1;
    }

    const char *service_type = argv[1];
    const char *domain = argv[2];

    int error;
    AvahiServiceBrowser *sb = NULL;

    simple_poll = avahi_simple_poll_new();
    if (!simple_poll) {
        fprintf(stderr, "Failed to create simple poll\n");
        return 1;
    }

    client = avahi_client_new(
        avahi_simple_poll_get(simple_poll),
        0, client_callback, NULL, &error);

    if (!client) {
        fprintf(stderr, "Failed to create Avahi client: %s\n",
                avahi_strerror(error));
        return 1;
    }

    sb = avahi_service_browser_new(
        client, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC,
        service_type, domain, 0,
        browse_callback, NULL);

    if (!sb) {
        fprintf(stderr, "Failed to create service browser: %s\n",
                avahi_strerror(avahi_client_errno(client)));
        return 1;
    }

    avahi_simple_poll_loop(simple_poll);

    avahi_service_browser_free(sb);
    avahi_client_free(client);
    avahi_simple_poll_free(simple_poll);

    return 0;
}

