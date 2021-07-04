#ifndef _SED_H_
# define _SED_H_

# define _POSIX_C_SOURCE 200809L
# include <unistd.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <ctype.h>
# include <stdbool.h>
# include <stdarg.h>
# include <regex.h>
# include <errno.h>

enum address_type
{
	ADDRESS_LINE,
	ADDRESS_LAST,
	ADDRESS_RE,
};

struct address
{
	enum address_type type;
	union
	{
		size_t line;
		regex_t preg;
	};
};

struct addresses
{
	size_t count;
	struct address addresses[2];
};

typedef void (*command_function)(void);

struct command
{
	char id;
	bool inverse;
	struct addresses addresses;
	union
	{
		char *text;
		char *filepath;
		struct command *children;
		struct
		{
			regex_t preg;
			char *dest;
		} substitute;
	};
};

typedef struct command *script_t;

// utils.c
char *strjoinf(char *origin, ...);
char *read_file(char *filepath);
void die(char *fmt, ...);

#endif
