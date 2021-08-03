#include "sed.h"
#include <criterion/criterion.h>

static struct address   address;
static struct addresses addresses;
static struct command   command;
static char *           rest;
static char             input[64];

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
    cr_assert_str_eq("foo", parse_address("$foo", &address));
    cr_assert_str_eq("foo", parse_address("1000foo", &address));
    cr_assert_str_eq("foo", parse_address(strcpy(input, "/a\\/bc*/foo"), &address));
}

Test(parse_addresses, none)
{
    rest = parse_addresses("a bonjour", &addresses);
    cr_assert_str_eq("a bonjour", rest);
    cr_assert_eq(addresses.count, 0);
}

Test(parse_addresses, end)
{
    rest = parse_addresses("$", &addresses);
    cr_assert_str_empty(rest);
    cr_assert_eq(addresses.count, 1);
    cr_assert_eq(addresses.addresses[0].type, ADDRESS_LAST);

    rest = parse_addresses("$,$", &addresses);
    cr_assert_str_empty(rest);
    cr_assert_eq(addresses.count, 2);
    cr_assert_eq(addresses.addresses[0].type, ADDRESS_LAST);
    cr_assert_eq(addresses.addresses[1].type, ADDRESS_LAST);
}

Test(parse_addresses, line)
{
    rest = parse_addresses("10", &addresses);
    cr_assert_str_empty(rest);
    cr_assert_eq(addresses.count, 1);
    cr_assert_eq(addresses.addresses[0].type, ADDRESS_LINE);

    rest = parse_addresses("10,400", &addresses);
    cr_assert_str_empty(rest);
    cr_assert_eq(addresses.count, 2);
    cr_assert_eq(addresses.addresses[0].type, ADDRESS_LINE);
    cr_assert_eq(addresses.addresses[1].type, ADDRESS_LINE);
}

Test(parse_addresses, re)
{
    rest = parse_addresses(strcpy(input, "/abc*/"), &addresses);
    cr_assert_str_empty(rest);
    cr_assert_eq(addresses.count, 1);
    cr_assert_eq(addresses.addresses[0].type, ADDRESS_RE);

    rest = parse_addresses(strcpy(input, "/abc*/,|.*cba|"), &addresses);
    cr_assert_str_empty(rest);
    cr_assert_eq(addresses.count, 2);
    cr_assert_eq(addresses.addresses[0].type, ADDRESS_RE);
    cr_assert_eq(addresses.addresses[1].type, ADDRESS_RE);
}

Test(parse_addresses, output)
{
    cr_assert_str_eq("foo", parse_addresses("$foo", &addresses));
    cr_assert_str_eq("foo", parse_addresses("$,$foo", &addresses));
    cr_assert_str_eq("foo", parse_addresses("10foo", &addresses));
    cr_assert_str_eq("foo", parse_addresses("10,400foo", &addresses));
    cr_assert_str_eq("foo", parse_addresses(strcpy(input, "/abc*/foo"), &addresses));
    cr_assert_str_eq("foo",
                     parse_addresses(strcpy(input, "/abc*/,|.*cba|foo"), &addresses));
}

Test(parse_command, inverse)
{
    rest = parse_command("!p", &command);
    cr_assert_str_empty(rest);
    cr_assert_eq(command.id, 'p');
    cr_assert(command.inverse);
}

Test(parse_command, singleton)
{
    const char *singleton_commands = "dDgGhHlnNpPqx=#";
    for (size_t i = 0; singleton_commands[i] != '\0'; i++)
    {
        input[0] = singleton_commands[i];
        input[1] = '\0';
        rest     = parse_command(input, &command);
        cr_assert_str_empty(rest);
        cr_assert_eq(command.id, singleton_commands[i]);
    }
}

