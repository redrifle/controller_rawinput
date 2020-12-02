#ifndef UTIL_H
#define UTIL_H

#ifndef NDEBUG
#define cr_printf cr_printf_debug
#endif /* NDEBUG */

void* cr_safe_malloc(size_t);
void cr_safe_free(void**);
void cr_printf(const char*, ...);

typedef struct cr_state_struct
{
    int x;
} cr_state;

#endif /* UTIL_H */