#include "sed.h"
#include <criterion/criterion.h>

static struct address   address;
static struct addresses addresses;
static struct command   command;
static char            *rest;
static char             input[2048];

Test(parse_address, last)
{
    rest = parse_address("$", &address);
    cr_expect_str_empty(rest);
    cr_expect_eq(address.type, ADDRESS_LAST);
}

Test(parse_address, line)
{
    rest = parse_address("1", &address);
    cr_expect_str_empty(rest);
    cr_expect_eq(address.type, ADDRESS_LINE);
    cr_expect_eq(address.data.line, 1);

    rest = parse_address("1000", &address);
    cr_expect_str_empty(rest);
    cr_expect_eq(address.type, ADDRESS_LINE);
    cr_expect_eq(address.data.line, 1000);

    rest = parse_address("001", &address);
    cr_expect_str_empty(rest);
    cr_expect_eq(address.type, ADDRESS_LINE);
    cr_expect_eq(address.data.line, 1);
}

Test(parse_address, line_error, .exit_code = 1)
{
    parse_address("0", &address);
}

Test(parse_address, re)
{
    rest = parse_address(strcpy(input, "/abc*/"), &address);
    cr_expect_str_empty(rest);
    cr_expect_eq(address.type, ADDRESS_RE);
    cr_expect_eq(regexec(&address.data.preg, "abc", 0, NULL, 0), 0);
    cr_expect_eq(regexec(&address.data.preg, "abcccc", 0, NULL, 0), 0);
    cr_expect_eq(regexec(&address.data.preg, "bccc", 0, NULL, 0), REG_NOMATCH);

    rest = parse_address(strcpy(input, "|abc*|"), &address);
    cr_expect_str_empty(rest);
    cr_expect_eq(address.type, ADDRESS_RE);
    cr_expect_eq(regexec(&address.data.preg, "abc", 0, NULL, 0), 0);
    cr_expect_eq(regexec(&address.data.preg, "abcccc", 0, NULL, 0), 0);
    cr_expect_eq(regexec(&address.data.preg, "bccc", 0, NULL, 0), REG_NOMATCH);
}

Test(parse_address, re_escape)
{
    rest = parse_address(strcpy(input, "/a\\/bc*/"), &address);
    cr_expect_str_empty(rest);
    cr_expect_eq(address.type, ADDRESS_RE);
    cr_expect_eq(regexec(&address.data.preg, "a/bc", 0, NULL, 0), 0);
    cr_expect_eq(regexec(&address.data.preg, "a/bcccc", 0, NULL, 0), 0);
    cr_expect_eq(regexec(&address.data.preg, "/bccc", 0, NULL, 0), REG_NOMATCH);
}

Test(parse_address, re_error, .exit_code = 1)
{
    parse_address(strcpy(input, "/abc*"), &address);
}

Test(parse_address, re_escape_error, .exit_code = 1)
{
    parse_address(strcpy(input, "/a\\/bc*"), &address);
}

Test(parse_address, re_regex_error, .exit_code = 1)
{
    parse_address(strcpy(input, "/ab[c/"), &address);
}

Test(parse_address, re_backslash_delimiter_error, .exit_code = 1)
{
    parse_address(strcpy(input, "\\abc\\"), &address);
}

Test(parse_address, output)
{
    cr_expect_str_eq("foo", parse_address("$foo", &address));
    cr_expect_str_eq("foo", parse_address("1000foo", &address));
    cr_expect_str_eq("foo", parse_address(strcpy(input, "/a\\/bc*/foo"), &address));
}

Test(parse_addresses, none)
{
    rest = parse_addresses("a bonjour", &addresses);
    cr_expect_str_eq("a bonjour", rest);
    cr_expect_eq(addresses.count, 0);
}

Test(parse_addresses, end)
{
    rest = parse_addresses("$", &addresses);
    cr_expect_str_empty(rest);
    cr_expect_eq(addresses.count, 1);
    cr_expect_eq(addresses.addresses[0].type, ADDRESS_LAST);

    rest = parse_addresses("$,$", &addresses);
    cr_expect_str_empty(rest);
    cr_expect_eq(addresses.count, 2);
    cr_expect_eq(addresses.addresses[0].type, ADDRESS_LAST);
    cr_expect_eq(addresses.addresses[1].type, ADDRESS_LAST);
}

