#include "sed.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define CHAR_SPACE_MAX 20480
// need under buffer for implementation of the 'x' command (can't swap array aka
// constant pointers)
static char   pattern_space_under[CHAR_SPACE_MAX + 1] = {'\0'};
static char   hold_space_under[CHAR_SPACE_MAX + 1] = {'\0'};
static char  *pattern_space = pattern_space_under;
static char  *hold_space = hold_space_under;
static size_t line_index = 0;
static bool   auto_print = false;

static char *
next_cycle(void);

void
exec_insert(union command_data *data)
{
    fputs(data->text, stdout);
}

void
exec_read_file(union command_data *data)
{
    FILE *file = fopen(data->text, "r");
    if (file == NULL)
        return;
    char c;
    while ((c = fgetc(file)) != EOF)
        fputc(c, stdout);
}

void
exec_delete()
{
    pattern_space[0] = '\0';
}

void
exec_delete_newline()
{
    char *newline = strchr(pattern_space, '\n');
    if (newline == NULL)
    {
        exec_delete();
        return;
    }
    memmove(pattern_space, newline + 1, strlen(newline + 1) + 1);
    if (pattern_space[0] == '\0')
        ;  // load new line
}

void
exec_replace_pattern_by_hold()
{
    strcpy(pattern_space, hold_space);
}

void
exec_append_pattern_by_hold()
{
    char *pattern_space_end = pattern_space + strlen(pattern_space);
    strcat(pattern_space_end, "\n");
    strcat(pattern_space_end, hold_space);
}

void
exec_replace_hold_by_pattern()
{
    strcpy(hold_space, pattern_space);
}

void
exec_append_hold_by_pattern()
{
    char *hold_space_end = hold_space + strlen(hold_space);
    strcat(hold_space_end, "\n");
    strcat(hold_space_end, pattern_space);
}

void
exec_print(union command_data *data)
{
    (void)data;
    fputs(pattern_space, stdout);
}

void
exec_print_until_newline()
{
    size_t newline_index = strcspn(pattern_space, "\n");
    fwrite(pattern_space, sizeof(*pattern_space), newline_index, stdout);
}

void
exec_exchange()
{
    char *tmp = hold_space;
    hold_space = pattern_space;
    pattern_space = tmp;
}

void
exec_translate(union command_data *data)
{
    char *from = data->translate.from;
    for (size_t i = 0; pattern_space[i] != '\0'; i++)
    {
        char *from_found = strchr(from, pattern_space[i]);
        if (from_found == NULL)
            continue;
        pattern_space[i] = data->translate.to[from_found - from];
    }
}

#define SUBSTITUTE_NMATCH 10

void
exec_substitute(union command_data *data)
{
    assert(data->substitute.preg.re_nsub <= SUBSTITUTE_NMATCH - 1);
    if (data->substitute.occurence_index == 0)
        data->substitute.occurence_index = 1;
    char      *space = pattern_space;
    regmatch_t pmatch[SUBSTITUTE_NMATCH + 1];
    bool       found = false;
    for (size_t occurence = 1;
         *space != '\0' &&
         regexec(&data->substitute.preg, space, SUBSTITUTE_NMATCH, pmatch, 0) == 0;
         occurence++)
    {
        found = true;
        if (occurence < data->substitute.occurence_index)
        {
            space += pmatch[0].rm_eo;
            continue;
        }
        char *replacement = xstrdup(data->substitute.replacement);
        for (char *r = replacement; *r != '\0'; r++)
        {
            size_t group = -1;
            if (*r == '&')
                group = 0;
            else if (*r == '\\')
            {
                memmove(r, r + 1, strlen(r + 1) + 1);
                if (isdigit(*r))
                    group = todigit(*r);
            }
            if (group == -1)
                continue;
            memmove(r, r + 1, strlen(r + 1) + 1);
            if (pmatch[group].rm_so == -1 || pmatch[group].rm_eo == -1)
            {
                r--;
                continue;
            }
            size_t group_len = pmatch[group].rm_eo - pmatch[group].rm_so;
            size_t old_offset = r - replacement;
            replacement = xrealloc(replacement, strlen(replacement) + group_len + 1);
            r = replacement + old_offset;
            memmove(r + group_len, r, strlen(r) + 1);
            memcpy(r, space + pmatch[group].rm_so, group_len);
            r += group_len - 1;
        }
        char  *dest = space + pmatch[0].rm_so;
        char  *src = space + pmatch[0].rm_eo;
        size_t replacement_len = strlen(replacement);
        memmove(dest + replacement_len, src, strlen(src) + 1);
        memcpy(dest, replacement, replacement_len);
        free(replacement);
        if (!data->substitute.global &&
            occurence == data->substitute.occurence_index)
            break;
        space += replacement_len;
    }
    if (data->substitute.print && found)
        fputs(pattern_space, stdout);
    if (data->substitute.write_filepath != NULL && found)
    {
        // This is pretty inefficient since we reopen the file on every substitute
        FILE *file = fopen(data->substitute.write_filepath, "a");
        if (file == NULL)
            die("couldn't open file %s: %s",
                data->substitute.write_filepath,
                strerror(errno));
        fputs(pattern_space, file);
        fclose(file);
    }
}

static const char *reverse_available_escape = "\\\a\b\t\r\v\f";
static const char  reverse_escape_lookup[] = {
     ['\\'] = '\\',
     ['\a'] = 'a',
     ['\b'] = 'b',
     ['\t'] = 't',
     ['\r'] = 'r',
     ['\v'] = 'v',
     ['\f'] = 'f',
};

