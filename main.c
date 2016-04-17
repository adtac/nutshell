#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "constants.h"
#include "definitions.h"
#include "utils.h"

struct dictionary *definitions = NULL, *definitions_end = NULL;
char *cwd;

void throw_error(int code, int line_number, char *text) {
	switch(code) {
		case INVALID_STATEMENT:
			if(line_number == -1)
				fprintf(stderr, "Error: Statement invalid\n");
			else {
				fprintf(stderr, "Error in %s:%d: Statement invalid\n", CONFIG_FILE, line_number);
				fprintf(stderr, "Exiting\n");
			}
			break;

		case UNDEFINED_VARIABLE:
			if(line_number == -1)
				fprintf(stderr, "Error: Undefined variable %s\n", text);
			else {
				fprintf(stderr, "Error in %s:%d: Undefined variable %s\n", CONFIG_FILE, line_number, text);
				fprintf(stderr, "Exiting\n");
			}
			break;

		case CANNOT_FORK:
			if(line_number == -1)
				fprintf(stderr, "Error: cannot fork\n");
			else {
				fprintf(stderr, "Error in %s:%d: cannot fork\n", CONFIG_FILE, line_number);
				fprintf(stderr, "Exiting\n");
			}
			break;

		default:
			fprintf(stderr, "Encountered some error... Exiting\n");
	}
	exit(0);
}

/* 
 * Removes all whitespaces in the line 
 * Returns a char pointer to a cleaned-up line
 */
char *remove_whitespaces(char *line, int length, int line_number) {
	char *output = (char *) malloc(sizeof(char) * length + MAX_LINE * 2), *beginning = output;
	char sc;
	while(*line) {
		if(is_quote(*line)) {
			sc = *line;
			*output++ = *line;
			line++;
			if(DEBUG) printf("[%s]\n", line);
			while(*line != sc) {
				if(DEBUG) printf("[%d]", *line);
				if(*line == '\\')
					line++;
				*output++ = *line++;
				if(is_end(*line))
					throw_error(INVALID_STATEMENT, line_number, NULL);
			}
			*output++ = *line++;
		}
		else if(is_whitespace(*line)) {
			if(is_special(*(line + 1)) || is_special(*(line - 1))) {
				line++;
				continue;
			}
			*output++ = *line;
			line++;
			while(*line && is_whitespace(*line))
				line++;
			if(is_end(*line)) {
				*--output = 0; // since we don't want the last one whitespace
				return beginning;
			}
		}
		else
			*output++ = *line++;
	}
	*output = '\0';
	return beginning;
}

/*
 * Identifies the line type. Can be one of the following:
 *   LINE_EXEC:        an executable command
 *   SET_STATEMENT:    sets a variable
 *   EXPORT_STATEMENT: exports a variable to environment
 */
int identify_line(char *line, int line_number) {
	char *original = line;
	char key[MAX_LINE];
	int pos = 0;
	while(*line && !is_whitespace(*line) && !is_special(*line)) {
		if(is_quote(*line))
			throw_error(INVALID_STATEMENT, line_number, NULL);
		key[pos++] = *line++;
	}
	key[pos] = '\0';
	for(int i = 0; i < pos; i++) {
		if(key[i] >= 'A' && key[i] <= 'Z')
			key[i] += 'a' - 'A';
	}
	return is_keyword(key);
}

void parse_equation(char *line, int length, char **variable, char **value, int line_number) {
	int var_start = -1, var_end = -1;
	for(int i = 0; line[i]; i++) {
		if(line[i] == ' ')
			var_start = i + 1;
		else if(line[i] == '=') {
			var_end = i;
			break;
		}
	}
	if(var_end == -1)
		throw_error(INVALID_STATEMENT, line_number, NULL);
	*variable = (char *) malloc(var_end - var_start);
	strncpy(*variable, line + var_start, var_end - var_start);
	*(*variable + var_end - var_start) = '\0';
	*value = (char *) malloc(length - var_end);
	strncpy(*value, line + var_end, length - var_end + 1);
}

struct dictionary *set_vars_find(char *variable) {
	struct dictionary *result = definitions;
	while(result != NULL && strcmp(result -> variable, variable))
		result = result -> next;
	return result;
}

void add_vars_list(struct dictionary *result) {
	if(definitions_end == NULL) {
		definitions_end = result;
		definitions = result;
	}
	else
		definitions_end -> next = result;
}

char *replace_vars(char *line, int line_number) {
	char *temp = line;
	while(*temp++);
	char *output = (char *) malloc(temp - line + 1), *beginning = output;
	temp = line;
	int flag = 1;
	while(flag) {
		flag = 0;
		output = beginning;
		while(*line) {
			if(*line == '$') {
				flag = 1;
				line++;
				char value[MAX_WORD_LEN];
				int i = 0;
				while(*line && !is_whitespace(*line) && !is_special(*line)) {
					value[i++] = *line;
					line++;
				}
				value[i] = '\0';
				struct dictionary *result = set_vars_find(value);
				if(result == NULL)
					throw_error(UNDEFINED_VARIABLE, line_number, value);
				i = 0;
				while(result -> translation[i])
					*output++ = result -> translation[i++];
			}
			else if((*line == '.' && *(line + 1) != '.' && *(line - 1) != '.' && *(line - 1) == ' ') || *line == '~') {
				flag = 1;
				line++;
				char *home = getenv("HOME");
				while(*home)
					*output++ = *home++;
			}
			else
				*output++ = *line++;
		}
		*output = '\0';
		if(flag)
			strcpy(line, beginning);
	}
	return beginning;
}

