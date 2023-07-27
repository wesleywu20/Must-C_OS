#include <unistd.h>
#include <stdio.h>

int main(void)
{
    const int size = 100;
    char buf[size + 1];

    int nread = read(STDIN_FILENO, buf, 0);
    printf("%d", nread);
    printf("%c", buf[0]);
    write(STDOUT_FILENO, buf, nread);
    return 0;
}