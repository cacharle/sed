#include "sed.h"
#include <assert.h>
#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <criterion/redirect.h>
#include <errno.h>

char *
_debug_exec_pattern_space(void);
char *
_debug_exec_hold_space(void);
bool
_debug_exec_last_line(void);
char *
_debug_exec_set_pattern_space(const char *content);
char *
_debug_exec_set_hold_space(const char *content);
void
_debug_exec_set_line_index(const size_t line_index_);
void
_debug_exec_set_last_line(const bool last_line_);
void
exec_init(char **local_filepaths, size_t local_filepaths_len, bool auto_print_);
FILE *
current_file(void);
char *
next_line(void);
void
exec_commands(script_t commands);

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

Test(exec_command, substitute_occurence)
{
    command.id = 's';
    command.data.substitute.occurence_index = 1;
    assert(regcomp(&command.data.substitute.preg, "abc*", 0) == 0);
    command.data.substitute.replacement = "foo";
    _debug_exec_set_pattern_space("###abccc###abccc###abccc###");
    exec_command(&command);
    cr_assert_str_eq(_debug_exec_pattern_space(), "###foo###abccc###abccc###");

    command.data.substitute.occurence_index = 2;
    command.data.substitute.replacement = "foo";
    _debug_exec_set_pattern_space("###abccc###abccc###abccc###");
    exec_command(&command);
    cr_assert_str_eq(_debug_exec_pattern_space(), "###abccc###foo###abccc###");

    command.data.substitute.occurence_index = 3;
    command.data.substitute.replacement = "foo";
    _debug_exec_set_pattern_space("###abccc###abccc###abccc###");
    exec_command(&command);
    cr_assert_str_eq(_debug_exec_pattern_space(), "###abccc###abccc###foo###");

    command.data.substitute.occurence_index = 2;
    command.data.substitute.replacement = "foo";
    _debug_exec_set_pattern_space("###abcccabcccabccc###");
    exec_command(&command);
    cr_assert_str_eq(_debug_exec_pattern_space(), "###abcccfooabccc###");
}

Test(exec_command, substitute_global)
{
    command.id = 's';
    command.data.substitute.occurence_index = 0;
    command.data.substitute.global = true;
    assert(regcomp(&command.data.substitute.preg, "abc*", 0) == 0);
    command.data.substitute.replacement = "foo";
    _debug_exec_set_pattern_space("###abccc###abccc###abccc###");
    exec_command(&command);
    cr_assert_str_eq(_debug_exec_pattern_space(), "###foo###foo###foo###");

    _debug_exec_set_pattern_space("###abcccabcccabccc###");
    exec_command(&command);
    cr_assert_str_eq(_debug_exec_pattern_space(), "###foofoofoo###");
}

Test(exec_command, substitute_print)
{
    command.id = 's';
    command.data.substitute.occurence_index = 0;
    command.data.substitute.print = true;
    assert(regcomp(&command.data.substitute.preg, "abc*", 0) == 0);
    command.data.substitute.replacement = "foo";
    _debug_exec_set_pattern_space("###abccc###");
    fflush(stdout);
    cr_redirect_stdout();
    exec_command(&command);
    char *expected = "###foo###";
    cr_assert_str_eq(_debug_exec_pattern_space(), expected);
    fflush(stdout);
    cr_expect_stdout_eq_str(expected);
}

Test(exec_command, substitute_print_no_replacement)
{
    command.id = 's';
    command.data.substitute.occurence_index = 0;
    command.data.substitute.print = true;
    assert(regcomp(&command.data.substitute.preg, "abc*", 0) == 0);
    command.data.substitute.replacement = "foo";
    _debug_exec_set_pattern_space("###accc###");
    fflush(stdout);
    cr_redirect_stdout();
    exec_command(&command);
    cr_assert_str_eq(_debug_exec_pattern_space(), "###accc###");
    fflush(stdout);
    cr_expect_stdout_eq_str("");
}

