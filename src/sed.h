#ifndef _SED_H_
#define _SED_H_

#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <errno.h>
#include <regex.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
        size_t  line;
        regex_t preg;
    } data;
};

struct addresses
{
    size_t         count;
    struct address addresses[2];
};

#define COMMAND_LAST -1

union command_data
{
    char           *text;
    struct command *children;
    struct
    {
        regex_t preg;
        char   *replacement;
        size_t  occurence_index;
        bool    global;
        bool    print;
        char   *write_filepath;
    } substitute;
    struct
    {
        char *from;
        char *to;
    } translate;
};

struct command
{
    char               id;
    bool               inverse;
    struct addresses   addresses;
    union command_data data;
};

typedef struct command *script_t;

// utils.c
void *
xmalloc(size_t size);
void *
xrealloc(void *ptr, size_t size);
char *
strjoinf(char *origin, ...);
char *
read_file(char *filepath);
void
put_error(const char *format, ...);
void
die(const char *format, ...);
int
todigit(int c);

// parse.c
char *
parse_address(char *s, struct address *address);
char *
parse_addresses(char *s, struct addresses *addresses);
char *
parse_command(char *s, struct command *command);
struct command *
parse(char *s);

// exec.c
void
exec_command(struct command *command);

#endif
