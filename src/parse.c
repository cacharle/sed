#include "sed.h"

static const char *available_commands = "ais!";

#define ERRBUF_SIZE 128

char *
parse_address(char *s, struct address *address)
{
    if (strchr(available_commands, s[0]) != NULL)
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

char *
parse_escapable_text(char *s, struct command *command)
{
    command->data.text = s;
    while (*s != '\0' && *s != '\n')
    {
        if (*s == '\\' && *s != '\0')
            s++;
        s++;
    }
    *s = '\0';
    return s + 1;
}

char *
parse_text(char *s, struct command *command)
{
    command->data.text = s;
    while (*s != '\0' && *s != '\n')
        s++;
    *s = '\0';
    return s + 1;
}

char *
parse_list(char *s, struct command *command)
{
    return s;
}

char *
parse_dummy(char *s, struct command *)
{
    return s;
}

typedef char *(*command_parse_func)(char *, struct command *);

static command_parse_func command_parse_func_lookup[] = {
    // TODO use condition
    ['{'] = parse_list,
    ['a'] = parse_escapable_text,
    ['c'] = parse_escapable_text,
    ['i'] = parse_escapable_text,
    ['b'] = parse_text,
    ['t'] = parse_text,
    ['r'] = parse_text,
    ['w'] = parse_text,
    ['d'] = parse_dummy,
    ['D'] = parse_dummy,
    ['g'] = parse_dummy,
    ['G'] = parse_dummy,
    ['h'] = parse_dummy,
    ['H'] = parse_dummy,
    ['l'] = parse_dummy,
    ['n'] = parse_dummy,
    ['N'] = parse_dummy,
    ['p'] = parse_dummy,
    ['P'] = parse_dummy,
    ['q'] = parse_dummy,
    ['x'] = parse_dummy,
    ['='] = parse_dummy,
    ['s'] = NULL,
    ['y'] = NULL,
};

char *
parse_command(char *s, struct command *command)
{
    s                = parse_addresses(s, &command->addresses);
    command->inverse = *s == '!';
    if (*s == '!')
        s++;
    command->id = *s;
    s++;
    while (isblank(*s))
        s++;
    return command_parse_func_lookup[(size_t)command->id](s, command);
}

struct command *
parse_script(char *)
{
    return NULL;
}