Test(exec_command, substitute_write_file)
{
    char template[] = "/tmp/sed_testXXXXXX";
    FILE *tmp_file = fdopen(mkstemp(template), "w");
    assert(tmp_file != NULL);
    fputs("bonjour\naurevoir\n", tmp_file);
    fclose(tmp_file);

    command.id = 's';
    command.data.substitute.occurence_index = 0;
    command.data.substitute.write_filepath = template;
    assert(regcomp(&command.data.substitute.preg, "abc*", 0) == 0);
    command.data.substitute.replacement = "foo";
    _debug_exec_set_pattern_space("###abccc###");
    exec_command(&command);
    cr_assert_str_eq(_debug_exec_pattern_space(), "###foo###");

    tmp_file = fopen(template, "r");
    assert(tmp_file != NULL);
    cr_expect_file_contents_eq_str(tmp_file, "bonjour\naurevoir\n###foo###");
    fclose(tmp_file);
}

Test(exec_command, substitute_write_file_no_replacement)
{
    char template[] = "/tmp/sed_testXXXXXX";
    FILE *tmp_file = fdopen(mkstemp(template), "w");
    assert(tmp_file != NULL);
    fputs("bonjour\naurevoir\n", tmp_file);
    fclose(tmp_file);

    command.id = 's';
    command.data.substitute.occurence_index = 0;
    command.data.substitute.write_filepath = template;
    assert(regcomp(&command.data.substitute.preg, "abc*", 0) == 0);
    command.data.substitute.replacement = "foo";
    _debug_exec_set_pattern_space("###accc###");
    exec_command(&command);
    cr_assert_str_eq(_debug_exec_pattern_space(), "###accc###");

    tmp_file = fopen(template, "r");
    assert(tmp_file != NULL);
    cr_expect_file_contents_eq_str(tmp_file, "bonjour\naurevoir\n");
    fclose(tmp_file);
}

Test(exec_command, print_escape_nothing_special)
{
    cr_redirect_stdout();
    command.id = 'l';
    _debug_exec_set_pattern_space("bonjour");
    exec_command(&command);
    fflush(stdout);
    cr_expect_stdout_eq_str("bonjour");
}

Test(exec_command, print_escape_with_newline)
{
    cr_redirect_stdout();
    command.id = 'l';
    _debug_exec_set_pattern_space("bonjour\n");
    exec_command(&command);
    fflush(stdout);
    cr_expect_stdout_eq_str("bonjour$\n");
}

Test(exec_command, print_escape_with_escape)
{
    cr_redirect_stdout();
    command.id = 'l';
    _debug_exec_set_pattern_space("\\_\b_\t_\r_\v_\f_\n");
    exec_command(&command);
    fflush(stdout);
    cr_expect_stdout_eq_str("\\\\_\\b_\\t_\\r_\\v_\\f_$\n");
}

Test(exec_command, print_escape_with_non_printable)
{
    cr_redirect_stdout();
    command.id = 'l';
    _debug_exec_set_pattern_space("\033\037\001\004\177");
    exec_command(&command);
    fflush(stdout);
    cr_expect_stdout_eq_str("\\033\\037\\001\\004\\177");
}

Test(exec_command, print_escape_fold)
{
    cr_redirect_stdout();
    command.id = 'l';
    _debug_exec_set_pattern_space("0123456789"
                                  "0123456789"
                                  "0123456789"
                                  "0123456789"
                                  "0123456789"
                                  "0123456789"
                                  "foo");
    exec_command(&command);
    fflush(stdout);
    cr_expect_stdout_eq_str("0123456789"
                            "0123456789"
                            "0123456789"
                            "0123456789"
                            "0123456789"
                            "0123456789\\\n"
                            "foo");
}

Test(current_file, no_filepaths)
{
    char  *filepaths[] = {};
    size_t filepaths_len = 0;
    exec_init(filepaths, filepaths_len, false);
    FILE *file = current_file();
    cr_expect_eq(file, stdin);
}

Test(current_file, one_file)
{
    char template[] = "/tmp/sed_testXXXXXX";
    FILE *t = fdopen(mkstemp(template), "w");
    assert(t != NULL);
    fputs("bonjour", t);
    fclose(t);
    char  *filepaths[] = {template};
    size_t filepaths_len = 1;
    exec_init(filepaths, filepaths_len, false);
    FILE *file = current_file();
    cr_expect(!feof(file));
    cr_expect_eq(file, current_file());
    while (fgetc(file) != EOF)
        ;
    cr_expect(feof(file));
    cr_expect_null(current_file());
}