Test(parse_addresses, line)
{
    rest = parse_addresses("10", &addresses);
    cr_expect_str_empty(rest);
    cr_expect_eq(addresses.count, 1);
    cr_expect_eq(addresses.addresses[0].type, ADDRESS_LINE);

    rest = parse_addresses("10,400", &addresses);
    cr_expect_str_empty(rest);
    cr_expect_eq(addresses.count, 2);
    cr_expect_eq(addresses.addresses[0].type, ADDRESS_LINE);
    cr_expect_eq(addresses.addresses[1].type, ADDRESS_LINE);
}

Test(parse_addresses, re)
{
    rest = parse_addresses(strcpy(input, "/abc*/"), &addresses);
    cr_expect_str_empty(rest);
    cr_expect_eq(addresses.count, 1);
    cr_expect_eq(addresses.addresses[0].type, ADDRESS_RE);

    rest = parse_addresses(strcpy(input, "/abc*/,|.*cba|"), &addresses);
    cr_expect_str_empty(rest);
    cr_expect_eq(addresses.count, 2);
    cr_expect_eq(addresses.addresses[0].type, ADDRESS_RE);
    cr_expect_eq(addresses.addresses[1].type, ADDRESS_RE);
}

Test(parse_addresses, output)
{
    cr_expect_str_eq("foo", parse_addresses("$foo", &addresses));
    cr_expect_str_eq("foo", parse_addresses("$,$foo", &addresses));
    cr_expect_str_eq("foo", parse_addresses("10foo", &addresses));
    cr_expect_str_eq("foo", parse_addresses("10,400foo", &addresses));
    cr_expect_str_eq("foo", parse_addresses(strcpy(input, "/abc*/foo"), &addresses));
    cr_expect_str_eq(
        "foo", parse_addresses(strcpy(input, "/abc*/,|.*cba|foo"), &addresses));
}

Test(parse_command, inverse)
{
    rest = parse_command("!p", &command);
    cr_expect_str_empty(rest);
    cr_expect_eq(command.id, 'p');
    cr_expect(command.inverse);
}

Test(parse_command, singleton)
{
    const char *singleton_commands = "dDgGhHlnNpPqx=#";
    for (size_t i = 0; singleton_commands[i] != '\0'; i++)
    {
        input[0] = singleton_commands[i];
        input[1] = '\0';
        rest = parse_command(input, &command);
        cr_expect_str_empty(rest);
        cr_expect_eq(command.id, singleton_commands[i]);
    }
}

Test(parse_command, error_unknown_command, .exit_code = 1)
{
    parse_command("o", &command);
}

Test(parse_command, text)
{
    const char *text_commands = ":btrw";
    for (size_t i = 0; text_commands[i] != '\0'; i++)
    {
        input[0] = text_commands[i];
        strcpy(input + 1, "bonjour");
        rest = parse_command(input, &command);
        cr_expect_str_empty(rest);
        cr_expect_eq(command.id, text_commands[i]);
        cr_expect_str_eq(command.data.text, "bonjour");
    }
}

Test(parse_command, error_read_no_filepath, .exit_code = 1)
{
    parse_command(strcpy(input, "r"), &command);
}

Test(parse_command, error_write_no_filepath, .exit_code = 1)
{
    parse_command(strcpy(input, "w"), &command);
}

Test(parse_command, branch_no_filepath, .exit_code = 0)
{
    parse_command(strcpy(input, "t"), &command);
    parse_command(strcpy(input, "b"), &command);
}

Test(parse_command, escapable_text)
{
    const char *text_commands = "aci";
    for (size_t i = 0; text_commands[i] != '\0'; i++)
    {
        input[0] = text_commands[i];
        strcpy(input + 1, "\\bonjour");
        rest = parse_command(input, &command);
        cr_expect_str_empty(rest);
        cr_expect_eq(command.id, text_commands[i]);
        cr_expect_str_eq(command.data.text, "bonjour");
    }
}

Test(parse_command, escapable_text_escape)
{
    rest = parse_command(strcpy(input, "a\\\\b\\on\\j\\o\\ur"), &command);
    cr_expect_str_empty(rest);
    cr_expect_eq(command.id, 'a');
    cr_expect_str_eq(command.data.text, "bonjour");

    rest = parse_command(strcpy(input, "a\\foo\\t\\nbaz\\r\\v\\fbar"), &command);
    cr_expect_str_empty(rest);
    cr_expect_eq(command.id, 'a');
    cr_expect_str_eq(command.data.text, "foo\t\nbaz\r\v\fbar");
}

