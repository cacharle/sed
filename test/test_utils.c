#include "sed.h"
#include <criterion/criterion.h>

Test(strjoinf, base)
{
    char *s;
    s = strjoinf(NULL);
    cr_assert_str_empty(s);
    free(s);
    s = strjoinf(strdup("bonjour"), NULL);
    cr_assert_str_eq(s, "bonjour");
    free(s);
    s = strjoinf(strdup("bonjour"), "je", "suis", "charles", NULL);
    cr_assert_str_eq(s, "bonjourjesuischarles");
    free(s);
    s = strjoinf(strdup("bonjour"), NULL, "je", "suis", "charles");
    cr_assert_str_eq(s, "bonjour");
    free(s);
}

Test(read_file, base)
{
    char template[] = "/tmp/sed_testXXXXXX";  // modified by mkstemp
    FILE *tmp_file  = fdopen(mkstemp(template), "w+");
    char *expected  = "bonjour je suis charles";
    fputs(expected, tmp_file);
    fclose(tmp_file);
    char *actual = read_file(template);
    remove(template);
    cr_assert_str_eq(actual, expected);
}
