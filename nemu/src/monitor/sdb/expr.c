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

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <string.h>
#include <assert.h>
#include "message.h"
#include <memory/paddr.h>
enum
{
  TK_NOTYPE = 256,
  TK_EQ,
  TK_NEQ,
  AND,
  REG,
  DREF,
  MINUS,
  /* TODO: Add more token types */

};

static struct rule
{
  const char *regex;
  int token_type;
} rules[] = {

    /* TODO: Add more rules.
     * Pay attention to the precedence level of different rules.
     */

    {"\\*", '*'},                 // mul,DREF
    {"/", '/'},                   // division
    {"\\+", '+'},                 // plus
    {"-", '-'},                   // minus,subtract
    {"==", TK_EQ},                // equal
    {"!=", TK_NEQ},               //!=
    {"&&", AND},                  // and
    {"0[xX][0-9a-f]{1,8}", 'h'},  // hexadecimal
    {"[$][a-z]{1,2}[0-9]*", REG}, // register
    {"[0-9]+", 'd'},              // decimal
    {"\\(", '('},                 //(
    {"\\)", ')'},                 //)
    {" +", TK_NOTYPE},            // spaces
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex()
{
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i++)
  {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0)
    {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token
{
  int type;
  char str[32];
} Token;

static Token tokens[8192] __attribute__((used)) = {};
static int nr_token __attribute__((used)) = 0;

static bool make_token(char *e)
{
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0')
  {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i++)
    {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0)
      {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type)
        {
        case TK_NOTYPE:;
          break; // ignore spaces
        case '*':
        {
          // assert(substr_len <= 32);
          if (nr_token == 0 || (tokens[nr_token - 1].type != ')' && tokens[nr_token - 1].type != REG && tokens[nr_token - 1].type != 'h' && tokens[nr_token - 1].type != 'd'))
          {
            tokens[nr_token].type = DREF;
          }
          else
          {
            tokens[nr_token].type = '*';
          }
          memcpy(tokens[nr_token].str, substr_start, substr_len); // record str to tokens.str
          nr_token++;
        }
        break;
        case '-':
        {
          if (nr_token == 0 || (tokens[nr_token - 1].type != ')' && tokens[nr_token - 1].type != REG && tokens[nr_token - 1].type != 'h' && tokens[nr_token - 1].type != 'd'))
          {
            tokens[nr_token].type = MINUS; // minus,-a
          }
          else
          {
            tokens[nr_token].type = '-'; // subtract,a-b
          }
          memcpy(tokens[nr_token].str, substr_start, substr_len); // record str to tokens.str
          nr_token++;
        }
        break;
        case '+':
        case '/':
        case '(':
        case ')':
        case 'd':
        case 'h':
        case AND:
        case TK_NEQ:
        case TK_EQ:
        case REG:
        {
          // assert(substr_len <= 32);
          tokens[nr_token].type = rules[i].token_type;
          memcpy(tokens[nr_token].str, substr_start, substr_len); // record str to tokens.str
          nr_token++;
        }
        break;
        default:
          assert(0);
          break;
        }
        break;
      }
    }

    if (i == NR_REGEX)
    {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}
bool check_match(int p, int q)
{
  int left_bracket_num = 0;
  for (int i = p; i <= q; i++)
  {
    if (tokens[i].type == '(')
      left_bracket_num++;
    else if (tokens[i].type == ')')
    {
      left_bracket_num--;
      if (left_bracket_num < 0)
        return false;
    }
  }
  if (left_bracket_num != 0)
    return false;
  return true;
}

bool check_parentheses(int p, int q)
{
  if (tokens[p].type == '(' && tokens[q].type == ')')
  {
    return check_match(p + 1, q - 1);
  }
  return false;
}
#define MAXPRIORITY (10)

static const int OFFSET = 10;

static int get_priority(int op)
{
  switch (op)
  {
  case MINUS:
    return MAXPRIORITY - (OFFSET - 5);
  case DREF:
    return MAXPRIORITY - (OFFSET - 4);
  case '*':
  case '/':
    return MAXPRIORITY - (OFFSET - 3);
  case '+':
  case '-':
    return MAXPRIORITY - (OFFSET - 2);
  case TK_EQ:
  case TK_NEQ:
    return MAXPRIORITY - (OFFSET - 1);
  case AND:
    return MAXPRIORITY - OFFSET;
  default:
    assert(0);
  }
}
word_t eval(int p, int q, bool *success)
{

#if 0
  printf("eval:  ");
  for (int i = p; i <= q; i++)
  {
    printf("%s", tokens[i].str);
  }
  printf("\n");
#endif

  if (p > q)
  {
    *success = false;
#ifdef DEBUG
    printf("p > q, p = %d,q = %d\n", p, q);
#endif
  }
  else if (p == q && success)
  {
    /* Single token.
     * For now this token should be a number.
     * Return the value of the number.
     */
    {
      switch (tokens[p].type)
      {
      case 'd': // decimal
      {
        if (tokens[p].str[0] == '0')
          return 0;
        word_t num = atoi(tokens[p].str);
        if (num == 0)
        {
          *success = false;
          printf("convert to num error: %s\n", tokens[p].str);
        }
        return num;
      };
      break;
      case 'h': // hexadecimal
      {
        assert(strlen(tokens[p].str) >= 3);
        word_t num;
        if (tokens[p].str[2] == '0')
          return 0;
        if (sscanf(tokens[p].str, "%x", &num) == EOF)
        {
          *success = false;
          printf("convert to num error: %s\n", tokens[p].str);
        }
        return num;
      }
      break;
      case REG: // register
      {
        word_t num = isa_reg_str2val(tokens[p].str + 1, success);
        return num;
      }
      break;
      default:
        *success = false;
        break;
      }
    }
  }
  else if (check_parentheses(p, q) && success)
  {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1, q - 1, success);
  }
  else if (success)
  {
    /*
    找主运算符
    1.非运算符的token不是主运算符.
    2.出现在一对括号中的token不是主运算符. 注意到这里不会出现有括号包围整个表达式的情况, 因为这种情况已经在check_parentheses()相应的if块中被处理了.
    3.主运算符的优先级在表达式中是最低的. 这是因为主运算符是最后一步才进行的运算符.
    4.当有多个运算符的优先级都是最低时, 根据结合性, 最后被结合的运算符才是主运算符. 一个例子是1 + 2 + 3, 它的主运算符应该是右边的+.
    */
    int pos = p;
    int min_priority = MAXPRIORITY;
    int in_bracket = 0;
    for (int i = p; i <= q; ++i)
    {
      switch (tokens[i].type)
      {
      case '*':
      case '/':
      case '+':
      case '-':
      case TK_EQ:
      case TK_NEQ:
      case AND:
      case DREF:
      case MINUS:
      {
        if (in_bracket == 0 && get_priority(tokens[i].type) <= min_priority)
        {
          min_priority = get_priority(tokens[i].type);
          pos = i;
        }
      };
      break;
      case '(':
        in_bracket++;
        break;
      case ')':
        in_bracket--;
        break;
      case 'd':
      case 'h':
      case REG:
        break;
      default:
        assert(0);
        break;
      }
    }

    if (tokens[pos].type == DREF)
    {
      word_t addr = eval(p + 1, q, success);
      return getnum(addr);
    }
    else if (tokens[pos].type == MINUS)
    {
      word_t val = eval(p + 1, q, success);
      return -val;
    }
    else
    {
      word_t val1 = eval(p, pos - 1, success);
      word_t val2 = eval(pos + 1, q, success);

#if 0
    printf("%u %s %u\n", val1, tokens[pos].str, val2);
#endif
      switch (tokens[pos].type)
      {
      case '+':
        return val1 + val2;
      case '-':
        return val1 - val2;
      case '*':
        return val1 * val2;
      case '/':
        assert(val2 != 0);
        return val1 / val2;
      case TK_EQ:
        return val1 == val2;
      case TK_NEQ:
        return val1 != val2;
      case AND:
        return val1 && val2;
      default:
        assert(0);
        break;
      }
    }
  }
  return 0;
}

static void init()
{
  nr_token = 0;
  memset(tokens, 0, sizeof(tokens));
}

word_t expr(char *e, bool *success)
{
  init();
  if (!make_token(e))
  {
    *success = false;
    return 0;
  }
  if (check_match(0, nr_token - 1) == false)
  {
    *success = false;
    printMessage(BRACKET, NULL);
    return 0;
  }
/* TODO: Insert codes to evaluate the expression. */
#if 0
  printf("nr_token is: %d\n", nr_token);
  for (int i = 0; i < nr_token; i++)
  {
    printf("%c  %d", tokens[i].type, tokens[i].type);
    if (tokens[i].str != NULL)
      printf("\t%s\n", tokens[i].str);
  }
#endif
  word_t ans = 0;
  ans = eval(0, nr_token - 1, success);
  return ans;
}