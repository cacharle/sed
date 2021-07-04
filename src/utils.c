#include "sed.h"

static void *xmalloc(size_t size)
{
	void *ret = malloc(size);
	if (ret == NULL)
		exit(1);
	return ret;
}

static char *xstrdup(const char *s)
{
	char *ret = strdup(s);
	if (ret == NULL)
		exit(1);
	return ret;
}

/**
** \param origin  base string to free
** \param ...     any number of strings to concat to origin, must end with NULL
*/
char *strjoinf(char *origin, ...)
{
	if (origin == NULL)
		origin = xstrdup("");
	va_list ap;
	va_list ap_clone;
	va_start(ap, origin);
	va_copy(ap_clone, ap);
	size_t length_sum = strlen(origin);
	while (true)
	{
		char *s = va_arg(ap_clone, char*);
		if (s == NULL)
			break;
		length_sum += strlen(s);
	}
	va_end(ap_clone);
	char *new = xmalloc((length_sum + 1) * sizeof(char));
	strcpy(new, origin);
	while (true)
	{
		char *s = va_arg(ap, char*);
		if (s == NULL)
			break;
		strcat(new, s);
	}
	free(origin);
	va_end(ap);
	return new;
}

#define READ_FILE_BUF_SIZE 1024

char *read_file(char *filepath)
{
	char buf[READ_FILE_BUF_SIZE + 1] = {'\0'};
	char *ret = NULL;
	FILE *file = fopen(filepath, "r");
	if (file == NULL)
		exit(1);
	while (!feof(file))
	{
		fread(buf, sizeof(char), READ_FILE_BUF_SIZE, file);
		ret = strjoinf(ret, buf, NULL);
	}
	return ret;
}