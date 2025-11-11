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

	/* Shared configuration */
extern char *tahoma;	/* Tahoma's hostname */
extern uint16_t port;	/* TaHoma's port */
extern char *token;	/* Bearer Token */

extern char *url;	/* base API url */
extern size_t url_len;	/* URL's length */
extern bool debug;

	/* Utilities */
extern const char *FreeAndSet(char **storage, const char *val);

#endif