Test(current_file, two_file)
{
    char  template1[] = "/tmp/sed_testXXXXXX";
    FILE *t1 = fdopen(mkstemp(template1), "w");
    assert(t1 != NULL);
    fputs("bonjour", t1);
    fclose(t1);

    char  template2[] = "/tmp/sed_testXXXXXX";
    FILE *t2 = fdopen(mkstemp(template2), "w");
    assert(t2 != NULL);
    fputs("bonjour", t2);
    fclose(t2);

    char  *filepaths[] = {template1, template2};
    size_t filepaths_len = 2;
    exec_init(filepaths, filepaths_len, false);

    FILE *file = current_file();
    cr_expect(!feof(file));
    cr_expect_eq(file, current_file());
    while (fgetc(file) != EOF)
        ;
    cr_expect(feof(file));

    file = current_file();
    cr_expect_not_null(file);
    cr_expect(!feof(file));
    cr_expect_eq(file, current_file());
    while (fgetc(file) != EOF)
        ;
    cr_expect(feof(file));

    cr_expect_null(current_file());
}

Test(next_line, one_file_three_lines)
{
    char template[] = "/tmp/sed_testXXXXXX";
    FILE *t = fdopen(mkstemp(template), "w");
    assert(t != NULL);
    fputs("a\nb\nc\n", t);
    fclose(t);
    char  *filepaths[] = {template};
    size_t filepaths_len = 1;
    exec_init(filepaths, filepaths_len, false);
    char *line;
    line = next_line();
    cr_expect_str_eq(line, "a\n");
    cr_expect(!_debug_exec_last_line());
    line = next_line();
    cr_expect_str_eq(line, "b\n");
    line = next_line();
    cr_expect_str_eq(line, "c\n");
    line = next_line();
    cr_expect_null(line);
    cr_expect(_debug_exec_last_line());
}

Test(next_line, two_files_four_lines)
{
    char  template1[] = "/tmp/sed_testXXXXXX";
    FILE *t1 = fdopen(mkstemp(template1), "w");
    assert(t1 != NULL);
    fputs("a\nb\n", t1);
    fclose(t1);
    char  template2[] = "/tmp/sed_testXXXXXX";
    FILE *t2 = fdopen(mkstemp(template2), "w");
    assert(t2 != NULL);
    fputs("c\nd\n", t2);
    fclose(t2);

    char  *filepaths[] = {template1, template2};
    size_t filepaths_len = 2;
    exec_init(filepaths, filepaths_len, false);
    char *line;
    line = next_line();
    cr_expect_str_eq(line, "a\n");
    cr_expect(!_debug_exec_last_line());
    line = next_line();
    cr_expect_str_eq(line, "b\n");
    cr_expect(_debug_exec_last_line());
    line = next_line();
    cr_expect(!_debug_exec_last_line());
    cr_expect_str_eq(line, "c\n");
    line = next_line();
    cr_expect_str_eq(line, "d\n");
    line = next_line();
    cr_expect_null(line);
    cr_expect(_debug_exec_last_line());
}

Test(exec_command, next_no_auto_print)
{
    char template[] = "/tmp/sed_testXXXXXX";
    FILE *t = fdopen(mkstemp(template), "w");
    assert(t != NULL);
    fputs("a\nb\nc\nd\n", t);
    fclose(t);
    char  *filepaths[] = {template};
    size_t filepaths_len = 1;
    exec_init(filepaths, filepaths_len, false);

    command.id = 'n';
    exec_command(&command);
    cr_expect_str_eq(_debug_exec_pattern_space(), "a\n");
    exec_command(&command);
    cr_expect_str_eq(_debug_exec_pattern_space(), "b\n");
    exec_command(&command);
    cr_expect_str_eq(_debug_exec_pattern_space(), "c\n");
    exec_command(&command);
    cr_expect_str_eq(_debug_exec_pattern_space(), "d\n");
}

