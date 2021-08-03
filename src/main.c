#include "sed.h"

void
function_append(void)
{
}
void
function_insert(void)
{
}

command_function command_functions_lookup[] = {
    ['a'] = function_append,
    ['i'] = function_insert,
};

#define CHAR_SPACE_MAX 8192
// need under buffer for implementation of the 'x' command (can't swap array aka constant pointers)
static char  pattern_space_under[CHAR_SPACE_MAX + 1] = {'\0'};
static char  hold_space_under[CHAR_SPACE_MAX + 1]    = {'\0'};
static char *pattern_space                           = pattern_space_under;
static char *hold_space                              = hold_space_under;

static bool auto_print = true;

char *script_string = NULL;

int
main(int argc, char *argv[])
{
    int option;
    while ((option = getopt(argc, argv, "e:f:n")) != -1)
    {
        switch (option)
        {
        case 'e':
            script_string = strjoinf(script_string, optarg, "\n", NULL);
            break;
        case 'f':
            script_string = strjoinf(script_string, read_file(optarg), "\n", NULL);
            break;
        case 'n':
            auto_print = false;
            break;
        }
    }
    if (script_string == NULL)
    {
        if (argc == optind)
            die("missing script");
        script_string = argv[optind];
    }

    struct command command;
    parse_command(script_string, &command);

    printf("%d\n", command.addresses.count);
    printf("%d\n", command.inverse);
    printf("%s\n", command.data.text);

    /* script = parse_script(script_string); */

    return 0;
}
