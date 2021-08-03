#include "sed.h"

static const char *available_commands = "{aci:btrwdDgGhHlnNpPqx=#sy";

#define ERRBUF_SIZE 128

char *
parse_address(char *s, struct address *address)
{
    if (s[0] == '!' || strchr(available_commands, s[0]) != NULL)
        return s;
    if (s[0] == '$')
    {
        address->type = ADDRESS_LAST;
        return s + 1;
    }
    if (isdigit(s[0]))
    {
        address->type      = ADDRESS_LINE;
        address->data.line = strtoul(s, &s, 10);
        if (address->data.line == 0)
            die("invalid address: 0");
        return s;
    }
    char  delim = s[0];
    char *end   = s + 1;
    while (*end != delim && *end != '\0')
    {
        if (end[0] == '\\' && end[1] == delim)
            memmove(end, end + 1, strlen(end));
        end++;
    }
    if (*end == '\0')
        die("unterminated address regex '%s'", s);
    *end          = '\0';
    address->type = ADDRESS_RE;
    int errcode   = regcomp(&address->data.preg, s + 1, 0);
    if (errcode != 0)
    {
        char errbuf[ERRBUF_SIZE + 1];
        regerror(errcode, &address->data.preg, errbuf, ERRBUF_SIZE);
        die("regex error '%s': %s", s, errbuf);
    }
    return end + 1;
}

char *
parse_addresses(char *s, struct addresses *addresses)
{
    char *end;
    addresses->count = 0;
    end              = parse_address(s, &addresses->addresses[0]);
    if (s == end)
        return s;
    s = end;
    addresses->count++;
    if (*s != ',')
        return s;
    s++;
    end = parse_address(s, &addresses->addresses[1]);
    if (s == end)
        return s;
    s = end;
    addresses->count++;
    return s;
}

static const char *available_escape = "tnrvf";
static const char  escape_lookup[]  = {
    ['t'] = '\t', ['n'] = '\n', ['r'] = '\r', ['v'] = '\v', ['f'] = '\f',
};

char *
parse_escapable_text(char *s, struct command *command)
{
    if (*s != '\\')
        die("expected '\\' after a/c/i commands");
    s++;
    while (isspace(*s))
        s++;
    command->data.text = s;
    while (*s != '\0' && *s != '\n')
    {
        if (s[0] == '\\' && s[1] != '\0')
        {
            if (strchr(available_escape, s[1]) != NULL)
                s[1] = escape_lookup[(size_t)s[1]];
            memmove(s, s + 1, strlen(s));
        }
        s++;
    }
    *s = '\0';
    return s + 1;
}

char *
parse_text(char *s, struct command *command)
{
    command->data.text = s;
    while (*s != '\0' && *s != '\n' && *s != ';')
        s++;
    *s = '\0';
    if ((command->id == 'r' || command->id == 'w') && *command->data.text == '\0')
        die("missing filename in r/w commands");
    return s + 1;
}

char *
parse_list(char *s, struct command *command)
{
    (void)command;
    return s;
}

char *
parse_singleton(char *s, struct command *command)
{
    (void)command;
    return s;
}

typedef char *(*t_command_parse_func)(char *, struct command *);

static t_command_parse_func command_parse_func_lookup[] = {
    ['{'] = parse_list,
    ['a'] = parse_escapable_text,
    ['c'] = parse_escapable_text,
    ['i'] = parse_escapable_text,
    [':'] = parse_text,
    ['b'] = parse_text,
    ['t'] = parse_text,
    ['r'] = parse_text,
    ['w'] = parse_text,
    ['d'] = parse_singleton,
    ['D'] = parse_singleton,
    ['g'] = parse_singleton,
    ['G'] = parse_singleton,
    ['h'] = parse_singleton,
    ['H'] = parse_singleton,
    ['l'] = parse_singleton,
    ['n'] = parse_singleton,
    ['N'] = parse_singleton,
    ['p'] = parse_singleton,
    ['P'] = parse_singleton,
    ['q'] = parse_singleton,
    ['x'] = parse_singleton,
    ['='] = parse_singleton,
    ['#'] = parse_singleton,
    ['s'] = NULL,
    ['y'] = NULL,
};

char *
parse_command(char *s, struct command *command)
{
    s                = parse_addresses(s, &command->addresses);
    command->inverse = (*s == '!');
    if (*s == '!')
        s++;
    command->id = *s;
    if (strchr(available_commands, command->id) == NULL)
        die("unknown command: '%c'", command->id);
    s++;
    while (isblank(*s))
        s++;
    return command_parse_func_lookup[(size_t)command->id](s, command);
}

struct command *
parse_script(char *s)
{
    (void)s;
    return NULL;
}