Test(exec_command, next_auto_print)
{
    char template[] = "/tmp/sed_testXXXXXX";
    FILE *t = fdopen(mkstemp(template), "w");
    assert(t != NULL);
    fputs("a\nb\nc\nd\n", t);
    fclose(t);
    char  *filepaths[] = {template};
    size_t filepaths_len = 1;
    exec_init(filepaths, filepaths_len, true);

    cr_redirect_stdout();
    _debug_exec_set_pattern_space("bonjour\n");
    command.id = 'n';
    exec_command(&command);
    cr_expect_str_eq(_debug_exec_pattern_space(), "a\n");
    exec_command(&command);
    cr_expect_str_eq(_debug_exec_pattern_space(), "b\n");
    exec_command(&command);
    cr_expect_str_eq(_debug_exec_pattern_space(), "c\n");
    exec_command(&command);
    cr_expect_str_eq(_debug_exec_pattern_space(), "d\n");
    fflush(stdout);
    cr_expect_stdout_eq_str("bonjour\na\nb\nc\n");
}

Test(exec_command, comment)
{
    command.id = '#';
    exec_command(&command);
}

Test(exec_command, quit)
{
    command.id = 'q';
    exec_command(&command);
    cr_expect(false);  // should never get executed
}

Test(exec_command, print_line_number)
{
    char template[] = "/tmp/sed_testXXXXXX";
    FILE *t = fdopen(mkstemp(template), "w");
    assert(t != NULL);
    fputs("a\nb\nc\nd\n", t);
    fclose(t);
    char  *filepaths[] = {template};
    size_t filepaths_len = 1;
    exec_init(filepaths, filepaths_len, false);

    cr_redirect_stdout();
    command.id = '=';
    next_line();
    exec_command(&command);
    next_line();
    exec_command(&command);
    next_line();
    exec_command(&command);
    next_line();
    exec_command(&command);
    fflush(stdout);
    cr_expect_stdout_eq_str("1\n2\n3\n4\n");
}

static void
exec_commands_setup()
{
    _debug_exec_set_pattern_space("foo");
    _debug_exec_set_hold_space("bar");
    _debug_exec_set_line_index(1);
}

Test(exec_commands, addresses_count_0, .init = exec_commands_setup)
{
    struct command commands[] = {
        {.id = 'G', .addresses = {.count = 0}},
        {.id = 'G', .addresses = {.count = 0}},
        {.id = COMMAND_LAST},
    };
    exec_commands(commands);
    cr_assert_str_eq(_debug_exec_pattern_space(), "foo\nbar\nbar");
}

Test(exec_commands, addresses_last_line_true, .init = exec_commands_setup)
{
    _debug_exec_set_last_line(true);
    struct command commands[] = {
        {.id = 'G', .addresses = {.count = 1, .addresses = {{ADDRESS_LAST}}}},
        {.id = COMMAND_LAST},
    };
    exec_commands(commands);
    cr_assert_str_eq(_debug_exec_pattern_space(), "foo\nbar");
}

Test(exec_commands, addresses_last_line_false, .init = exec_commands_setup)
{
    _debug_exec_set_last_line(false);
    struct command commands[] = {
        {.id = 'G', .addresses = {.count = 1, .addresses = {{ADDRESS_LAST}}}},
        {.id = COMMAND_LAST},
    };
    exec_commands(commands);
    cr_assert_str_eq(_debug_exec_pattern_space(), "foo");
}

Test(exec_commands, addresses_line_count_1_all_match, .init = exec_commands_setup)
{
    struct command commands[] = {
        {.id = 'G',
         .addresses = {.count = 1, .addresses = {{ADDRESS_LINE, {.line = 1}}}}},
        {.id = 'G',
         .addresses = {.count = 1, .addresses = {{ADDRESS_LINE, {.line = 1}}}}},
        {.id = COMMAND_LAST},
    };
    exec_commands(commands);
    cr_assert_str_eq(_debug_exec_pattern_space(), "foo\nbar\nbar");
}

