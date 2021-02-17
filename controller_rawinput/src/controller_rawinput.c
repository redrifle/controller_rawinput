#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <wchar.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <CommCtrl.h>
#include <hidsdi.h>
#include <SetupAPI.h>
#include "util.h"
#include "error.h"

HWND trackbar_global;
HANDLE hid_device_global;

HANDLE cr_open_device(const char *path)
{
    HANDLE hid_device = CreateFile(
        path,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, 0, 0);

    if (hid_device == INVALID_HANDLE_VALUE)
    {
        cr_print_error(GetLastError());
        return NULL;
    }

    return hid_device;
}

int cr_set_force_feedback(HANDLE hid_device, uint8_t value)
{
    uint8_t right_motor = value, left_motor = 0, buf[32];
    memset(buf, 0, sizeof(buf));

    buf[0] = 0x05;
    buf[1] = 0xFF;
    buf[4] = right_motor;  // 0-255
    buf[5] = left_motor;   // 0-255

    DWORD bytes_written;
    BOOL rv = WriteFile(hid_device, buf, sizeof(buf), &bytes_written, NULL);

    if (!rv)
    {
        cr_print_error(GetLastError());
        return 0;
    }

    cr_printf("Wrote %lu bytes", bytes_written);

    return 1;
}


HWND cr_create_label(HWND hwnd)
{
    HWND label = CreateWindowW(L"Static", L"Hello", WS_CHILD | WS_VISIBLE,
        0, 100, 100, 30, hwnd, (HMENU)3, NULL, NULL);

    return label;
}

HWND cr_create_trackbar(HWND hwnd)
{
    HWND trackbar = CreateWindow(TRACKBAR_CLASS, "Trackbar Control",
        WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_TOOLTIPS |
        TBS_DOWNISLEFT,
        200, 100, 170, 30, hwnd, (HMENU)3, NULL, NULL);

    SendMessage(trackbar, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
    SendMessage(trackbar, TBM_SETPAGESIZE, 0, 5);
    SendMessage(trackbar, TBM_SETTICFREQ, 5, 0);
    SendMessage(trackbar, TBM_SETPOS, FALSE, 0);

    return trackbar;
}

LRESULT CALLBACK cr_window_procedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_CREATE:
            trackbar_global = cr_create_trackbar(hWnd);
            break;

        case WM_DESTROY:
            PostQuitMessage(EXIT_SUCCESS);
            break;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            HBRUSH brush = CreateSolidBrush(RGB(100, 100, 100));
            FillRect(hdc, &ps.rcPaint, brush);
            EndPaint(hWnd, &ps);
            break;
        }

        case WM_NCCREATE:
        {
            CREATESTRUCT* create = (CREATESTRUCT*)lParam;
            cr_state *state = (cr_state*) create->lpCreateParams;
            cr_printf("%d\n\n", state->x);
        }

        case WM_HSCROLL:
        {
            int value;
            int loword = LOWORD(wParam);
            if (loword == SB_THUMBTRACK)
            {
                value = HIWORD(wParam);
                cr_printf("Current slider value: %d\n\n", value);
            }

            if (loword == TB_ENDTRACK)
            {
                value = SendMessage(trackbar_global, TBM_GETPOS, (WPARAM)NULL, (LPARAM)NULL);
                cr_printf("Final slider value: %d\n\n", value);
                //cr_set_force_feedback(hid_device_global, value);
            }
            break;
        }

        case WM_INPUT:
        {
            cr_printf("GETTING INPUT\n\n");
        }

    }

    return DefWindowProc(hWnd, message, wParam, lParam);;
}

ATOM cr_register_window(HINSTANCE hInstance, const char* name)
{
    WNDCLASSEX wc = { 0 };

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = cr_window_procedure;
    wc.hInstance = hInstance;
    wc.lpszClassName = (LPCSTR)name;

    ATOM window_atom = RegisterClassEx(&wc);

    if (!window_atom)
    {
        cr_print_error(GetLastError());
        return 0;
    }

    return window_atom;
}

