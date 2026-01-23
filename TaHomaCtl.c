/* TaHomaCtl
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
#include <time.h>
#include <assert.h>
#include <pwd.h>
#include <readline/readline.h>
#include <readline/history.h>

#define VERSION "0.5"

	/* **
	 * Configuration
	 * **/

char *tahoma = NULL;
char *ip = NULL;
uint16_t port = 0;
char *token = NULL;
bool unsafe = false;

char *url = NULL;
size_t url_len;
long timeout = 0;
bool verbose = false;
bool trace = false;
bool debug = false;

static const char *ascript = NULL;	/* User script to launch (from launch parameters) */
static bool nostartup = false;	/* Do not source .tahomactl */

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

void clean(char **obj){
	if(*obj){
		free(*obj);
		*obj = NULL;
	}
}

char *nextArg(char *arg){
	char *p = strchr(arg, ' ');
	if(!p)
		return NULL;

	*(p++) = 0;	/* end current argument */
	if(*p)
		return p;
	else
		return NULL;
}

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

static const char *affval(const char *v){
	if(v)
		return v;
	else
		return "Not set";
}

	/* ***
	 * Commands interpreter
	 * ***/

static void execscript(const char *, bool);
static void func_qmark(char *);

static void func_script(char *arg){
	if(arg)
		execscript(arg, false);
	else
		fputs("*E* Expecting a filename.\n", stderr);
}

static void func_token(char *arg){
	if(arg){
		FreeAndSet(&token, arg);
		buildURL();
	} else
		printf("*I* Token : %s\n", affval(token));
}

static void func_THost(char *arg){
	if(arg){
		FreeAndSet(&tahoma, arg);
		buildURL();
	} else
		printf("*I* Tahoma's host : %s\n", affval(tahoma));
}

static void func_TAddr(char *arg){
	if(arg){
		FreeAndSet(&ip, arg);
		buildURL();
	} else
		printf("*I* Tahoma's IP address : %s\n", affval(ip));
}

static void func_TPort(char *arg){
	if(arg){
		port = (uint16_t)atoi(arg);
		buildURL();
	} else
		printf("*I* Tahoma's port : %u\n", port);
}

static void func_save(char *arg){
	if(!arg){
		fputs("*E* file name expected\n", stderr);
		return;
	}

	FILE *f = fopen(arg, "w");
	if(!f){
		perror(arg);
		return;
	}

	if(tahoma)
		fprintf(f, "TaHoma_host %s\n", tahoma);

	if(ip)
		fprintf(f, "TaHoma_address %s\n", ip);

	if(port)
		fprintf(f, "TaHoma_port %u\n", port);

	if(token)
		fprintf(f, "token %s\n", token);

	fclose(f);
}

static void func_status(char *){
	printf("*I* Connection :\n"
		"\tTahoma's host : %s\n"
		"\tTahoma's IP : %s\n"
		"\tTahoma's port : %u\n"
		"\tToken : %s\n"
		"\tSSL chaine : %s\n",
		affval(tahoma),
		affval(ip),
		port,
		token ? "set": "unset",
		unsafe ? "not checked (unsafe)" : "Enforced"
	);
	if(timeout)
		printf("\tTimeout : %lds\n", timeout);

	puts("*I* Stored devices :");
	for(struct Device *dev = devices_list; dev; dev = dev->next){
		printf("%s : %s\n", dev->label, dev->url);
		puts("\tCommands");
		for(struct Command *cmd = dev->commands; cmd; cmd = cmd->next)
			printf("\t\t%s (%d %s)\n", cmd->command, cmd->nparams, cmd->nparams > 1 ? "args": "arg");
		puts("\tStates");
		for(struct State *state = dev->states; state; state = state->next)
			printf("\t\t%s\n", state->state);
	}
}

static void func_history(char *arg){
	if(arg)
		fputs("*E* Argument is ignored\n", stderr);

	HISTORY_STATE *my_history_state = history_get_history_state();

	if(!my_history_state || !my_history_state->entries){
		puts("*I* The history is empty");
		return;
	}

	for(int i = 0; i < my_history_state->length; ++i)
		printf("\t%s\n", my_history_state->entries[i]->line);

	history_set_history_state(my_history_state);
}

