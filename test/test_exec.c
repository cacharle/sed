#include "sed.h"
#include <assert.h>
#include <criterion/criterion.h>
#include <criterion/redirect.h>

char *
_debug_exec_set_pattern_space(const char *content);
char *
_debug_exec_set_hold_space(const char *content);
char *
_debug_exec_hold_space(void);
char *
_debug_exec_pattern_space(void);

static struct command command;

Test(exec_command, insert)
{
    cr_redirect_stdout();
    command.id = 'i';
    command.data.text = "bonjour";
    exec_command(&command);
    fflush(stdout);
    cr_expect_stdout_eq_str("bonjour");
}

Test(exec_command, read_file)
{
    char template[] = "/tmp/sed_testXXXXXX";
    FILE *tmp_file = fdopen(mkstemp(template), "w+");
    char *expected = "bonjourjesuis lala";
    fputs(expected, tmp_file);
    fclose(tmp_file);
    cr_redirect_stdout();
    command.id = 'r';
    command.data.text = template;
    exec_command(&command);
    remove(template);
    fflush(stdout);
    cr_expect_stdout_eq_str(expected);
}

Test(exec_command, read_file_not_exist)
{
    cr_redirect_stdout();
    command.id = 'r';
    command.data.text = "/foo/bar/qux";
    exec_command(&command);
    fflush(stdout);
    FILE  *cr_stdout = cr_get_redirected_stdout();
    char   buf[8] = {0};
    size_t read_size = fread(buf, sizeof(char), 8, cr_stdout);
    cr_expect_eq(read_size, 0);
    cr_expect_stdout_eq_str("");
}

Test(exec_command, print_base)
{
    cr_redirect_stdout();
    command.id = 'p';
    _debug_exec_set_pattern_space("bonjour");
    exec_command(&command);
    fflush(stdout);
    cr_expect_stdout_eq_str("bonjour");
}

Test(exec_command, print_with_newlines)
{
    cr_redirect_stdout();
    command.id = 'p';
    _debug_exec_set_pattern_space("bon\njour\n");
    exec_command(&command);
    fflush(stdout);
    cr_expect_stdout_eq_str("bon\njour\n");
}

Test(exec_command, print_until_newline)
{
    command.id = 'P';
    cr_redirect_stdout();
    _debug_exec_set_pattern_space("bonj\nour\n");
    exec_command(&command);
    fflush(stdout);
    cr_expect_stdout_eq_str("bonj");
}

Test(exec_command, print_until_newline_without_newline)
{
    command.id = 'P';
    cr_redirect_stdout();
    _debug_exec_set_pattern_space("bonjour");
    exec_command(&command);
    fflush(stdout);
    cr_expect_stdout_eq_str("bonjour");
}

Test(exec_command, delete)
{
    command.id = 'd';
    _debug_exec_set_pattern_space("foo");
    exec_command(&command);
    cr_assert_str_empty(_debug_exec_pattern_space());
    _debug_exec_set_pattern_space("foo\nbar\nbaz");
    exec_command(&command);
    cr_assert_str_empty(_debug_exec_pattern_space());
}

Test(exec_command, delete_newline)
{
    command.id = 'D';
    _debug_exec_set_pattern_space("foo");
    exec_command(&command);
    cr_assert_str_empty(_debug_exec_pattern_space());
    _debug_exec_set_pattern_space("foo\nbar\nbaz");
    exec_command(&command);
    cr_assert_str_eq(_debug_exec_pattern_space(), "bar\nbaz");
}

Test(exec_command, exec_replace_pattern_by_hold)
{
    command.id = 'g';
    _debug_exec_set_pattern_space("foo");
    _debug_exec_set_hold_space("bar");
    exec_command(&command);
    cr_assert_str_eq(_debug_exec_pattern_space(), "bar");
    cr_assert_str_eq(_debug_exec_hold_space(), "bar");
}

Test(exec_command, exec_append_pattern_by_hold)
{
    command.id = 'G';
    _debug_exec_set_pattern_space("foo");
    _debug_exec_set_hold_space("bar");
    exec_command(&command);
    cr_assert_str_eq(_debug_exec_pattern_space(), "foo\nbar");
    cr_assert_str_eq(_debug_exec_hold_space(), "bar");
}

Test(exec_command, exec_replace_hold_by_pattern)
{
    command.id = 'h';
    _debug_exec_set_pattern_space("foo");
    _debug_exec_set_hold_space("bar");
    exec_command(&command);
    cr_assert_str_eq(_debug_exec_hold_space(), "foo");
    cr_assert_str_eq(_debug_exec_pattern_space(), "foo");
}

Test(exec_command, exec_append_hold_by_pattern)
{
    command.id = 'H';
    _debug_exec_set_pattern_space("foo");
    _debug_exec_set_hold_space("bar");
    exec_command(&command);
    cr_assert_str_eq(_debug_exec_hold_space(), "bar\nfoo");
    cr_assert_str_eq(_debug_exec_pattern_space(), "foo");
}

