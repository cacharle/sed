#include "sed.h"

static char *
skip_blank(char **s_ptr)
{
    while (isblank(**s_ptr))
        (*s_ptr)++;
    return *s_ptr;
}

static char *
delimited_cut(char *s, char delim, const char *error_id)
{
    for (; *s != delim; s++)
    {
        if (*s == '\0')
            die("unterminated %s", error_id);
        if (s[0] == '\\' && s[1] == delim)
            memmove(s, s + 1, strlen(s));
    }
    *s = '\0';
    return s + 1;
}

static char *
extract_delimited(char *s, char **extracted1, char **extracted2, const char *error_id)
{
    const char delim = *s;
    if (delim == '\\')
        die("delimiter can not be a backslash");
    *extracted1 = s + 1;
    s = delimited_cut(*extracted1, delim, error_id);
    if (extracted2 == NULL)
        return s;
    *extracted2 = s;
    s = delimited_cut(*extracted2, delim, error_id);
    return s;
}

static char *
strchr_newline_or_end(const char *s)
{
    char *ret = NULL;
    ret = strchr(s, '\n');
    if (ret == NULL)
        ret = strchr(s, '\0');
    return ret;
}

static const char *available_escape = "tnrvf";
static const char  escape_lookup[] = {
    ['t'] = '\t',
    ['n'] = '\n',
    ['r'] = '\r',
    ['v'] = '\v',
    ['f'] = '\f',
};

static void
replace_escape_sequence(char *s)
{
    for (; *s != '\0'; s++)
    {
        if (s[0] == '\\' && s[1] != '\0')
        {
            if (strchr(available_escape, s[1]) != NULL)
                s[1] = escape_lookup[(size_t)s[1]];
            memmove(s, s + 1, strlen(s));
        }
    }
}

static const char *available_commands = "{}aci:btrwdDgGhHlnNpPqx=#sy";

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
        address->type = ADDRESS_LINE;
        address->data.line = strtoul(s, &s, 10);
        if (address->data.line == 0)
            die("invalid address: 0");
        return s;
    }
    char *regex = NULL;
    s = extract_delimited(s, &regex, NULL, "address regex");
    address->type = ADDRESS_RE;
    const int errcode = regcomp(&address->data.preg, regex, 0);
    if (errcode != 0)
    {
        const size_t errbuf_size = 128;
        char         errbuf[errbuf_size + 1];
        regerror(errcode, &address->data.preg, errbuf, errbuf_size);
        die("regex error '%s': %s", s, errbuf);
    }
    return s;
}

char *
parse_addresses(char *s, struct addresses *addresses)
{
    char *end;
    addresses->count = 0;
    end = parse_address(s, &addresses->addresses[0]);
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
    s = strchr_newline_or_end(s);
    *s = '\0';
    if ((command->id == 'r' || command->id == 'w') && *command->data.text == '\0')
        die("missing filename in r/w commands");
    return s + 1;
}

static char *
parse_escapable_text(char *s, struct command *command)
{
    if (*s != '\\')
        die("expected '\\' after a/c/i commands");
    s++;
    skip_blank(&s);
    command->data.text = s;
    s = strchr_newline_or_end(s);
    *s = '\0';
    replace_escape_sequence(command->data.text);
    return s + 1;
}

static const size_t commands_realloc_size = 10;

static char *
parse_commands(char *s, struct command **commands, bool end_on_closing_brace)
{
    *commands = xmalloc(sizeof(struct command) * commands_realloc_size);
    char   saved = '\0' + 1;
    size_t i = 0;
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
            *commands = xrealloc(
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

static char *
parse_translate(char *s, struct command *command)
{
    s = extract_delimited(
        s, &command->data.translate.from, &command->data.translate.to, "'y' command");
    replace_escape_sequence(command->data.translate.from);
    replace_escape_sequence(command->data.translate.to);
    if (strlen(command->data.translate.from) != strlen(command->data.translate.to))
        die("string for 'y' command are different lengths");
    return s;
}

struct command_info
{
    char *(*func)(char *, struct command *);
    size_t addresses_max;
};

static const struct command_info command_info_lookup[] = {
    ['{'] = {parse_list, 2},           ['}'] = {parse_singleton, 0},
    ['a'] = {parse_escapable_text, 2}, ['c'] = {parse_escapable_text, 2},
    ['i'] = {parse_escapable_text, 2}, [':'] = {parse_text, 0},
    ['b'] = {parse_text, 2},           ['t'] = {parse_text, 2},
    ['r'] = {parse_text, 2},           ['w'] = {parse_text, 2},
    ['d'] = {parse_singleton, 2},      ['D'] = {parse_singleton, 2},
    ['g'] = {parse_singleton, 2},      ['G'] = {parse_singleton, 2},
    ['h'] = {parse_singleton, 2},      ['H'] = {parse_singleton, 2},
    ['l'] = {parse_singleton, 2},      ['n'] = {parse_singleton, 2},
    ['N'] = {parse_singleton, 2},      ['p'] = {parse_singleton, 2},
    ['P'] = {parse_singleton, 2},      ['q'] = {parse_singleton, 1},
    ['x'] = {parse_singleton, 2},      ['='] = {parse_singleton, 2},
    ['#'] = {parse_singleton, 0},      ['s'] = {NULL, 2},
    ['y'] = {parse_translate, 2},
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
