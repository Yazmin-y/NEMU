#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint32_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
	static char *line_read = NULL;

	if (line_read) {
		free(line_read);
		line_read = NULL;
	}

	line_read = readline("(nemu) ");

	if (line_read && *line_read) {
		add_history(line_read);
	}

	return line_read;
}

static int cmd_c(char *args) {
	cpu_exec(-1);
	return 0;
}

static int cmd_q(char *args) {
	return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args) {
	char* arg = strtok(args, " ");
	int num;
	if (arg==NULL) {
		// printf("Need more arguments.\n");
		num = 1;
	} else {
		num = atoi(arg);
	}
	cpu_exec(num);
	// printf("Done.");
	return 0;
};

static int cmd_info(char *args) {
	char *arg = strtok(args, " ");
	// printf("%s\n", arg);
	if (strcmp(arg, "r") == 0)
	{
		int i;
		for ( i = R_EAX; i <= R_EDI; i++)
		{
			printf("%s\t0x%08x\n", regsl[i], reg_l(i));
		}
		
	}

	if (strcmp(arg, "w") == 0)
	{
		printf("Will print the watch point.\n");
	}
	
	
	return 0;
};

static int cmd_x(char *args) {
	if (args == NULL)
	{
		printf("Need more parameters.\n");
		return 1;
	}
	
	char *arg = strtok(args, " ");
	if (arg == NULL)
	{
		printf("Need more parameters.\n");
		return 1;
	}

	int n = atoi(arg);
	char *EXPR = strtok(NULL, " ");
	if (EXPR == NULL)
	{
		printf("Need more parameters.\n");
	}


	char *str;
	swaddr_t address = strtol(EXPR, &str, 16);

	// Scan
	int i;
	int j;
	for (i = 0; i < n; i++)
	{
		uint32_t data = swaddr_read(address+i*4, 4);
		printf("0x%08x: ", address + i * 4);

		for (j = 0; j < 4; j++)
		{
			printf("0x%02x ", data & 0xff);
			data = data >> 8; /*4 bytes each time*/
		}
		printf("\n");
	}
	
	return 0;
	
};

static struct {
	char *name;
	char *description;
	int (*handler) (char *);
} cmd_table [] = {
	{ "help", "Display informations about all supported commands", cmd_help },
	{ "c", "Continue the execution of the program", cmd_c },
	{ "q", "Exit NEMU", cmd_q },
	{ "si", "Step into implementation of N instructions after the execution with a default value of 1 when N is not given.", cmd_si},
	{ "info", "r: print the state of registers.\nw: print watch point position.", cmd_info},
	{ "x", "Caculate the result of the expression and print continuous N byte in hex started with the value.", cmd_x},

	/* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
	/* extract the first argument */
	char *arg = strtok(NULL, " ");
	int i;

	if(arg == NULL) {
		/* no argument given */
		for(i = 0; i < NR_CMD; i ++) {
			printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
		}
	}
	else {
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(arg, cmd_table[i].name) == 0) {
				printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
				return 0;
			}
		}
		printf("Unknown command '%s'\n", arg);
	}
	return 0;
}

void ui_mainloop() {
	while(1) {
		char *str = rl_gets();
		char *str_end = str + strlen(str);

		/* extract the first token as the command */
		char *cmd = strtok(str, " ");
		if(cmd == NULL) { continue; }

		/* treat the remaining string as the arguments,
		 * which may need further parsing
		 */
		char *args = cmd + strlen(cmd) + 1;
		if(args >= str_end) {
			args = NULL;
		}

#ifdef HAS_DEVICE
		extern void sdl_clear_event_queue(void);
		sdl_clear_event_queue();
#endif

		int i;
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(cmd, cmd_table[i].name) == 0) {
				if(cmd_table[i].handler(args) < 0) { return; }
				break;
			}
		}

		if(i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
	}
}
