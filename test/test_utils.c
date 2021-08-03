#include "sed.h"
#include <criterion/criterion.h>

Test(strjoinf, base)
{
	char *s;
	s = strjoinf(NULL);
	cr_assert_str_empty(s);
}
