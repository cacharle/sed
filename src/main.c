#include "sed.h"

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
    script_t script = parse(script_string);
    // exec(argv + optind, argc - optind, script);
    return EXIT_SUCCESS;
}
