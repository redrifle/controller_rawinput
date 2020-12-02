#include <Windows.h>
#include <hidsdi.h>
#include "util.h"

int detect_hid_devices()
{

    return 1;
}

int cr_initialize_hid()
{
    int rv;

    rv = detect_hid_devices();

    if (!rv)
    {
        cr_printf("Failed to detect HID devices");
        return 0;
    }

    return 1;
}