Test(exec_commands, addresses_line_count_1_none_match, .init = exec_commands_setup)
{
    struct command commands[] = {
        {.id = 'G',
         .addresses = {.count = 1, .addresses = {{ADDRESS_LINE, {.line = 2}}}}},
        {.id = 'G',
         .addresses = {.count = 1, .addresses = {{ADDRESS_LINE, {.line = 3}}}}},
        {.id = COMMAND_LAST},
    };
    exec_commands(commands);
    cr_assert_str_eq(_debug_exec_pattern_space(), "foo");
}

Test(exec_commands, addresses_line_count_1_some_match, .init = exec_commands_setup)
{
    struct command commands[] = {
        {.id = 'G',
         .addresses = {.count = 1, .addresses = {{ADDRESS_LINE, {.line = 2}}}}},
        {.id = 'G',
         .addresses = {.count = 1, .addresses = {{ADDRESS_LINE, {.line = 1}}}}},
        {.id = COMMAND_LAST},
    };
    exec_commands(commands);
    cr_assert_str_eq(_debug_exec_pattern_space(), "foo\nbar");
}

Test(exec_commands, addresses_regex_count_1_some_match, .init = exec_commands_setup)
{
    struct command commands[] = {
        {.id = 'G', .addresses = {.count = 1, .addresses = {{ADDRESS_RE}}}},
        {.id = 'G', .addresses = {.count = 1, .addresses = {{ADDRESS_RE}}}},
        {.id = COMMAND_LAST},
    };
    assert(regcomp(&commands[0].addresses.addresses[0].data.preg, "abc*", 0) == 0);
    assert(regcomp(&commands[1].addresses.addresses[0].data.preg, "fo*", 0) == 0);
    exec_commands(commands);
    cr_assert_str_eq(_debug_exec_pattern_space(), "foo\nbar");
}

Test(exec_commands, addresses_line_range, .init = exec_commands_setup)  // 2,5 p
{
    struct command commands[] = {
        {'G',
         .addresses = {2,
                       {{ADDRESS_LINE, {.line = 2}}, {ADDRESS_LINE, {.line = 3}}},
                       .in_range = false}},
        {COMMAND_LAST},
    };
    exec_commands(commands);  // nothing happens
    _debug_exec_set_line_index(2);
    exec_commands(commands);  // executed
    _debug_exec_set_line_index(3);
    exec_commands(commands);  // executed
    _debug_exec_set_line_index(4);
    exec_commands(commands);  // nothing happens
    cr_assert_str_eq(_debug_exec_pattern_space(), "foo\nbar\nbar");
}

Test(exec_commands, addresses_line_last_range, .init = exec_commands_setup)  // 3,$ p
{
    struct command commands[] = {
        {'G',
         .addresses = {2,
                       {{ADDRESS_LINE, {.line = 2}}, {ADDRESS_LAST}},
                       .in_range = false}},
        {COMMAND_LAST},
    };
    exec_commands(commands);  // nothing happens
    _debug_exec_set_line_index(2);
    exec_commands(commands);  // executed
    _debug_exec_set_line_index(3);
    _debug_exec_set_last_line(true);
    exec_commands(commands);  // executed
    _debug_exec_set_line_index(4);
    exec_commands(commands);  // nothing happens
    cr_assert_str_eq(_debug_exec_pattern_space(), "foo\nbar\nbar");
}

Test(exec_commands,
     addresses_line_reverse_range,
     .init = exec_commands_setup)  // 2,1 p
{
    struct command commands[] = {
        {'G',
         .addresses = {2,
                       {{ADDRESS_LINE, {.line = 2}}, {ADDRESS_LINE, {.line = 1}}},
                       .in_range = false}},
        {COMMAND_LAST},
    };
    exec_commands(commands);  // nothing happens
    _debug_exec_set_line_index(2);
    exec_commands(commands);  // executed
    _debug_exec_set_line_index(3);
    exec_commands(commands);  // nothing happens
    cr_assert_str_eq(_debug_exec_pattern_space(), "foo\nbar");
}

