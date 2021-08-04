#include "sed.h"

static inline char *
skip_blank(char **s_ptr)
{
    while (isblank(**s_ptr))
        (*s_ptr)++;
    return *s_ptr;
}

static const char *available_commands = "{}aci:btrwdDgGhHlnNpPqx=#sy";

#define ERRBUF_SIZE 128

char *
parse_address(char *s, struct address *address)
{
    if (strchr("!;\n", s[0]) != NULL || strchr(available_commands, s[0]) != NULL)
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
    const char  delim = s[0];
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
    const int errcode   = regcomp(&address->data.preg, s + 1, 0);
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

static char *
parse_singleton(char *s, struct command *command)
{
    (void)command;
    return s;
}

static char *
parse_text(char *s, struct command *command)
{
    command->data.text = s;
    while (*s != '\0' && *s != '\n')
        s++;
    *s = '\0';
    if ((command->id == 'r' || command->id == 'w') && *command->data.text == '\0')
        die("missing filename in r/w commands");
    return s + 1;
}

static const char *available_escape = "tnrvf";
static const char  escape_lookup[]  = {
    ['t'] = '\t',
    ['n'] = '\n',
    ['r'] = '\r',
    ['v'] = '\v',
    ['f'] = '\f',
};

static char *
parse_escapable_text(char *s, struct command *command)
{
    if (*s != '\\')
        die("expected '\\' after a/c/i commands");
    s++;
    skip_blank(&s);
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

static const size_t commands_realloc_size = 10;

static char *
parse_commands(char *s, struct command **commands, bool end_on_closing_brace)
{
    *commands    = xmalloc(sizeof(struct command) * commands_realloc_size);
    char   saved = '\0' + 1;
    size_t i     = 0;
    for (i = 0; true; i++)
    {
        skip_blank(&s);
        if (end_on_closing_brace && *s == '\0')
            die("unmatched '{'");
        s = parse_command(s, &(*commands)[i]);
        if ((*commands)[i].id == ';' || (*commands)[i].id == '\n')
		{
			i--;
			continue;
		}
        if (end_on_closing_brace && (*commands)[i].id == '}')
            break;
        saved = s[-1];  // weird hack for parse*_text where the parser puts a null
                        // character in place of the separator to split the string
        skip_blank(&s);
        if (end_on_closing_brace && *s == '\0')
            die("unmatched '{'");
        if (*s == '\0')
            break;
        if (*s == ';' || *s == '\n')
            s++;
        else if (saved == '\0' || *s == '}')
            ;
        else
            die("extra characters after command");
        if ((i + 1) % commands_realloc_size == 0)
        {
            size_t slots = (i / commands_realloc_size) + 2;
            *commands    = xrealloc(
                *commands, sizeof(struct command) * (slots * commands_realloc_size));
        }
    }
    return s;
}

static char *
parse_list(char *s, struct command *command)
{
    return parse_commands(s, &command->data.children, '}');
}

struct command_info
{
    char *(*func)(char *, struct command *);
    size_t addresses_max;
};

static const struct command_info command_info_lookup[] = {
    ['{'] = {parse_list, 2},
    ['}'] = {parse_singleton, 0},
    ['a'] = {parse_escapable_text, 2},
    ['c'] = {parse_escapable_text, 2},
    ['i'] = {parse_escapable_text, 2},
    [':'] = {parse_text, 0},
    ['b'] = {parse_text, 2},
    ['t'] = {parse_text, 2},
    ['r'] = {parse_text, 2},
    ['w'] = {parse_text, 2},
    ['d'] = {parse_singleton, 2},
    ['D'] = {parse_singleton, 2},
    ['g'] = {parse_singleton, 2},
    ['G'] = {parse_singleton, 2},
    ['h'] = {parse_singleton, 2},
    ['H'] = {parse_singleton, 2},
    ['l'] = {parse_singleton, 2},
    ['n'] = {parse_singleton, 2},
    ['N'] = {parse_singleton, 2},
    ['p'] = {parse_singleton, 2},
    ['P'] = {parse_singleton, 2},
    ['q'] = {parse_singleton, 1},
    ['x'] = {parse_singleton, 2},
    ['='] = {parse_singleton, 2},
    ['#'] = {parse_singleton, 0},
    ['s'] = {NULL, 2},
    ['y'] = {NULL, 2},
};

char *
parse_command(char *s, struct command *command)
{
    s = parse_addresses(s, &command->addresses);
    skip_blank(&s);
    command->inverse = (*s == '!');
    if (*s == '!')
        s++;
    skip_blank(&s);
    command->id = *s;
    if (command->id == ';' || command->id == '\n')
        return s + 1;
    if (strchr(available_commands, command->id) == NULL)
        die("unknown command: '%c'", command->id);
    const struct command_info *command_info = &command_info_lookup[(size_t)command->id];
    if (command->addresses.count > command_info->addresses_max)
        die("too much addresses: '%c' accepts maximum %zu address",
            command->id,
            command_info->addresses_max);
    s++;
    skip_blank(&s);
    return command_info->func(s, command);
}

struct command *
parse(char *s)
{
    struct command *commands = NULL;
    (void)parse_commands(s, &commands, '\0');
    /* *commands             = xrealloc(*commands, sizeof(struct command) * (i + 2)); */
    /* (*commands)[i + 1].id = COMMAND_LAST; */
    return commands;
}