Test(parse_command, escapable_text_escape_error_no_backslash, .exit_code = 1)
{
    parse_command(strcpy(input, "a foo"), &command);
}

Test(parse_command, addresses_max_1_comment, .exit_code = 1)
{
    parse_command("1#bonjour", &command);
}

Test(parse_command, addresses_max_2_comment, .exit_code = 1)
{
    parse_command("1,10#bonjour", &command);
}

Test(parse_command, addresses_max_1_label, .exit_code = 1)
{
    parse_command("1:bonjour", &command);
}

Test(parse_command, addresses_max_2_label, .exit_code = 1)
{
    parse_command("1,10:bonjour", &command);
}

Test(parse_command, addresses_max_2_quit, .exit_code = 1)
{
    parse_command("1,10q", &command);
}

Test(parse_command, list)
{
    rest = parse_command("{p}", &command);
    cr_expect_str_empty(rest);
    cr_expect_eq(command.id, '{');
    cr_expect_eq(command.data.children[0].id, 'p');
    cr_expect_eq(command.data.children[1].id, '}');
    free(command.data.children);

    rest = parse_command("{p;p;p;p;p;p;p;p;p;p;p;p;p;p;p;p;p;p;p;p}", &command);
    cr_expect_str_empty(rest);
    cr_expect_eq(command.id, '{');
    for (size_t i = 0; i < 20; i++)
        cr_expect_eq(command.data.children[i].id, 'p');
    cr_expect_eq(command.data.children[20].id, '}');
    free(command.data.children);

    rest = parse_command("{10q}", &command);
    cr_expect_str_empty(rest);
    cr_expect_eq(command.id, '{');
    cr_expect_eq(command.data.children[0].id, 'q');
    cr_expect_eq(command.data.children[0].addresses.count, 1);
    cr_expect_eq(command.data.children[0].addresses.addresses[0].type, ADDRESS_LINE);
    cr_expect_eq(command.data.children[0].addresses.addresses[0].data.line, 10);
    cr_expect_eq(command.data.children[1].id, '}');
    free(command.data.children);

    rest = parse_command("{10,20p}", &command);
    cr_expect_str_empty(rest);
    cr_expect_eq(command.id, '{');
    cr_expect_eq(command.data.children[0].id, 'p');
    cr_expect_eq(command.data.children[0].addresses.count, 2);
    cr_expect_eq(command.data.children[0].addresses.addresses[0].type, ADDRESS_LINE);
    cr_expect_eq(command.data.children[0].addresses.addresses[0].data.line, 10);
    cr_expect_eq(command.data.children[0].addresses.addresses[1].type, ADDRESS_LINE);
    cr_expect_eq(command.data.children[0].addresses.addresses[1].data.line, 20);
    cr_expect_eq(command.data.children[1].id, '}');
    free(command.data.children);

    rest = parse_command(strcpy(input, "{rfoo\nwbar\n}"), &command);
    cr_expect_str_empty(rest);
    cr_expect_eq(command.id, '{');
    cr_expect_eq(command.data.children[0].id, 'r');
    cr_expect_str_eq(command.data.children[0].data.text, "foo");
    cr_expect_eq(command.data.children[1].id, 'w');
    cr_expect_str_eq(command.data.children[1].data.text, "bar");
    cr_expect_eq(command.data.children[2].id, '}');
    free(command.data.children);

    rest = parse_command(strcpy(input, "{a\\ bonjour\n}"), &command);
    cr_expect_str_empty(rest);
    cr_expect_eq(command.id, '{');
    cr_expect_eq(command.data.children[0].id, 'a');
    cr_expect_str_eq(command.data.children[0].data.text, "bonjour");
    cr_expect_eq(command.data.children[1].id, '}');
    free(command.data.children);

    rest = parse_command(strcpy(input, "{}"), &command);
    cr_expect_str_empty(rest);
    cr_expect_eq(command.id, '{');
    cr_expect_eq(command.data.children[0].id, '}');
    free(command.data.children);

    rest = parse_command(strcpy(input, "{;;;;; ;;;;;;;;;}"), &command);
    cr_expect_str_empty(rest);
    cr_expect_eq(command.id, '{');
    cr_expect_eq(command.data.children[0].id, '}');
    free(command.data.children);

    rest = parse_command(strcpy(input, "{;;; ;;;;p;;;;;; ;}"), &command);
    cr_expect_str_empty(rest);
    cr_expect_eq(command.id, '{');
    cr_expect_eq(command.data.children[0].id, 'p');
    cr_expect_eq(command.data.children[1].id, '}');
    free(command.data.children);

    rest = parse_command(strcpy(input, "{\n\n\n\n\n\n\n \n\n\n}"), &command);
    cr_expect_str_empty(rest);
    cr_expect_eq(command.id, '{');
    cr_expect_eq(command.data.children[0].id, '}');
    free(command.data.children);

    rest = parse_command(strcpy(input, "{\n\n\n \n\np\n\n\n\n\n\n}"), &command);
    cr_expect_str_empty(rest);
    cr_expect_eq(command.id, '{');
    cr_expect_eq(command.data.children[0].id, 'p');
    cr_expect_eq(command.data.children[1].id, '}');
    free(command.data.children);

    rest = parse_command(strcpy(input, "{\n\n;\n;\n\np\n\n;\n\n\n;\n}"), &command);
    cr_expect_str_empty(rest);
    cr_expect_eq(command.id, '{');
    cr_expect_eq(command.data.children[0].id, 'p');
    cr_expect_eq(command.data.children[1].id, '}');
    free(command.data.children);

    rest = parse_command(strcpy(input, "{;;;{;;{;;p;;};;};;;}"), &command);
    cr_expect_str_empty(rest);
    cr_expect_eq(command.id, '{');
    cr_expect_eq(command.data.children[0].id, '{');
    cr_expect_eq(command.data.children[0].data.children[0].id, '{');
    cr_expect_eq(command.data.children[0].data.children[0].data.children[0].id, 'p');
    cr_expect_eq(command.data.children[0].data.children[0].data.children[1].id, '}');
    cr_expect_eq(command.data.children[0].data.children[1].id, '}');
    cr_expect_eq(command.data.children[1].id, '}');
    free(command.data.children);

    rest = parse_command(strcpy(input, "{;p;;{;;p;;};;{p;p;p};;}"), &command);
    cr_expect_str_empty(rest);
    cr_expect_eq(command.id, '{');
    cr_expect_eq(command.data.children[0].id, 'p');
    cr_expect_eq(command.data.children[1].id, '{');
    cr_expect_eq(command.data.children[1].data.children[0].id, 'p');
    cr_expect_eq(command.data.children[1].data.children[1].id, '}');
    cr_expect_eq(command.data.children[2].id, '{');
    cr_expect_eq(command.data.children[2].data.children[0].id, 'p');
    cr_expect_eq(command.data.children[2].data.children[1].id, 'p');
    cr_expect_eq(command.data.children[2].data.children[2].id, 'p');
    cr_expect_eq(command.data.children[2].data.children[3].id, '}');
    cr_expect_eq(command.data.children[3].id, '}');

    const size_t len = 2048;
    const size_t buf_size = len * 2 + 2;
    char         buf[buf_size + 1];
    memset(buf, '\0', buf_size + 1);
    buf[0] = '{';
    size_t i;
    for (i = 1; i < buf_size - 1;)
    {
        buf[i++] = 'p';
        buf[i++] = ';';
    }
    buf[i - 1] = '}';
    buf[i] = '\0';
    rest = parse_command(buf, &command);
    cr_expect_str_empty(rest);
    cr_expect_eq(command.id, '{');
    for (size_t i = 0; i < len - 1; i++)
        cr_expect_eq(command.data.children[i].id, 'p');
    cr_expect_eq(command.data.children[len].id, '}');
    free(command.data.children);
}