static void func_verbose(char *arg){
	if(arg){
		if(!strcmp(arg, "on"))
			verbose = true;
		else if(!strcmp(arg, "off"))
			verbose = false;
		else
			fputs("*E* verbose accepts only 'on' and 'off'\n", stderr);
	} else
		puts(verbose ? "I'm verbose" : "I'm quiet");
}

static void func_trace(char *arg){
	if(arg){
		if(!strcmp(arg, "on"))
			trace = true;
		else if(!strcmp(arg, "off"))
			trace = false;
		else
			fputs("*E* trace accepts only 'on' and 'off'\n", stderr);
	} else
		puts(trace ? "Traces enabled" : "Traces disabled");
}

static void func_timeout(char *arg){
	if(arg){
		timeout = atol(arg);

		if(debug || verbose)
			printf("*I* Timeout : %lds\n", timeout);
	} else
		fputs("timeout is execting the number of seconds to wait.\n", stderr);
}

static void func_quit(char *){
	exit(EXIT_SUCCESS);
}

struct _commands {
	const char *name;			// Command's name
	void(*func)(char *);	// executor
	const char *help;			// Help message
} Commands[] = {
	{ NULL, NULL, "TaHoma's Configuration"},
	{ "TaHoma_host", func_THost, "[name] set or display TaHoma's host" },
	{ "TaHoma_address", func_TAddr, "[ip] set or display TaHoma's ip address" },
	{ "TaHoma_port", func_TPort, "[num] set or display TaHoma's port number" },
	{ "TaHoma_token", func_token, "[value] indicate application token" },
	{ "timeout", func_timeout, "[value] specify API call timeout (seconds)" },
	{ "scan", func_scan, "Look for Tahoma's ZeroConf advertising" },
	{ "status", func_status, "Display current connection informations" },

	{ NULL, NULL, "Scripting"},
	{ "save_config", func_save, "<file> save current configuration to the given file" },
	{ "script", func_script, "<file> execute the file" },

	{ NULL, NULL, "Verbosity"},
	{ "verbose", func_verbose, "[on|off|] Be verbose" },
	{ "trace", func_trace, "[on|off|] Trace every commands" },

	{ NULL, NULL, "Interacting"},
	{ "Gateway", func_Tgw, "Query your gateway own configuration" },
	{ "Devices", func_Devs, "Query and store attached devices" },
	{ "States", func_States, "<device name> [State's name] query the states of a device" },

	{ NULL, NULL, "Miscs"},
	{ "#", NULL, "Comment, ignored line" },
	{ "?", func_qmark, "List available commands" },
	{ "history", func_history, "List command line history" },
	{ "Quit", func_quit, "See you" },
	{ NULL, NULL, NULL }
};

static void func_qmark(char *){
	puts("List of known commands\n"
		 "======================");

	for(struct _commands *c = Commands; c->help; ++c){
		if(c->name)
			printf("'%s' : %s\n", c->name, c->help);
		else {
			printf("\n%s\n", c->help);
			for(const char *p = c->help; *p; ++p)
				putchar('-');
			putchar('\n');
		}
	}
}

static void exec(const char *cmd, char *arg){
	if(trace && *cmd != '#')
		printf("> %s\n", cmd);

	for(struct _commands *c = Commands; c->help; ++c){
		if(c->name && !strcmp(cmd, c->name)){
			if(c->func)
				c->func(arg);
			return;
		}
	}

	printf("*E* Unknown command \"%s\" : type '?' for list of known directives\n", cmd);
}

static void execline(char *l){
	char *arg;

	if((arg = strpbrk(l, " \t"))){
		*(arg++) = 0;
		while( *arg && !isgraph(*arg))	// skip non printable
			++arg;

		exec(l, *arg ? arg:NULL );
	} else	// No argument
		exec(l, NULL);
}

void execscript(const char *name, bool dontfail){
	FILE *f = fopen(name, "r");
	if(!f){
		if(dontfail)
			return;
		else {
			perror(name);
			exit(EXIT_FAILURE);
		}
	}

	char *l = NULL;
	size_t len = 0;
	while(getline(&l, &len, f) != -1){
		char *c = strchr(l, '\n');	// Remove leading CR
		if(c)
			*c = 0;

		if(*l)	// Ignore empty line
			execline(l);
	}

	free(l);
	fclose(f);
}

