#include <stdio.h>
#include <stdarg.h>
#include <Windows.h>
#include "util.h"

void cr_safe_free(void** p)
{
	if (p && *p)
	{
		free(*p);
		*p = NULL;
	}
}

void* cr_safe_malloc(size_t n)
{
	errno = 0;
	void* p = malloc(n);

	if (!p)
	{
		perror(NULL);
		cr_printf("Could not allocate %zu bytes", n);
		return NULL;
	}

	return p;
}

#ifdef NDEBUG
void cr_printf(const char *format, ...)
{
	if (!format)
		return;

	va_list vargs;
	va_start(vargs, format);
	vfprintf(stderr, format, vargs);
	va_end(vargs);
}
#endif /* NDEBUG */

void cr_printf_debug(const char* format, ...)
{
	if (!format)
		return;

	va_list vargs;
	va_start(vargs, format);
	size_t n = vsnprintf(NULL, 0, format, vargs);

	if (!n)
		return;

	char *s = cr_safe_malloc(n);
	vsnprintf(s, n, format, vargs);
	va_end(vargs);

	OutputDebugString((LPCSTR)s);

	cr_safe_free(&s);
}
