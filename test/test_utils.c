#include "sed.h"
#include <assert.h>
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
    FILE *tmp_file = fdopen(mkstemp(template), "w+");
    assert(tmp_file != NULL);
    char *expected = "bonjour je suis charles";
    fputs(expected, tmp_file);
    fclose(tmp_file);
    char *actual = read_file(template);
    remove(template);
    cr_assert_str_eq(actual, expected);
}

Test(read_file, error, .exit_code = 1)
{
    read_file("doesnotexist");
}

Test(todigit, base)
{
    cr_assert_eq(todigit('0'), 0);
    cr_assert_eq(todigit('1'), 1);
    cr_assert_eq(todigit('2'), 2);
    cr_assert_eq(todigit('3'), 3);
    cr_assert_eq(todigit('4'), 4);
    cr_assert_eq(todigit('5'), 5);
    cr_assert_eq(todigit('5'), 5);
    cr_assert_eq(todigit('6'), 6);
    cr_assert_eq(todigit('8'), 8);
    cr_assert_eq(todigit('9'), 9);
    cr_assert_eq(todigit('a'), -1);
    cr_assert_eq(todigit('~'), -1);
}
