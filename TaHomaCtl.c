/* TaHomaCtrl
 *
 * CLI tool to control your TaHoma
 *
 * History:
 * 	8/11/2025 - LF - First version
 */

#include "TaHomaCtl.h"

#include <unistd.h>	/* getopt() */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <readline/readline.h>
#include <readline/history.h>

#define VERSION "0.1"

	/* **
	 * Configuration
	 * **/
char *tahoma = NULL;
uint16_t port = 0;
char *token = NULL;

char *url = NULL;
size_t url_len;
bool debug = false;

	/* **
	 * Utilities
	 * **/

const char *FreeAndSet(char **storage, const char *val){
	if(*storage)
		free(*storage);

	*storage = strdup(val);
	assert(*storage);

	return(*storage);
}

	/* ***
	 * Commands interpreter
	 * ***/

void func_qmark(const char *);

void func_quit(const char *){
	exit(EXIT_SUCCESS);
}

struct _commands {
	const char *name;			// Command's name
	void(*func)(const char *);	// executor
	const char *help;			// Help message
} Commands[] = {
	{ "?", func_qmark, "List available commands" },
	{ "Quit", func_quit, "See you" },
	{ NULL }
};

void func_qmark(const char *){
	puts("List of known commands\n"
		 "----------------------");

	for(struct _commands *c = Commands; c->name; ++c)
		printf("'%s' : %s\n", c->name, c->help);
}

	/* ***
	 * Here we go
	 * ***/

int main(int ac, char **av){
	int opt;

	while( (opt = getopt(ac, av, ":+hH:p:t:d")) != -1){
		switch(opt){
		case 'H':
			FreeAndSet(&tahoma, optarg);
			break;
		case 'p':
			port = (uint16_t)atoi(optarg);	// Quick and dirty but harmless
			break;
		case 'd':
			debug = true;
			break;
		case '?':	/* Unknown option */
			fprintf(stderr, "unknown option: -%c\n", optopt);
		case 'h':
		case ':':	/* no argument provided (or missing argument) */
			puts(
				"TaHomaCrl v" VERSION "\n"
				"\tControl your TaHoma box from a command line.\n"
				"(c) L.Faillie (destroyedlolo) 2025\n"
				"\nKnown options :\n"
				"\t-H : set TaHoma's hostname\n"
				"\t-p : set TaHoma's port\n"
				"\t-t : set bearer token\n"
				"\t-d : add some debugging messages\n"
				"\t-h ; display this help"
			);
			exit(EXIT_FAILURE);
		}
	}

	if(isatty(STDIN_FILENO))
		puts("Interactive");
	else
		puts("Non Interactive");

func_qmark(NULL);
}
