/***************************************************************************************
 * Copyright (c) 2014-2022 Zihao Yu, Nanjing University
 *
 * NEMU is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *
 * See the Mulan PSL v2 for more details.
 ***************************************************************************************/

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <memory/paddr.h>
#include "message.h"
#include "sdb.h"
#include <assert.h>

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();
void setwp(char *args);
void displayWp();
void free_wp(int num);
/* We use the `readline' library to provide more flexibility to read from stdin. */
static char *rl_gets()
{
  static char *line_read = NULL;

  if (line_read)
  {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read)
  {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args)
{
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args)
{
  return -1;
}

static int get_arg_num(char *args)
{
  if (args == NULL)
    return 0;
  int num = 1;
  strtok(args, " ");
  while (strtok(NULL, " "))
  {
    num += 1;
  }
  return num;
}

static int cmd_info(char *args)
{
  int arg_num = get_arg_num(args);
  if (arg_num == 0)
  {
    printMessage(NULL_ARG, NULL);
    return 0;
  }
  if (arg_num >= 2)
  {
    printMessage(ARG_NUM_WRONG, NULL);
    return 0;
  }
  if (*args == 'r') // print register
  {
    isa_reg_display();
  }
  // TODO:
  else if (*args == 'w') // print watchpoint
  {
    displayWp();
  }
  else // unknown command
  {
    printMessage(Unknown, args);
  }
  return 0;
}
static int cmd_d(char *args)
{
  int num = atoi(args);
  free_wp(num);
  return 0;
}
static int cmd_si(char *args)
{
  if (get_arg_num(args) == 0)
    cpu_exec(1);
  else
  {
    if (get_arg_num(args) >= 2)
    {
      printMessage(ARG_NUM_WRONG, NULL);
      return 0;
    }
    int steps = atoi(args);
    steps == 0 ? printMessage(Unknown, args) : cpu_exec(steps);
  }
  return 0;
}
static int cmd_test(char *args)
{
  FILE *fp = fopen("/home/mxj/ics2021/nemu/tools/gen-expr/build/input.txt", "r");
  if (!fp)
  {
    perror("Failed to open input file");
    return 0;
  }
  char exprbuf[10000] = {0};
  while (fgets(exprbuf, sizeof(exprbuf), fp) != NULL)
  {
    word_t filenum;
    sscanf(exprbuf, "%u", &filenum);
#ifdef DEBUG
    printf("filenum is: %u\n", filenum);
#endif
    char *pos = strchr(exprbuf, ' ');
    if (pos == NULL)
    {
      printf("can't find \' \'\n");
      return 0;
    }
    *(pos + strlen(pos + 1)) = '\0'; // set '\n' to '\0';
#if 0
    printf("expr is: %s\n", pos + 1);
#endif
    bool success = true;
    word_t evalnum = expr(pos + 1, &success);
    if (success)
    {
      if (evalnum != filenum)
        printf("evalnum: %u != filenum: %u\n", evalnum, filenum);
      else
        printf("evalnum: %u == filenum: %u\n", evalnum, filenum);
    }
    else
      printMessage(WRONG_EXPR, NULL);
    memset(exprbuf, 0, sizeof(exprbuf));
  }
  fclose(fp);
  return 0;
}
static int cmd_p(char *args)
{
  if (args == NULL)
  {
    printMessage(NULL_ARG, NULL);
    return 0;
  }
  bool success = true;
  word_t ans = expr(args, &success);
  if (success)
  {
    printf("value is: %#.8x(hex)\t%d(dec)\n", ans, ans);
  }
  else
  {
    printMessage(WRONG_EXPR, NULL);
  }
  return 0;
}

static int cmd_x(char *args)
{
  if (get_arg_num(args) < 2)
  {
    printMessage(ARG_NUM_WRONG, NULL);
    return 0;
  }
  int num = atoi(args);
  bool success = true;
  uint32_t vaddr = expr(args + strlen(args) + 1, &success);
  if (!success)
  {
    printf("something wrong with argument %s\n", args + strlen(args) + 1);
    return 0;
  }
  for (int i = 0; i < num; i++)
  {
    if (i % 4 == 0)
      printf("%#x: ", vaddr + i * 4);
    uint32_t val = getnum(vaddr + i * 4);
    printf("%#.8x\t", val);
    if ((i + 1) % 4 == 0)
      printf("\n");
  }
  if (num % 4 != 0)
    printf("\n");
  return 0;
}
static int cmd_w(char *args)
{
  if (get_arg_num(args) == 0)
  {
    printMessage(ARG_NUM_WRONG, NULL);
    return 0;
  }
  setwp(args);
  return 0;
}
static int cmd_help(char *args);

static struct
{
  const char *name;
  const char *description;
  int (*handler)(char *);
} cmd_table[] =
    {
        {"help", "Display informations about all supported commands", cmd_help},
        {"c", "Continue the execution of the program", cmd_c},
        {"q", "Exit NEMU", cmd_q},
        {"si", "step into for n", cmd_si},
        {"info", "print program state,r:print register; w:print watchpoint", cmd_info},
        {"x", "scan the memory", cmd_x},
        {"p", "evaluate expression", cmd_p},
        {"test", "test_calculation", cmd_test},
        {"w", "set watchpoint", cmd_w},
        {"d", "delete watchpoint", cmd_d},
        /* TODO: Add more commands */
};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args)
{
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL)
  {
    /* no argument given */
    for (i = 0; i < NR_CMD; i++)
    {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else
  {
    for (i = 0; i < NR_CMD; i++)
    {
      if (strcmp(arg, cmd_table[i].name) == 0)
      {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode()
{
  is_batch_mode = true;
}

void sdb_mainloop()
{
  if (is_batch_mode)
  {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL;)
  {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL)
    {
      continue;
    }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end)
    {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i++)
    {
      if (strcmp(cmd, cmd_table[i].name) == 0)
      {
        if (cmd_table[i].handler(args) < 0)
        {
          return;
        }
        break;
      }
    }

    if (i == NR_CMD)
    {
      printf("Unknown command '%s'\n", cmd);
    }
  }
}

void init_sdb()
{
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
