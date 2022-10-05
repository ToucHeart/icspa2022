#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
// this should be enough
// #define DEBUG
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
    "#include <stdio.h>\n"
    "int main() { "
    "  unsigned result = %s; "
    "  printf(\"%%u\", result); "
    "  return 0; "
    "}";

static int buf_num = 0;

static void copy2buf(const char *str)
{
  memcpy(buf + buf_num, str, strlen(str));
  buf_num += strlen(str);
}

static void gen_rand_op()
{
  int num = rand() % 7;
  switch (num)
  {
  case 0:
    copy2buf("+");
    break;
  case 1:
    copy2buf("-");
    break;
  case 2:
    copy2buf("*");
    break;
  case 3:
    copy2buf("/");
    break;
  case 4:
    copy2buf("&&");
    break;
  case 5:
    copy2buf("==");
    break;
  case 6:;
    copy2buf("!=");
    break;
  default:
    assert(0);
    break;
  }
}
static void gen(char c)
{
  buf[buf_num++] = c;
}
static void gen_num()
{
  int num = rand() % 1000;
  char temp[10] = {0};
  sprintf(temp, "%d", num);
  memcpy(buf + buf_num, temp, strlen(temp));
  buf_num += strlen(temp);
#ifdef DEBUG
  printf("rand num is %s\n", temp);
#endif
}

int before = -1;

static void gen_rand_expr()
{
  int num = rand() % 3;
#ifdef DEBUG
  printf("rand case is %d\n", num);
#endif
  switch (num)
  {
  case 0:
    if (before != '(' && before != -1 && before != 'o')
      gen_rand_op();
    {
      const char *a = "(unsigned)";
      memcpy(buf + buf_num, a, strlen(a));
      buf_num += strlen(a);
    }
    gen_num();
    before = 0;
    break;
  case 1:
    // if (buf_num < 20)
    {
      if (before != '(' && before != -1 && before != 'o')
        gen_rand_op();
      gen('(');
      before = '(';
      gen_rand_expr();
      gen(')');
      before = ')';
    }
    break;
  case 2:
    // if (buf_num < 20)
    {
      gen_rand_expr();
      gen_rand_op();
      before = 'o';
      gen_rand_expr();
      before = 2;
    }
    break;
  }
}

void init()
{
  buf_num = 0;
  memset(buf, 0, sizeof(buf));
  memset(code_buf, 0, sizeof(code_buf));
  before = -1;
}

int main(int argc, char *argv[])
{
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1)
  {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i++)
  {
    init();
    gen_rand_expr();

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);
    char string[20] = "/tmp/error";
    sprintf(string + strlen(string), "%d", i);
    char cmd[64] = "gcc /tmp/.code.c -o /tmp/.expr 2> ";
    strcat(cmd, string);
    int ret = system(cmd);
    if (ret != 0)
    {
      printf("system run Error");
      continue;
    }

    FILE *fp2 = fopen(string, "r");
    if (!fp2)
    {
      printf("open failed /tmp/%s\n", string);
      return 0;
    }
    char tmpbuf[256];
    bool division0 = false;
    while (fgets(tmpbuf, 256, fp2) != NULL)
    {
      if (strstr(tmpbuf, "division by zero") != NULL)
      {
        division0 = true;
        break;
      }
    }
    fclose(fp2);
    if (division0)
    {
      // printf("division by zero,pass this\n");
      i -= 1;
      continue;
    }

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    if (fscanf(fp, "%d", &result) == EOF)
      printf("read error from /tmp/.expr\n");
    pclose(fp);

    printf("%u %s\n", result, buf);
  }
  return 0;
}
