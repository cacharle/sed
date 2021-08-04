#include "sed.h"

static char *
skip_blank(char **s_ptr)
{
    while (isblank(**s_ptr))
        (*s_ptr)++;
    return *s_ptr;
}

static char *
str_left_shift(char *s)
{
    return memmove(s, s + 1, strlen(s));
}

static char *
delimited_cut(char *s, char delim, const char *error_id)
{
    for (; *s != delim; s++)
    {
        if (*s == '\0')
            die("unterminated %s", error_id);
        if (s[0] == '\\' && s[1] == delim)
            str_left_shift(s);
    }
    *s = '\0';
    return s + 1;
}

static char *
extract_delimited(char *      s,
                  char **     extracted1,
                  char **     extracted2,
                  const char *error_id)
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
replace_escape_sequence_reject(char *s, const char *reject)
{
    for (; *s != '\0'; s++)
    {
        if (s[0] == '\\' && s[1] != '\0' && strchr(reject, s[1]) == NULL)
        {
            if (strchr(available_escape, s[1]) != NULL)
                s[1] = escape_lookup[(size_t)s[1]];
            str_left_shift(s);
        }
    }
}

static void
replace_escape_sequence(char *s)
{
    replace_escape_sequence_reject(s, "");
}

static void
xregcomp(regex_t *preg, const char *regex, int cflags)
{
    const int errcode = regcomp(preg, regex, cflags);
    if (errcode != 0)
    {
        const size_t errbuf_size = 128;
        char         errbuf[errbuf_size + 1];
        regerror(errcode, preg, errbuf, errbuf_size);
        die("regex error '%s': %s", regex, errbuf);
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
    xregcomp(&address->data.preg, regex, 0);
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
    bool has_separator = false;
    *commands = xmalloc(sizeof(struct command) * commands_realloc_size);
    size_t i = 0;
    for (i = 0; true; i++)
    {
        s = parse_command(s, &(*commands)[i]);
        if (s[-1] == '\0')
            has_separator = true;  // weird hack for parse*_text where the parser
                                   // replaces the separator with  a null character
        while (*s == ';' || *s == '\n' || isspace(*s))
        {
            if (*s == ';' || *s == '\n')
                has_separator = true;
            s++;
        }
        if (end_on_closing_brace)
        {
            if ((*commands)[i].id == '}')
                break;
            if (*s == '\0')
                die("unmatched '{'");
        }
        else
        {
            if ((*commands)[i].id == '}')
                die("unexpected '}'");
            if (*s == '\0')
                break;
        }
        if (*s != '{' && *s != '}' && !has_separator)
            die("extra characters after command");
        has_separator = false;
        if ((*commands)[i].id == COMMAND_LAST)
            i--;
        if ((i + 1) % commands_realloc_size == 0)
        {
            size_t slots = (i / commands_realloc_size) + 2;
            *commands = xrealloc(
                *commands, sizeof(struct command) * (slots * commands_realloc_size));
        }
    }
    if (!end_on_closing_brace)
    {
        *commands = xrealloc(*commands, sizeof(struct command) * (i + 2));
        (*commands)[i + 1].id = COMMAND_LAST;
    }
    return s;
}

static char *
parse_list(char *s, struct command *command)
{
    return parse_commands(s, &command->data.children, true);
}

static char *
parse_translate(char *s, struct command *command)
{
    s = extract_delimited(s,
                          &command->data.translate.from,
                          &command->data.translate.to,
                          "'y' command");
    replace_escape_sequence(command->data.translate.from);
    replace_escape_sequence(command->data.translate.to);
    if (strlen(command->data.translate.from) != strlen(command->data.translate.to))
        die("string for 'y' command are different lengths");
    return s;
}

static char *
parse_substitute(char *s, struct command *command)
{
    char *regex;
    s = extract_delimited(
        s, &regex, &command->data.substitute.replacement, "'s' command");
    xregcomp(&command->data.substitute.preg, regex, 0);
    replace_escape_sequence_reject(command->data.substitute.replacement,
                                   "&0123456789");
    command->data.substitute.occurence_index = 0;
    command->data.substitute.global = false;
    command->data.substitute.print = false;
    command->data.substitute.write_filepath = NULL;
    while (*s != '\0' && (strchr("gpw", *s) != NULL || isdigit(*s)))
    {
        if (*s == 'g')
        {
            if (command->data.substitute.global)
                die("multiple number 'g' options to 's' command");
            command->data.substitute.global = true;
            s++;
        }
        else if (*s == 'p')
        {
            if (command->data.substitute.global)
                die("multiple number 'p' options to 's' command");
            command->data.substitute.print = true;
            s++;
        }
        else if (*s == 'w')
        {
            struct command write_file_command;
            write_file_command.id = 'w';
            s++;
            skip_blank(&s);
            s = parse_text(s, &write_file_command);
            command->data.substitute.write_filepath = write_file_command.data.text;
            break;
        }
        else
        {
            if (command->data.substitute.occurence_index != 0)
                die("multiple number options to 's' command");
            errno = 0;
            command->data.substitute.occurence_index = strtoul(s, &s, 10);
            if (errno != 0)
                die("number option invalid: %s", strerror(errno));
            if (command->data.substitute.occurence_index == 0)
                die("number option in 's' command may not be zero");
        }
    }
    if (*s != '\0' && *s != '\n' && *s != ';' && *s != '}' && !isblank(*s))
        die("unknown option to 's' command");
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
    ['#'] = {parse_singleton, 0},      ['s'] = {parse_substitute, 2},
    ['y'] = {parse_translate, 2},
};

char *
parse_command(char *s, struct command *command)
{
    if (isblank(*s) || *s == ';' || *s == '\n' || *s == '\0')
    {
        command->id = COMMAND_LAST;
        return s;
    }
    s = parse_addresses(s, &command->addresses);
    skip_blank(&s);
    command->inverse = (*s == '!');
    if (*s == '!')
        s++;
    skip_blank(&s);
    command->id = *s;
    if (strchr(available_commands, command->id) == NULL)
        die("unknown command: '%c'", command->id);
    const struct command_info *command_info =
        &command_info_lookup[(size_t)command->id];
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
    (void)parse_commands(s, &commands, false);
    return commands;
}