Test(parse_command, error_list_extra_characters, .exit_code = 1)
{
    parse_command("{p p}", &command);
}

Test(parse_command, error_list_unmatched, .exit_code = 1)
{
    parse_command("{", &command);
}

Test(parse_command, error_list_unmatched_nested, .exit_code = 1)
{
    parse_command("{;p;p;{p;p;p};p;p", &command);
}

Test(parse_command, translate)
{
    rest = parse_command(strcpy(input, "y/abc/def/"), &command);
    cr_expect_str_empty(rest);
    cr_expect_eq(command.id, 'y');
    cr_expect_str_eq(command.data.translate.from, "abc");
    cr_expect_str_eq(command.data.translate.to, "def");

    rest = parse_command(strcpy(input, "y/\\/\\a\\b\\c/\\d\\ef\\//"), &command);
    cr_expect_str_empty(rest);
    cr_expect_eq(command.id, 'y');
    cr_expect_str_eq(command.data.translate.from, "/abc");
    cr_expect_str_eq(command.data.translate.to, "def/");

    rest =
        parse_command(strcpy(input, "y/\\t\\n\\r\\v\\f/\\f\\v\\r\\n\\t/"), &command);
    cr_expect_str_empty(rest);
    cr_expect_eq(command.id, 'y');
    cr_expect_str_eq(command.data.translate.from, "\t\n\r\v\f");
    cr_expect_str_eq(command.data.translate.to, "\f\v\r\n\t");

    rest = parse_command(strcpy(input, "y|\\|abc|def\\||"), &command);
    cr_expect_str_empty(rest);
    cr_expect_eq(command.id, 'y');
    cr_expect_str_eq(command.data.translate.from, "|abc");
    cr_expect_str_eq(command.data.translate.to, "def|");
}

