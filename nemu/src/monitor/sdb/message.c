
#include "message.h"
#include <stdio.h>

char *message[] = {
    "argument number is wrong",
    "Unknown argument:",
    "no argument is input",
    "brackets don't match!",
    "something is wrong with expression",
};

void printMessage(Message m, char *args)
{
    printf("%s", message[m]);
    if (args)
    {
        printf("%s", args);
    }
    printf("\n");
}