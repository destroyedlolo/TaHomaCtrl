/* Utility functions
 *
 * 28/01/2026 - LF - Emencipate from other source files
 */

#include "TaHomaCtl.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <ctype.h>

	/* Sometime we can modify the orignal string to parce token,
	 * sometime we can't (as example during line edition).
	 * It's why 2 versions are provided.
	 */
bool extractTokenSub(struct substring *res, const char *l, const char**arg){
	/* Extract the first token of a line
	 * <- bool : is an argument ?
	 */
	res->s = l;
	if((*arg = strpbrk(l, " \t"))){
		res->len = *arg - l;

		++(*arg);
		while( **arg && !isgraph(**arg))	/* skip non printable */
			++(*arg);
		return true;
	} else {
		res->len = strlen(l);
		return false;
	}
}

int substringcmp(struct substring *s, const char *with){
	/* in the current version, the returned value says :
	 *	<0 is s<with
	 *	==0 if strings are the same
	 *	>0 if s>with
	 * but the value itself can't be used for anything more
	 */
	size_t l = strlen(with);
	if(l != s->len)
		return(l - s->len);

	return strncmp(s->s, with, s->len);
}

	/* Storage management */
const char *FreeAndSet(char **storage, const char *val){
	if(*storage)
		free(*storage);

	*storage = strdup(val);
	assert(*storage);

	return(*storage);
}

void clean(char **obj){
	if(*obj){
		free(*obj);
		*obj = NULL;
	}
}

	/* Time spent */
static unsigned long timespec_to_ms(const struct timespec *ts){
	return((unsigned long)ts->tv_sec * 1000) + (ts->tv_nsec / 1000000L);
}

void spent(bool ending){
		/* Measure time spent b/w starting (false) and ending (true) */
	static struct timespec beg, end;

	if(clock_gettime(CLOCK_MONOTONIC, ending ? &end : &beg) == -1){
		perror("clock_gettime')");
		return;
	}

	if((debug) && ending){
		unsigned long d = timespec_to_ms(&end) - timespec_to_ms(&beg);
		printf("*D* Time spent : %0.3f\n", (double)d / 1000.0);
	}
}


