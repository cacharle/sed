#include "sed.h"

#define CHAR_SPACE_MAX 20480
// need under buffer for implementation of the 'x' command (can't swap array aka
// constant pointers)
static char   pattern_space_under[CHAR_SPACE_MAX + 1] = {'\0'};
static char   hold_space_under[CHAR_SPACE_MAX + 1] = {'\0'};
static char * pattern_space = pattern_space_under;
static char * hold_space = hold_space_under;
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

void
exec_translate(struct command *command)
{
    char *from = command->data.translate.from;
    for (size_t i = 0; pattern_space[i] != '\0'; i++)
    {
        char *from_found = strchr(from, pattern_space[i]);
        if (from_found == NULL)
            continue;
        pattern_space[i] = command->data.translate.to[from_found - from];
    }
}

typedef void (*exec_func)(struct command *);

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
    ['y'] = exec_translate,
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
        /* next_cycle(); */
    }
}

static char * filepaths_stdin_only[] = {"-"};
static char **filepaths = NULL;
static size_t filepaths_len = 0;

void
exec_init(char **local_filepaths, size_t local_filepaths_len)
{
    filepaths = local_filepaths;
    filepaths_len = local_filepaths_len;
    if (local_filepaths_len == 0)
        filepaths = filepaths_stdin_only;
}

void
exec(char *local_filepaths[], size_t local_filepaths_len, script_t commands)
{
    exec_init(local_filepaths, local_filepaths_len);
    // run
}

FILE *
current_file(void)
{
    static size_t filepaths_index = 0;
    static FILE * file = NULL;

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
        file = stdin;
    else
    {
        file = fopen(filepath, "r");
        if (file == NULL)
        {
            put_error("can't read %s: %s", filepath, strerror(errno));
            return current_file();
        }
    }
    return file;
}

static const size_t line_size_init = 4098;

char *
next_cycle(void)
{
    static size_t line_size = line_size_init;
    static char * line = NULL;
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
    strcpy(pattern_space, line);
    return line;
}

/*****************************************************************************/
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
