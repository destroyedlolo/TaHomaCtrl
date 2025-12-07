/* TaHomaCtl.h
 *
 *	Shared definition
 *
 */

#ifndef TAHOMACTL_H
#define TAHOMACTL_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <curl/curl.h>

#include <avahi-common/address.h>	/* for AvahiProtocol */

	/* Shared configuration */
extern char *tahoma;	/* Tahoma's hostname */
extern char *ip;		/* Tahoma's IP address */
extern uint16_t port;	/* TaHoma's port */
extern char *token;		/* Bearer Token */
extern bool unsafe;		/* Don't verify SSL chaine */

extern char *url;	/* base API url */
extern size_t url_len;	/* URL's length */

extern AvahiProtocol avahiIP;

extern bool debug;
extern bool trace;
extern bool verbose;

	/* API calling */
extern CURL *curl;
extern void curl_cleanup(void);
extern void buildURL(void);

	/* Utilities */
extern const char *FreeAndSet(char **storage, const char *val);

	/* Sharing functions */
extern void clean(char **);		// Clean a configuration reference
extern void func_scan(const char *);

#endif