static const size_t print_escape_line_wrap = 60;

void
exec_print_escape(union command_data *data)
{
    size_t len = 1;
    for (char *space = pattern_space; *space != '\0'; space++, len++)
    {
        if (strchr(reverse_available_escape, *space) != NULL)
        {
            fputc('\\', stdout);
            fputc(reverse_escape_lookup[(size_t)*space], stdout);
            continue;
        }
        else if (*space == '\n')
        {
            fputc('$', stdout);
            fputc('\n', stdout);
        }
        else if (isprint(*space))
            fputc(*space, stdout);
        else
            printf("\\%03o", *space);
        if (len == print_escape_line_wrap)
        {
            fputc('\\', stdout);
            fputc('\n', stdout);
        }
    }
}

void
exec_next(union command_data *data)
{
    (void)data;
    if (auto_print)
        fputs(pattern_space, stdout);
    char *line = next_cycle();
    if (line == NULL)
        exit(EXIT_SUCCESS);
    strcpy(pattern_space, line);
}

void
exec_next_append(union command_data *data)
{
    (void)data;
    char *line = next_cycle();
    if (line == NULL)
        exit(EXIT_SUCCESS);
    strcat(pattern_space, "\n");
    strcat(pattern_space, line);
}

void
exec_quit(union command_data *data)
{
    (void)data;
    exit(EXIT_SUCCESS);
}

void
exec_print_line_number(union command_data *data)
{
    (void)data;
    printf("%zu\n", line_index);
}

void
exec_comment(union command_data *data)
{
    (void)data;
}

void
exec_write(union command_data *data)
{
    FILE *file = fopen(data->text, "a");
    if (file == NULL)
        die("couldn't open file %s: %s",
            data->text,
            strerror(errno));
    fputs(pattern_space, file);
    fclose(file);
}

typedef void (*exec_func)(union command_data *data);

static const exec_func exec_func_lookup[] = {
    ['{'] = NULL,
    ['}'] = NULL,
    ['a'] = NULL,
    ['c'] = NULL,
    ['i'] = exec_insert,
    [':'] = NULL,
    ['b'] = NULL,
    ['t'] = NULL,
    ['r'] = exec_read_file,
    ['w'] = exec_write,
    ['d'] = exec_delete,
    ['D'] = exec_delete_newline,
    ['g'] = exec_replace_pattern_by_hold,
    ['G'] = exec_append_pattern_by_hold,
    ['h'] = exec_replace_hold_by_pattern,
    ['H'] = exec_append_hold_by_pattern,
    ['l'] = exec_print_escape,
    ['n'] = exec_next,
    ['N'] = exec_next_append,
    ['p'] = exec_print,
    ['P'] = exec_print_until_newline,
    ['q'] = exec_quit,
    ['x'] = exec_exchange,
    ['='] = exec_print_line_number,
    ['#'] = exec_comment,
    ['s'] = exec_substitute,
    ['y'] = exec_translate,
};

void
exec_command(struct command *command)
{
    (exec_func_lookup[(size_t)command->id])(&command->data);
}

void
exec_line(char *line, script_t commands)
{
    for (size_t i = 0; commands[i].id != COMMAND_LAST; i++)
    {
        exec_command(&commands[i]);
        /* next_cycle(); */
    }
}

static char  *filepaths_stdin_only[] = {"-"};
static char **filepaths = NULL;
static size_t filepaths_len = 0;

void
exec(script_t commands, char *local_filepaths[], size_t local_filepaths_len)
{
    filepaths = local_filepaths;
    filepaths_len = local_filepaths_len;
    if (local_filepaths_len == 0)
    {
        filepaths = filepaths_stdin_only;
        filepaths_len = 1;
    }
}

FILE *
current_file(void)
{
    static size_t filepaths_index = 0;
    static FILE  *file = NULL;

    if (file != NULL && !feof(file))
        return file;
    if (filepaths_index == filepaths_len - 1 && feof(file))
        return NULL;
    if (file != NULL && file != stdin)
        fclose(file);
    if (file != NULL)
        filepaths_index++;
    char *filepath = filepaths[filepaths_index];
    if (strcmp(filepath, "-") == 0)
        return stdin;
    file = fopen(filepath, "r");
    if (file == NULL)
    {
        put_error("can't read %s: %s", filepath, strerror(errno));
        return current_file();
    }
    return file;
}

static const size_t line_size_init = 4098;

static char *
next_cycle(void)
{
    static size_t line_size = line_size_init;
    static char  *line = NULL;
    if (line == NULL)
        line = xmalloc(sizeof(char) * line_size);
    FILE *file = current_file();
    if (file == NULL)
    {
        free(line);
        return NULL;
    }
    errno = 0;
    if (getline(&line, &line_size, file) == -1 && errno != 0)
        die("error getline: %s", strerror(errno));
    line_index++;
    // strcpy(pattern_space, line);
    return line;
}

/******************************************************************/
/* debug fonctions used to access global variables during testing */

char *
_debug_exec_pattern_space(void)
{
    return pattern_space;
}

char *
_debug_exec_hold_space(void)
{
    return hold_space;
}

char *
_debug_exec_set_pattern_space(const char *content)
{
    return strcpy(pattern_space, content);
}

char *
_debug_exec_set_hold_space(const char *content)
{
    return strcpy(hold_space, content);
}
