#include <stdlib.h>
#include <stdio.h>
int main()
{
    int ret = system("./gen-expr 5 >./input.txt&&./test.py");
    if (ret != 0)
        printf("Error: system() failed\n");
}