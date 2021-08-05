#include "sed.h"
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

struct command command;

/* Test(exec_command, print) */
/* { */
/* 	command.id = 'p'; */
/* 	cr_redirect_stdout(); */
/* 	_debug_exec_set_pattern_space("bonjour\n"); */
/* 	exec_command(&command); */
/* 	cr_expect_stdout_eq_str("bonjour\n"); */
/* } */

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
    cr_assert_str_eq("bar\nbaz\n", _debug_exec_pattern_space());
}

Test(exec_command, exec_replace_pattern_by_hold)
{
    command.id = 'g';
    _debug_exec_set_pattern_space("foo");
    exec_command(&command);
}

Test(exec_command, exec_append_pattern_by_hold)
{
    command.id = 'G';
    _debug_exec_set_pattern_space("foo");
    exec_command(&command);
}

Test(exec_command, exec_replace_hold_by_pattern)
{
    command.id = 'h';
    _debug_exec_set_pattern_space("foo");
    exec_command(&command);
}

Test(exec_command, exec_append_hold_by_pattern)
{
    command.id = 'H';
    _debug_exec_set_pattern_space("foo");
    exec_command(&command);
}

Test(exec_command, exchange)
{
    command.id = 'x';
    _debug_exec_set_pattern_space("foo");
    _debug_exec_set_hold_space("bar");
    exec_command(&command);
    cr_assert_str_eq("bar", _debug_exec_pattern_space());
    cr_assert_str_eq("foo", _debug_exec_hold_space());
    exec_command(&command);
    cr_assert_str_eq("foo", _debug_exec_pattern_space());
    cr_assert_str_eq("bar", _debug_exec_hold_space());
    exec_command(&command);
    cr_assert_str_eq("bar", _debug_exec_pattern_space());
    cr_assert_str_eq("foo", _debug_exec_hold_space());
}
