#include "sed.h"
#include <criterion/criterion.h>

static struct address address;
static char *rest;
static char input[64];

Test(parse_address, last)
{
	rest = parse_address("$", &address);
	cr_assert_str_empty(rest);
	cr_assert_eq(address.type, ADDRESS_LAST);
}

Test(parse_address, line)
{
	rest = parse_address("1", &address);
	cr_assert_str_empty(rest);
	cr_assert_eq(address.type, ADDRESS_LINE);
	cr_assert_eq(address.data.line, 1);

	rest = parse_address("1000", &address);
	cr_assert_str_empty(rest);
	cr_assert_eq(address.type, ADDRESS_LINE);
	cr_assert_eq(address.data.line, 1000);

	rest = parse_address("001", &address);
	cr_assert_str_empty(rest);
	cr_assert_eq(address.type, ADDRESS_LINE);
	cr_assert_eq(address.data.line, 1);
}

Test(parse_address, line_error, .exit_code = 1)
{
	parse_address("0", &address);
}

Test(parse_address, re)
{
 	strcpy(input, "/abc*/");
	rest = parse_address(input, &address);
	cr_assert_str_empty(rest);
	cr_assert_eq(address.type, ADDRESS_RE);
	cr_assert_eq(regexec(&address.data.preg, "abc", 0, NULL, 0), 0);
	cr_assert_eq(regexec(&address.data.preg, "abcccc", 0, NULL, 0), 0);
	cr_assert_eq(regexec(&address.data.preg, "bccc", 0, NULL, 0), REG_NOMATCH);

 	strcpy(input, "|abc*|");
	rest = parse_address(input, &address);
	cr_assert_str_empty(rest);
	cr_assert_eq(address.type, ADDRESS_RE);
	cr_assert_eq(regexec(&address.data.preg, "abc", 0, NULL, 0), 0);
	cr_assert_eq(regexec(&address.data.preg, "abcccc", 0, NULL, 0), 0);
	cr_assert_eq(regexec(&address.data.preg, "bccc", 0, NULL, 0), REG_NOMATCH);
}

Test(parse_address, re_escape)
{
 	strcpy(input, "/a\\/bc*/");
	rest = parse_address(input, &address);
	cr_assert_str_empty(rest);
	cr_assert_eq(address.type, ADDRESS_RE);
	cr_assert_eq(regexec(&address.data.preg, "a/bc", 0, NULL, 0), 0);
	cr_assert_eq(regexec(&address.data.preg, "a/bcccc", 0, NULL, 0), 0);
	cr_assert_eq(regexec(&address.data.preg, "/bccc", 0, NULL, 0), REG_NOMATCH);
}

Test(parse_address, re_error, .exit_code = 1)
{
 	strcpy(input, "/abc*");
	parse_address(input, &address);
}

Test(parse_address, re_escape_error, .exit_code = 1)
{
 	strcpy(input, "/a\\/bc*");
	parse_address(input, &address);
}

Test(parse_address, re_regex_error, .exit_code = 1)
{
 	strcpy(input, "/ab[c/");
	parse_address(input, &address);
}

Test(parse_address, output)
{
	cr_assert_str_eq("foo", parse_address("1foo", &address));
	cr_assert_str_eq("foo", parse_address("1000foo", &address));
	cr_assert_str_eq("foo", parse_address("$foo", &address));
	cr_assert_str_eq("foo", parse_address(strcpy(input, "/abc*/foo"), &address));
	cr_assert_str_eq("foo", parse_address(strcpy(input, "/a\\/bc*/foo"), &address));
}