Test(parse_command, translate_error_from_smaller, .exit_code = 1)
{
    rest = parse_command(strcpy(input, "y/abc/defg/"), &command);
}

Test(parse_command, translate_error_to_smaller, .exit_code = 1)
{
    rest = parse_command(strcpy(input, "y/abcd/efg/"), &command);
}

Test(parse_command, substitute)
{
    rest = parse_command(strcpy(input, "s/abc*/def/"), &command);
    cr_expect_str_empty(rest);
    cr_expect_eq(command.id, 's');
    cr_expect_str_eq(command.data.substitute.replacement, "def");
    cr_expect_eq(regexec(&command.data.substitute.preg, "abc", 0, NULL, 0), 0);
    cr_expect_eq(regexec(&command.data.substitute.preg, "abcccc", 0, NULL, 0), 0);
    cr_expect_eq(regexec(&command.data.substitute.preg, "bccc", 0, NULL, 0),
                 REG_NOMATCH);

    rest = parse_command(strcpy(input,
                                "s/"
                                "\\(1\\)\\(2\\)\\(3\\)\\(4\\)\\(5\\)\\(6\\)\\(7\\)"
                                "\\(8\\)\\(9\\)/\\&\\1\\2\\3\\4\\5\\6\\7\\8\\9\\0/"),
                         &command);
    cr_expect_str_empty(rest);
    cr_expect_eq(command.id, 's');
    cr_expect_str_eq(command.data.substitute.replacement,
                     "\\&\\1\\2\\3\\4\\5\\6\\7\\8\\9\\0");

    rest = parse_command(strcpy(input, "s/x/\\a\\t\\n\\r\\v\\f\\o/"), &command);
    cr_expect_str_empty(rest);
    cr_expect_eq(command.id, 's');
    cr_expect_str_eq(command.data.substitute.replacement, "a\t\n\r\v\fo");

    rest = parse_command(strcpy(input, "s/a\\t*/def/"), &command);
    cr_expect_str_empty(rest);
    cr_expect_eq(command.id, 's');
    cr_expect_str_eq(command.data.substitute.replacement, "def");
    cr_expect_eq(regexec(&command.data.substitute.preg, "a\t", 0, NULL, 0), 0);
    cr_expect_eq(regexec(&command.data.substitute.preg, "a\t\t\t\t", 0, NULL, 0), 0);
    cr_expect_eq(regexec(&command.data.substitute.preg, "\t\t\t", 0, NULL, 0),
                 REG_NOMATCH);

    rest = parse_command(strcpy(input, "s_\\_abc*_def\\__"), &command);
    cr_expect_str_empty(rest);
    cr_expect_eq(command.id, 's');
    cr_expect_str_eq(command.data.substitute.replacement, "def_");
    cr_expect_eq(regexec(&command.data.substitute.preg, "_abc", 0, NULL, 0), 0);
    cr_expect_eq(regexec(&command.data.substitute.preg, "_abcccc", 0, NULL, 0), 0);
    cr_expect_eq(regexec(&command.data.substitute.preg, "abccc", 0, NULL, 0),
                 REG_NOMATCH);

    // delimiter can be a space
    rest = parse_command(strcpy(input, "s woo boing "), &command);
}