char *command_generator(const char *text, int state){
    static int list_index, len;
    const char *name;

    if(!state){
        list_index = 0;
        len = strlen(text);
    }

    // Iterate through the command_table for names
	while(Commands[list_index].help){
        ++list_index;
    	if((name = Commands[list_index].name)){
	        if(!strncmp(name, text, len))
    	        return(strdup(name));
	    }
	}

    return ((char *)NULL);
}

char *dev_generator(const char *text, int state){
	static struct Device *dev;
	static int len;

	if(!state){
		dev = devices_list;
		len = strlen(text);
	}

	while(dev){
		struct Device *cur = dev;
		dev = dev->next;

		if(!strncmp(cur->label, text, len))
			return(strdup(cur->label));
	}

    return ((char *)NULL);
}

char **command_completion(const char *text, int start, int end){
	rl_attempted_completion_over = 1;
	if(!start)
        return rl_completion_matches(text, command_generator);

	if(!strncmp(rl_line_buffer, "States", 6))
		return rl_completion_matches(text, dev_generator);

    return ((char **)NULL);
}

	/* ***
	 * Here we go
	 * ***/

int main(int ac, char **av){
	int opt;

	while( (opt = getopt(ac, av, ":+NhH:p:Uk:f:dvt46")) != -1){
		switch(opt){
		case 'f':
			ascript = optarg;
			break;
		case 'N':
			nostartup = true;
			break;
		case '4':
			avahiIP = AVAHI_PROTO_INET;
			break;
		case '6':
			avahiIP = AVAHI_PROTO_INET6;
			break;
		case 'H':
			FreeAndSet(&tahoma, optarg);
			break;
		case 'p':
			port = (uint16_t)atoi(optarg);	// Quick and dirty but harmless
			break;
		case 'U':
			unsafe = true;
			break;
		case 'd':
			debug = true;
			break;
		case 't':
			trace = true;
			break;
		case 'v':
			verbose = true;
			break;
		case '?':	/* Unknown option */
			fprintf(stderr, "unknown option: -%c\n", optopt);
		case 'h':
		case ':':	/* no argument provided (or missing argument) */
			puts(
				"TaHomaCrl v" VERSION "\n"
				"\tControl your TaHoma box from a command line.\n"
				"(c) L.Faillie (destroyedlolo) 2025-26\n"
				"\nScripting :\n"
				"\t-f : source provided script\n"
				"\t-N : don't execute ~/.tahomactl at startup\n"
				"\nTaHoma's :\n"
				"\t-H : set TaHoma's hostname\n"
				"\t-p : set TaHoma's port\n"
				"\t-k : set bearer token\n"
				"\t-U : don't verify SSL chaine (unsafe mode)\n"
				"\nLimiting scanning :\n"
				"\t-4 : resolve Avahi advertisement in IPv4 only\n"
				"\t-6 : resolve Avahi advertisement in IPv6 only\n"
				"\nMisc :\n"
				"\t-v : add verbosity\n"
				"\t-t : add tracing\n"
				"\t-d : add some debugging messages\n"
				"\t-h ; display this help"
			);
			exit(EXIT_FAILURE);
		}
	}

		/* libCURL's */
	curl_global_init(CURL_GLOBAL_DEFAULT);
	atexit(curl_cleanup);
	if(!(curl = curl_easy_init())){
		fputs("*F* curl_easy_init() failed.\n", stderr);
		exit(EXIT_FAILURE);
	}

	if(unsafe){
		if(debug || verbose)
			puts("*W* SSL chaine not enforced (unsafe mode)");

		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);	/* Don't verify SSL */
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	}

	if(!nostartup){
		struct passwd *pw = getpwuid(getuid());	/* Find user's info */
		if(!pw)
			fputs("*E* Can't read user's info\n", stderr);
		else {
			char t[strlen(pw->pw_dir) + 12];	/* "/.tahomactl" */
			sprintf(t, "%s/.tahomactl", pw->pw_dir);
			execscript(t, true);
		}
	}
	
		/* Command line handling */
	rl_attempted_completion_function = command_completion;
	for(;;){
		char *l = readline(isatty(STDIN_FILENO) ? "TaHomaCtl > ":NULL);
		
		if(!l)			// End requested
			break;

		char *line;
		for(line = l; *line && !isgraph(*line); ++line);	// Strip spaces

		if(*line){	// Ignore empty line
			if(isatty(fileno(stdin)))
				add_history(line);

			execline(line);
		}

		free(l);
	}
}