void set_var(char *variable, char *value) {
	struct dictionary *result = set_vars_find(variable);
	if(result == NULL) {
		result = (struct dictionary *) malloc(sizeof(struct dictionary));
		add_vars_list(result);
	}
	strcpy(result -> variable, variable);
	strcpy(result -> translation, value + 1);
}

void export_var(char *line, int line_number) {
	while(*line && !is_whitespace(*line))
		line++;
	if(!is_whitespace(*line))
		throw_error(INVALID_STATEMENT, line_number, NULL);
	line++;
	putenv(line);
}

void run(char *line, int line_number) {
	char *beginning = line;
	int argc = 1, i = 0;
	char **argv;
	while(*line) {
		if(is_quote(*line)) {
			line++;
			while(!is_quote(*line)) {
				if(*line == '\\')
					line++;
				line++;
			}
		}
		if(*line == ' ')
			argc++;
		line++;
	}
	argv = (char **) malloc(argc * sizeof(char *));
	line = beginning;
	while(1) {
		if(DEBUG) printf("%c\n", *line);
		if(is_quote(*line)) {
			line++;
			while(!is_quote(*line)) {
				if(*line == '\\')
					line++;
				line++;
			}
			line++;
		}
		else if(*line == ' ' || !*line) {
			if(DEBUG) printf("{%s}{%s}\n", beginning, line);
			argv[i] = (char *) malloc(line - beginning + 1);
			strncpy(argv[i], beginning, line - beginning);
			argv[i][line - beginning] = '\0';
			if(DEBUG) printf("--%s--\n", argv[i]);
			i++;
			beginning = line + 1;
			if(!*line)
				break;
			line++;
		}
		else
			line++;
	}
	pid_t pid, wpid;
	int status;
	pid = fork();
	if(pid == 0) {
		if(execvp(argv[0], argv) == -1)
			fprintf(stderr, "%s: command not found\n", argv[0]);
	}
	else if(pid < 0)
		throw_error(UNDEFINED_VARIABLE, line_number, NULL);
	else {
		do {
			wpid = waitpid(pid, &status, WUNTRACED);
		} while(!WIFEXITED(status) && !WIFSIGNALED(status));
	}
}

void work_line(char *line, int line_number) {
	if(is_empty_line(line))
		return;
	int length = strlen(line);
	if(line[length - 1] == '\n')
		line[length - 1] = '\0';
	char *l = remove_whitespaces(line, length, line_number);
	if(DEBUG) printf("1:[%s]\n", l);
	int result = identify_line(l, line_number);
	char *l_str = replace_vars(l, line_number);
	if(result == LINE_EXEC) {
		if(DEBUG) printf("2:[%s]\n", l_str);
		run(l_str, line_number);
	}
	else if(result == CD_STATEMENT)
		chdir(l_str + 3);
	else if(result == SUDO_STATEMENT) {
		system(l_str);
	}
	else if(result == EXIT_STATEMENT) {
		kill(0, SIGTERM);
		exit(0);
	}
	else {
		char *variable, *value;
		parse_equation(l_str, strlen(l_str), &variable, &value, line_number);
		if(result == SET_STATEMENT)
			set_var(variable, value);
		else if(result == EXPORT_STATEMENT)
			export_var(l_str, line_number);
	}
	free(l);
}

void handler(int var) {
	printf("\033[1m\033[31m" "\n%c " "\x1b[0m", geteuid()?'$':'#');
	fflush(stdout);
}

void init() {
	signal(SIGINT, handler);
	strcpy(CONFIG_FILE, getenv("HOME"));
	strcat(CONFIG_FILE, "/");
	strcat(CONFIG_FILE, CONFIG_FILE_NAME);
}

/* 
 * Loads the configuration file from the home directory
 */
void load_config() {
	int line_number = 0;
	char line[MAX_LINE];
	FILE *config = fopen(CONFIG_FILE, "r");
	if(config == NULL) {
		printf("You don't have any configuration file; loading the default settings\n");
		printf("You may write one at %s\n", CONFIG_FILE);
		fclose(fopen(CONFIG_FILE, "w"));
		return;
	}
	while(fgets(line, MAX_LINE, config) != NULL) {
		line_number++;
		work_line(line, line_number);
	}
}

void enter_shell() {
	while(1) {
		printf("\033[1m\033[31m" "%c " "\x1b[0m", geteuid()?'$':'#');
		char line[MAX_LINE];
		fgets(line, MAX_LINE, stdin);
		work_line(line, -1);
	}
}

void display_message() {
	printf("Welcome to Nutshell 1.0\n\n");
}

int main(int argc, char *argv[]) {
	display_message();
	init();
	load_config();
	enter_shell();

	return 0;
}
