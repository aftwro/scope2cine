#include <stdio.h>
#include <gnu/libc-version.h>

int main(void)
{
    printf("hello from mercury6, glibc %s\n", gnu_get_libc_version());
    return 0;
}