Test(parse_command, substitute_flags)
{
    rest = parse_command(strcpy(input, "s/abc*/def/p"), &command);
    cr_expect_str_empty(rest);
    cr_expect_eq(command.id, 's');
    cr_expect(command.data.substitute.print);
    cr_expect(!command.data.substitute.global);
    cr_expect_null(command.data.substitute.write_filepath);
    cr_expect_eq(command.data.substitute.occurence_index, 0);

    rest = parse_command(strcpy(input, "s/abc*/def/g"), &command);
    cr_expect_str_empty(rest);
    cr_expect_eq(command.id, 's');
    cr_expect(!command.data.substitute.print);
    cr_expect(command.data.substitute.global);
    cr_expect_null(command.data.substitute.write_filepath);
    cr_expect_eq(command.data.substitute.occurence_index, 0);

    rest = parse_command(strcpy(input, "s/abc*/def/w bonjour"), &command);
    cr_expect_str_empty(rest);
    cr_expect_eq(command.id, 's');
    cr_expect(!command.data.substitute.print);
    cr_expect(!command.data.substitute.global);
    cr_expect_str_eq(command.data.substitute.write_filepath, "bonjour");
    cr_expect_eq(command.data.substitute.occurence_index, 0);

    rest = parse_command(strcpy(input, "s/abc*/def/42"), &command);
    cr_expect_str_empty(rest);
    cr_expect_eq(command.id, 's');
    cr_expect(!command.data.substitute.print);
    cr_expect(!command.data.substitute.global);
    cr_expect_null(command.data.substitute.write_filepath);
    cr_expect_eq(command.data.substitute.occurence_index, 42);

    rest = parse_command(strcpy(input, "s/abc*/def/pg99w bonjour"), &command);
    cr_expect_str_empty(rest);
    cr_expect_eq(command.id, 's');
    cr_expect(command.data.substitute.print);
    cr_expect(command.data.substitute.global);
    cr_expect_str_eq(command.data.substitute.write_filepath, "bonjour");
    cr_expect_eq(command.data.substitute.occurence_index, 99);
}

Test(parse_command, substitute_flags_error_multiple_print, .exit_code = 1)
{
    parse_command(strcpy(input, "s/abc*/def/pg99pw bonjour"), &command);
}

Test(parse_command, substitute_flags_error_multiple_global, .exit_code = 1)
{
    parse_command(strcpy(input, "s/abc*/def/pg99gw bonjour"), &command);
}

Test(parse_command, substitute_flags_error_multiple_occurence_index, .exit_code = 1)
{
    parse_command(strcpy(input, "s/abc*/def/32pg99w bonjour"), &command);
}

Test(parse_command, substitute_flags_error_occurence_index_zero, .exit_code = 1)
{
    parse_command(strcpy(input, "s/abc*/def/pg0w bonjour"), &command);
}

Test(parse_command, substitute_flags_error_occurence_index_overflow, .exit_code = 1)
{
    parse_command(strcpy(input,
                         "s/abc*/def/"
                         "pg99999999999999999999999999999999999999999999999999999999"
                         "99999"
                         "99999999999999999999999w bonjour"),
                  &command);
}

Test(parse_command, substitute_group_index_error, .exit_code = 1)
{
    parse_command(strcpy(input, "s/\\(ab\\)c*/\\2/"), &command);
}

Test(parse_command, substitute_group_index_none_error, .exit_code = 1)
{
    parse_command(strcpy(input, "s/abc*/\\1/"), &command);
}

Test(parse_command, substitute_flags_error_unknown_flag, .exit_code = 1)
{
    parse_command(strcpy(input, "s/abc*/def/p42bg bonjour"), &command);
}

