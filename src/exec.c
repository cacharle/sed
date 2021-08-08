#include "sed.h"

#define CHAR_SPACE_MAX 20480
// need under buffer for implementation of the 'x' command (can't swap array aka
// constant pointers)
static char   pattern_space_under[CHAR_SPACE_MAX + 1] = {'\0'};
static char   hold_space_under[CHAR_SPACE_MAX + 1] = {'\0'};
static char * pattern_space = pattern_space_under;
static char * hold_space = hold_space_under;
static char * line = NULL;
static size_t line_size = 0;
static size_t line_index = 0;

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
exec_print_no_newline()
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

typedef void (*exec_func)(union command_data *);

static const exec_func exec_func_lookup[] = {
    ['{'] = NULL,
    ['}'] = NULL,
    ['a'] = NULL,
    ['c'] = NULL,
    ['i'] = NULL,
    [':'] = NULL,
    ['b'] = NULL,
    ['t'] = NULL,
    ['r'] = NULL,
    ['w'] = NULL,
    ['d'] = exec_delete,
    ['D'] = exec_delete_newline,
    ['g'] = exec_replace_pattern_by_hold,
    ['G'] = exec_append_pattern_by_hold,
    ['h'] = exec_replace_hold_by_pattern,
    ['H'] = exec_append_hold_by_pattern,
    ['l'] = NULL,
    ['n'] = NULL,
    ['N'] = NULL,
    ['p'] = exec_print,
    ['P'] = NULL,
    ['q'] = NULL,
    ['x'] = exec_exchange,
    ['='] = NULL,
    ['#'] = NULL,
    ['s'] = NULL,
    ['y'] = NULL,
};

void
exec_command(struct command *command)
{
    (exec_func_lookup[(size_t)command->id])(command);
}

void
exec_line(char *line, script_t commands)
{
    for (size_t i = 0; commands[i].id != COMMAND_LAST; i++)
    {
        exec_command(&commands[i]);
    }
}

void
exec_file(FILE *file, script_t commands)
{
    while (getline(&line, &line_size, file))
    {
        line_index++;
        exec_line(line, commands);
    }
}

static const size_t line_init_size = 4098;

void
exec(char *filepaths[], int filepaths_len, script_t commands)
{
    line_size = line_init_size;
    line = xmalloc(sizeof(char) * line_size);
    if (filepaths_len == 0)
    {
        exec_file(stdin, commands);
        return;
    }
    for (int i = 0; i < filepaths_len; i++)
    {
        FILE *file = NULL;
        if (strcmp(filepaths[i], "-") == 0)
            file = stdin;
        else
        {
            file = fopen(filepaths[i], "r");
            if (file == NULL)
                put_error("can't read %s: %s", filepaths[i], strerror(errno));
        }
        exec_file(file, commands);
        if (file != stdin)
            fclose(file);
    }
    free(line);
}

/*****************************************************************************/

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
