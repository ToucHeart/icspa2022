typedef enum Message
{
    ARG_NUM_WRONG,
    Unknown,
    NULL_ARG,
    BRACKET,
    WRONG_EXPR,
} Message;

void printMessage(Message m, char *args);