Test(exec_command, exchange)
{
    command.id = 'x';
    _debug_exec_set_pattern_space("foo");
    _debug_exec_set_hold_space("bar");
    exec_command(&command);
    cr_assert_str_eq(_debug_exec_pattern_space(), "bar");
    cr_assert_str_eq(_debug_exec_hold_space(), "foo");
    exec_command(&command);
    cr_assert_str_eq(_debug_exec_pattern_space(), "foo");
    cr_assert_str_eq(_debug_exec_hold_space(), "bar");
    exec_command(&command);
    cr_assert_str_eq(_debug_exec_pattern_space(), "bar");
    cr_assert_str_eq(_debug_exec_hold_space(), "foo");
}

Test(exec_command, translate)
{
    command.id = 'y';
    command.data.translate.from = "ABC";
    command.data.translate.to = "DEF";
    _debug_exec_set_pattern_space("ABCfooAbarBbazCABC");
    exec_command(&command);
    cr_assert_str_eq(_debug_exec_pattern_space(), "DEFfooDbarEbazFDEF");

    command.data.translate.from = "";
    command.data.translate.to = "";
    _debug_exec_set_pattern_space("fooAbarBbazC");
    exec_command(&command);
    cr_assert_str_eq(_debug_exec_pattern_space(), "fooAbarBbazC");

    command.data.translate.from = "\n\t\r";
    command.data.translate.to = "ABC";
    _debug_exec_set_pattern_space("foo\nbar\tbaz\r");
    exec_command(&command);
    cr_assert_str_eq(_debug_exec_pattern_space(), "fooAbarBbazC");
}

Test(exec_command, substitute)
{
    command.id = 's';
    command.data.substitute.occurence_index = 0;
    assert(regcomp(&command.data.substitute.preg, "abc*", 0) == 0);
    command.data.substitute.replacement = "foo";

    _debug_exec_set_pattern_space("abccccc");
    exec_command(&command);
    cr_assert_str_eq(_debug_exec_pattern_space(), "foo");

    _debug_exec_set_pattern_space("###abccccc###");
    exec_command(&command);
    cr_assert_str_eq(_debug_exec_pattern_space(), "###foo###");

    _debug_exec_set_pattern_space("###abccccc###abccc###");
    exec_command(&command);
    cr_assert_str_eq(_debug_exec_pattern_space(), "###foo###abccc###");
}

Test(exec_command, substitute_regex_group)
{
    command.id = 's';
    command.data.substitute.occurence_index = 0;

    assert(regcomp(&command.data.substitute.preg, "\\(abc*\\)_\\(def*\\)", 0) == 0);
    command.data.substitute.replacement = "[\\1]foo[\\2]";
    _debug_exec_set_pattern_space("###abccc_defff###");
    exec_command(&command);
    cr_assert_str_eq(_debug_exec_pattern_space(), "###[abccc]foo[defff]###");

    assert(regcomp(&command.data.substitute.preg,
                   "_\\(a\\)_\\(b\\)_\\(c\\)_\\(d\\)_\\(e\\)_\\(f\\)_\\(g\\)_\\(h\\)"
                   "_\\(i\\)_",
                   0) == 0);
    command.data.substitute.replacement = "-\\1-\\2-\\3-\\4-\\5-\\6-\\7-\\8-\\9-";
    _debug_exec_set_pattern_space("###_a_b_c_d_e_f_g_h_i_###");
    exec_command(&command);
    cr_assert_str_eq(_debug_exec_pattern_space(), "###-a-b-c-d-e-f-g-h-i-###");

    assert(regcomp(&command.data.substitute.preg, "I\\(abc*\\)I", 0) == 0);
    command.data.substitute.replacement = "\\0_&_\\1";
    _debug_exec_set_pattern_space("###IabcccI###");
    exec_command(&command);
    cr_assert_str_eq(_debug_exec_pattern_space(), "###IabcccI_IabcccI_abccc###");

    assert(regcomp(&command.data.substitute.preg, "I\\(abc*\\)I", 0) == 0);
    command.data.substitute.replacement = "\\2\\3\\0\\4_&\\5\\9_\\6\\1\\7\\8";
    _debug_exec_set_pattern_space("###IabcccI###");
    exec_command(&command);
    cr_assert_str_eq(_debug_exec_pattern_space(), "###IabcccI_IabcccI_abccc###");
}

Test(exec_command, substitute_escape)
{
    command.id = 's';
    command.data.substitute.occurence_index = 0;
    assert(regcomp(&command.data.substitute.preg, "\\(abc*\\)_\\(def*\\)", 0) == 0);
    command.data.substitute.replacement = "\\\\[\\1]\\f\\o\\&o\\[\\2]";
    _debug_exec_set_pattern_space("###abccc_defff###");
    exec_command(&command);
    cr_assert_str_eq(_debug_exec_pattern_space(), "###\\[abccc]fo&o[defff]###");
}

// Test(current_file, base)
// {
// 	cr_redirect_stdin();
// 	fputs("bonjour", stdin);
// 	char *filepaths[] = {};
// 	size_t filepaths_len = 0;
// 	FILE *file = NULL;
// 	file = current_file();
// 	cr_asser_eq(file, stdin);
// 	while (fgetc(stdin) != EOF)
// 		;
// 	file = current_file();
// 	cr_asser_eq(file, NULL);
// }
