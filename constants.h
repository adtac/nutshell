#define LINE_EXEC        1001
#define SET_STATEMENT    1002
#define EXPORT_STATEMENT 1003
#define CD_STATEMENT 1004
#define EXIT_STATEMENT 1005
#define SUDO_STATEMENT 1006

#define INVALID_STATEMENT  1004
#define UNDEFINED_VARIABLE 1005
#define CANNOT_FORK 1006

#define MAX_PATH 		256
#define MAX_LINE 		256
#define MAX_WORD_LEN 	16

#define DEBUG 0

char CONFIG_FILE_NAME[] = ".nutshell.cfg";
char CONFIG_FILE[MAX_PATH];

const char keyword_list[][MAX_WORD_LEN] = {
	"set",
	"export",
	"cd",
	"sudo",
	"exit"
};
int keyword_list_symbol[] = {
	SET_STATEMENT,
	EXPORT_STATEMENT,
	CD_STATEMENT,
	SUDO_STATEMENT,
	EXIT_STATEMENT
};