Test(exec_commands,
     addresses_line_last_reverse_range,
     .init = exec_commands_setup)  // $,2 p
{
    struct command commands[] = {
        {'G',
         .addresses = {2,
                       {{ADDRESS_LAST}, {ADDRESS_LINE, {.line = 2}}},
                       .in_range = false}},
        {COMMAND_LAST},
    };
    exec_commands(commands);  // nothing happens
    _debug_exec_set_line_index(2);
    exec_commands(commands);  // nothing happens
    _debug_exec_set_line_index(3);
    _debug_exec_set_last_line(true);
    exec_commands(commands);  // executed
    _debug_exec_set_line_index(1);
    _debug_exec_set_last_line(false);
    exec_commands(commands);  // nothing happens
    cr_assert_str_eq(_debug_exec_pattern_space(), "foo\nbar");
}

Test(exec_commands, addresses_regex_range)  // /foo/,/bar/ p
{
    struct command commands[] = {
        {'H', .addresses = {2, {{ADDRESS_RE}, {ADDRESS_RE}}, .in_range = false}},
        {.id = COMMAND_LAST},
    };
    _debug_exec_set_hold_space("");
    assert(regcomp(&commands[0].addresses.addresses[0].data.preg, "#fo*", 0) == 0);
    assert(regcomp(&commands[0].addresses.addresses[1].data.preg, "#ba*", 0) == 0);
    _debug_exec_set_pattern_space("asdfasdf");
    exec_commands(commands);  // nothing happens
    _debug_exec_set_pattern_space("#foo");
    exec_commands(commands);  // executed
    _debug_exec_set_pattern_space("HELLO");
    exec_commands(commands);  // executed
    _debug_exec_set_pattern_space("#bar");
    exec_commands(commands);  // executed
    _debug_exec_set_pattern_space("asdfasdf");
    exec_commands(commands);  // nothing happens
    cr_expect_str_eq(_debug_exec_hold_space(), "\n#foo\nHELLO\n#bar");
}

Test(exec_commands, addresses_same_line_range)  // 1,/foo/ p where line 1 == foo
{
    struct command commands[] = {
        {'H',
         .addresses = {2,
                       {{ADDRESS_LINE, {.line = 1}}, {ADDRESS_RE}},
                       .in_range = false}},
        {.id = COMMAND_LAST},
    };
    _debug_exec_set_hold_space("");
    assert(regcomp(&commands[0].addresses.addresses[1].data.preg, "#fo*", 0) == 0);
    _debug_exec_set_line_index(1);
    _debug_exec_set_pattern_space("#foo");
    exec_commands(commands);  // executed
    _debug_exec_set_line_index(2);
    _debug_exec_set_pattern_space("asdf");
    exec_commands(commands);  // nothing happens
    cr_expect_str_eq(_debug_exec_hold_space(), "\n#foo");
}

Test(exec_commands, inverse_address_1, .init = exec_commands_setup)
{
    struct command commands[] = {
        {.id = 'G', .inverse = true,
         .addresses = {.count = 1, .addresses = {{ADDRESS_LINE, {.line = 2}}}}},
        {.id = 'G', .inverse = true,
         .addresses = {.count = 1, .addresses = {{ADDRESS_LINE, {.line = 1}}}}},
        {.id = 'G', .inverse = true,
         .addresses = {.count = 1, .addresses = {{ADDRESS_LINE, {.line = 3}}}}},
        {.id = COMMAND_LAST},
    };
    exec_commands(commands);
    cr_assert_str_eq(_debug_exec_pattern_space(), "foo\nbar\nbar");
}

Test(exec_commands, inverse_addresses_line_range, .init = exec_commands_setup)  // 2,5 p
{
    struct command commands[] = {
        {'G', .inverse = true,
         .addresses = {2,
                       {{ADDRESS_LINE, {.line = 2}}, {ADDRESS_LINE, {.line = 3}}},
                       .in_range = false}},
        {COMMAND_LAST},
    };
    exec_commands(commands);  // executed
    _debug_exec_set_line_index(2);
    exec_commands(commands);  // nothing happens
    _debug_exec_set_line_index(3);
    exec_commands(commands);  // nothing happens
    cr_assert_str_eq(_debug_exec_pattern_space(), "foo\nbar");
}