Test(parse_command, text)
{
    const char *text_commands = ":btrw";
    for (size_t i = 0; text_commands[i] != '\0'; i++)
    {
        input[0] = text_commands[i];
        strcpy(input + 1, "bonjour");
        rest = parse_command(input, &command);
        cr_assert_str_empty(rest);
        cr_assert_eq(command.id, text_commands[i]);
        cr_assert_str_eq(command.data.text, "bonjour");
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
        cr_assert_str_empty(rest);
        cr_assert_eq(command.id, text_commands[i]);
        cr_assert_str_eq(command.data.text, "bonjour");
    }
}

Test(parse_command, escapable_text_escape)
{
    rest = parse_command(strcpy(input, "a\\\\b\\on\\j\\o\\ur"), &command);
    cr_assert_str_empty(rest);
    cr_assert_eq(command.id, 'a');
    cr_assert_str_eq(command.data.text, "bonjour");

    rest = parse_command(strcpy(input, "a\\foo\\t\\nbaz\\r\\v\\fbar"), &command);
    cr_assert_str_empty(rest);
    cr_assert_eq(command.id, 'a');
    cr_assert_str_eq(command.data.text, "foo\t\nbaz\r\v\fbar");
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
    cr_assert_str_empty(rest);
    cr_assert_eq(command.id, '{');
    cr_assert_eq(command.data.children[0].id, 'p');
    cr_assert_eq(command.data.children[1].id, COMMAND_LAST);
    free(command.data.children);

    rest = parse_command("{p;p;p;p;p;p;p;p;p;p}", &command);
    cr_assert_str_empty(rest);
    cr_assert_eq(command.id, '{');
    for (size_t i = 0; i < 10; i++)
        cr_assert_eq(command.data.children[i].id, 'p');
    cr_assert_eq(command.data.children[10].id, COMMAND_LAST);
    free(command.data.children);

    rest = parse_command("{10q}", &command);
    cr_assert_str_empty(rest);
    cr_assert_eq(command.id, '{');
    cr_assert_eq(command.data.children[0].id, 'q');
    cr_assert_eq(command.data.children[0].addresses.count, 1);
    cr_assert_eq(command.data.children[0].addresses.addresses[0].type, ADDRESS_LINE);
    cr_assert_eq(command.data.children[0].addresses.addresses[0].data.line, 10);
    cr_assert_eq(command.data.children[1].id, COMMAND_LAST);
    free(command.data.children);

    rest = parse_command("{10,20p}", &command);
    cr_assert_str_empty(rest);
    cr_assert_eq(command.id, '{');
    cr_assert_eq(command.data.children[0].id, 'p');
    cr_assert_eq(command.data.children[0].addresses.count, 2);
    cr_assert_eq(command.data.children[0].addresses.addresses[0].type, ADDRESS_LINE);
    cr_assert_eq(command.data.children[0].addresses.addresses[0].data.line, 10);
    cr_assert_eq(command.data.children[0].addresses.addresses[1].type, ADDRESS_LINE);
    cr_assert_eq(command.data.children[0].addresses.addresses[1].data.line, 20);
    cr_assert_eq(command.data.children[1].id, COMMAND_LAST);
    free(command.data.children);

    rest = parse_command(strcpy(input, "{rfoo\nwbar\n}"), &command);
    cr_assert_str_empty(rest);
    cr_assert_eq(command.id, '{');
    cr_assert_eq(command.data.children[0].id, 'r');
    cr_assert_str_eq(command.data.children[0].data.text, "foo");
    cr_assert_eq(command.data.children[1].id, 'w');
    cr_assert_str_eq(command.data.children[1].data.text, "bar");
    cr_assert_eq(command.data.children[2].id, COMMAND_LAST);
    free(command.data.children);

    rest = parse_command(strcpy(input, "{a\\ bonjour\n}"), &command);
    cr_assert_str_empty(rest);
    cr_assert_eq(command.id, '{');
    cr_assert_eq(command.data.children[0].id, 'a');
    cr_assert_str_eq(command.data.children[0].data.text, "bonjour");
    cr_assert_eq(command.data.children[1].id, COMMAND_LAST);
    free(command.data.children);
}
