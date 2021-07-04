#include "sed.h"

static const char *available_commands = "ais";

char *parse_address(char *s, struct address *address)
{
	if (strchr(available_commands, s[0]) != NULL)
		return s;
	if (s[0] == '$')
	{
		address->type = ADDRESS_LAST;
		return s + 1;
	}
	if (isdigit(s[0]))
	{
		address->type = ADDRESS_LINE;
		address->line = strtoul(s, &s, 10);
		return s;
	}
	char delim = s[0];
	char *end = s + 1;
	while (*end != delim && *end != '\0')
	{
		if (end[0] == '\\' && end[1] == delim)
			memmove(end, end + 1, strlen(end));
		end++;
	}
	*end = '\0';
	address->type = ADDRESS_RE;
	if (regcomp(&address->preg, s, 0) != 0)
		exit(1);
	return end;
}

char *parse_addresses(char *s, struct addresses *addresses)
{
	char *end;
	addresses->count = 0;
	end = parse_address(s, &addresses->addresses[0]);
	if (s == end)
		return s;
	s = end;
	addresses->count++;
	if (*s != ',')
		return s;
	end = parse_address(s, &addresses->addresses[1]);
	if (s == end)
		return s;
	addresses->count++;
	return s;
}

char *parse_append(char *s, struct command *command)
{
	while (isblank(*s))
		s++;
	command->text = s;
	while (*s != '\0' && *s != '\n')
	{
		if (*s == '\\' && *s != '\0')
			s++;
		s++;
	}
	*s = '\0';
	return s + 1;
}

typedef char *(*parse_command_function)(char*, struct command*);

static parse_command_function parse_command_function_lookup[] = {
	['a'] = parse_append,
};

static char *parse_command(char *s, struct command *command)
{
	s = parse_addresses(s, &command->addresses);
	command->inverse = *s == '!';
	if (*s == '!')
		s++;
	command->id = *s;
	s++;
	return parse_command_function_lookup[command->id](s, command);
}

struct command *parse_script(char *)
{
	return NULL;
}