int cr_create_window(HWND* window_handle, const char* name, const char* title, HINSTANCE* hInstance, LPVOID* state)
{
    *window_handle = CreateWindowEx(
        0, (LPCSTR)name, (LPCSTR)title, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, NULL, NULL, *hInstance, state);

    if (!*window_handle)
    {
        cr_print_error(GetLastError());
        return 0;
    }

    return 1;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    PULONGLONG h; /* fucking lol */
    int rv;

    const char* name = "main_window";
    ATOM window_atom = cr_register_window(hInstance, name);

    if (!window_atom)
        return EXIT_FAILURE;

    cr_state state;

    state.x = 4;

    const char* title = "controller_rawinput";
    HWND window_handle = { 0 };
    rv = cr_create_window(&window_handle, name, title, &hInstance, (LPVOID*)&state);

    if (!rv)
        return EXIT_FAILURE;

    GUID interface_guid;
    HidD_GetHidGuid(&interface_guid);

    HDEVINFO device_enumeration = INVALID_HANDLE_VALUE;
    device_enumeration = SetupDiGetClassDevs(&interface_guid, NULL, NULL, (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));

    if (device_enumeration == INVALID_HANDLE_VALUE)
    {
        cr_print_error(GetLastError());
        return EXIT_FAILURE;
    }

    int index = 0;

    for (int i = 0; ; ++i) /* Enumerate HID devices */
    {
        SP_DEVICE_INTERFACE_DATA device_interface_data;
        device_interface_data.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
        rv = SetupDiEnumDeviceInterfaces(device_enumeration, NULL, &interface_guid, i, &device_interface_data); /* Does the actual enumeration,
                                                                                                                each time we increase index i
                                                                                                                to get the next device */      
        int error;
        if (!rv)
        {
            error = GetLastError();
            if (error == ERROR_NO_MORE_ITEMS) /* If there are no more devices, break */
                break;
            else                              /* Otherwise, we have a legit error */
            {
                cr_print_error(GetLastError());
                return EXIT_FAILURE;
            }
        }

        DWORD required_size = 0;
        SetupDiGetDeviceInterfaceDetail(device_enumeration, &device_interface_data, NULL, 0, &required_size, NULL); /* Probing call only to get buffer size,
                                                                                                                    so error code (122) ignored */
        /* Allocate new device detail struct using buffer size we obtained */
        DWORD determined_size = required_size;
        PSP_DEVICE_INTERFACE_DETAIL_DATA device_interface_detail_data = cr_safe_malloc(required_size);
        device_interface_detail_data->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        /* Get detailed info about device */
        rv = SetupDiGetDeviceInterfaceDetail(device_enumeration, &device_interface_data, device_interface_detail_data, determined_size, &required_size, NULL);

        if (!rv)
        {
            cr_print_error(GetLastError());
            return EXIT_FAILURE;
        }
        
        HANDLE current_device = cr_open_device(device_interface_detail_data->DevicePath);

        /* If access is denied, device is probably a mouse/keyboard
        which are not allowed to be accessed by HID clients,
        therefore we skip them */
        if (!current_device)
        {
            if (GetLastError() == ERROR_ACCESS_DENIED)
                goto Done;

            cr_print_error(GetLastError());
            return EXIT_FAILURE;
        }

        cr_printf("OPENED DEVICE: %s\n", device_interface_detail_data->DevicePath);

        PHIDP_PREPARSED_DATA preparsed_data = NULL;
        rv = HidD_GetPreparsedData(current_device, &preparsed_data);

        if (!rv)
        {
            cr_print_error(GetLastError());
            return EXIT_FAILURE;
        }

        WCHAR product_string[128];
        product_string[127] = '\0';
        rv = HidD_GetProductString(current_device, product_string, sizeof(product_string));

        if (!rv)
        {
            cr_print_error(GetLastError());
            return EXIT_FAILURE;
        }

        cr_printf("\t(%S)\n\n", product_string);

        if (wcsncmp(product_string, L"Controller", 10) == 0) /* Good, we found a controller :\ don't blame me blame microsoft */
            index = i;

        HidD_FreePreparsedData(preparsed_data);
        DeleteFile(device_interface_detail_data->DevicePath);
        current_device = NULL;
    Done:
        cr_safe_free(&device_interface_detail_data);
    }

    SetupDiDestroyDeviceInfoList(device_enumeration);

    ShowWindow(window_handle, nCmdShow);

    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return EXIT_SUCCESS;
}