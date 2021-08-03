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

typedef void (*command_function)(void);

struct command
{
    char             id;
    bool             inverse;
    struct addresses addresses;
    union
    {
        char *          text;
        char *          filepath;
        struct command *children;
        struct
        {
            regex_t preg;
            char *  dest;
        } substitute;
    } data;
};

typedef struct command *script_t;

// utils.c
char *
strjoinf(char *origin, ...);
char *
read_file(char *filepath);
void
die(char *fmt, ...);

// parse.c
char *
parse_address(char *s, struct address *address);

#endif