Test(parse, base)
{
    struct command *commands;
    commands = parse("p");
    cr_expect_eq(commands[0].id, 'p');
    cr_expect_eq(commands[1].id, COMMAND_LAST);
    free(commands);

    commands = parse("p;p;p;p;p;p;p;p;p;p;p;p;p;p;p;p;p;p;p;p");
    for (size_t i = 0; i < 20; i++)
        cr_expect_eq(commands[i].id, 'p');
    cr_expect_eq(commands[20].id, COMMAND_LAST);
    free(commands);

    commands = parse("10q");
    cr_expect_eq(commands[0].id, 'q');
    cr_expect_eq(commands[0].addresses.count, 1);
    cr_expect_eq(commands[0].addresses.addresses[0].type, ADDRESS_LINE);
    cr_expect_eq(commands[0].addresses.addresses[0].data.line, 10);
    cr_expect_eq(commands[1].id, COMMAND_LAST);
    free(commands);

    commands = parse("10,20p");
    cr_expect_eq(commands[0].id, 'p');
    cr_expect_eq(commands[0].addresses.count, 2);
    cr_expect_eq(commands[0].addresses.addresses[0].type, ADDRESS_LINE);
    cr_expect_eq(commands[0].addresses.addresses[0].data.line, 10);
    cr_expect_eq(commands[0].addresses.addresses[1].type, ADDRESS_LINE);
    cr_expect_eq(commands[0].addresses.addresses[1].data.line, 20);
    cr_expect_eq(commands[1].id, COMMAND_LAST);
    free(command.data.children);

    commands = parse(strcpy(input, "rfoo\nwbar\n"));
    cr_expect_eq(commands[0].id, 'r');
    cr_expect_str_eq(commands[0].data.text, "foo");
    cr_expect_eq(commands[1].id, 'w');
    cr_expect_str_eq(commands[1].data.text, "bar");
    cr_expect_eq(commands[2].id, COMMAND_LAST);
    free(commands);

    commands = parse(strcpy(input, "a\\ bonjour\n"));
    cr_expect_eq(commands[0].id, 'a');
    cr_expect_str_eq(commands[0].data.text, "bonjour");
    cr_expect_eq(commands[1].id, COMMAND_LAST);
    free(commands);

    commands = parse(strcpy(input, ""));
    cr_expect_eq(commands[0].id, COMMAND_LAST);
    free(commands);

    commands = parse(strcpy(input, ";;;;;;;; ;;;;;;"));
    cr_expect_eq(commands[0].id, COMMAND_LAST);
    free(commands);

    commands = parse(strcpy(input, ";;;; ;;;p; ;;;;;;"));
    cr_expect_eq(commands[0].id, 'p');
    cr_expect_eq(commands[1].id, COMMAND_LAST);
    free(commands);

    commands = parse(strcpy(input, "\n\n \n\n\n\n\n \n\n\n"));
    cr_expect_eq(commands[0].id, COMMAND_LAST);
    free(commands);

    commands = parse(strcpy(input, "\n\n\n\n\np\n\n\n\n\n\n"));
    cr_expect_eq(commands[0].id, 'p');
    cr_expect_eq(commands[1].id, COMMAND_LAST);
    free(commands);

    commands = parse(strcpy(input, "\n\n;\n; \n\np\n\n;\n\n\n;\n"));
    cr_expect_eq(commands[0].id, 'p');
    cr_expect_eq(commands[1].id, COMMAND_LAST);
    free(commands);

    commands = parse(strcpy(input, ";;;{;;{;;p;;};;};;;"));
    cr_expect_eq(commands[0].id, '{');
    cr_expect_eq(commands[0].data.children[0].id, '{');
    cr_expect_eq(commands[0].data.children[0].data.children[0].id, 'p');
    cr_expect_eq(commands[0].data.children[0].data.children[1].id, '}');
    cr_expect_eq(commands[0].data.children[1].id, '}');
    cr_expect_eq(commands[1].id, COMMAND_LAST);
    free(commands);

    commands = parse(strcpy(input, ";p;;{;;p;;};;{p;p;p};;"));
    cr_expect_eq(commands[0].id, 'p');
    cr_expect_eq(commands[1].id, '{');
    cr_expect_eq(commands[1].data.children[0].id, 'p');
    cr_expect_eq(commands[1].data.children[1].id, '}');
    cr_expect_eq(commands[2].id, '{');
    cr_expect_eq(commands[2].data.children[0].id, 'p');
    cr_expect_eq(commands[2].data.children[1].id, 'p');
    cr_expect_eq(commands[2].data.children[2].id, 'p');
    cr_expect_eq(commands[2].data.children[3].id, '}');
    cr_expect_eq(commands[3].id, COMMAND_LAST);
}

Test(parse, error_unexpected_closing_brace, .exit_code = 1)
{
    parse("}");
}
