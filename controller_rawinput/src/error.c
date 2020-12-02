#include <Windows.h>
#include "util.h"

int cr_print_error(DWORD err)
{
	LPSTR buf = NULL;
	DWORD n = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, err, 0, (LPSTR)&buf, 512, NULL);

	if (!n)
	{
		cr_printf("Unknown error %d\n", err);
		return 0;
	}

	cr_printf("Error %d: %s\n", err, buf);

	return 1;
}