#include "sdb.h"

#define NR_WP 50

typedef struct watchpoint
{
  int NO;
  struct watchpoint *next;
  /* TODO: Add more members if necessary */
  char str[32];
  uint32_t oldval; // TODO:
} WP;

WP *new_wp();
void free_wp(int num);

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;
// head用于组织使用中的监视点结构, free_用于组织空闲的监视点结构
void init_wp_pool()
{
  int i;
  for (i = 0; i < NR_WP; i++)
  {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
    memset(wp_pool[i].str, 0, sizeof(wp_pool[i].str));
  }

  head = (WP *)malloc(sizeof(WP));
  head->NO = -1;
  head->next = NULL;
  free_ = (WP *)malloc(sizeof(WP));
  free_->NO = -1;
  free_->next = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
WP *new_wp()
{
  if (free_->next == NULL)
  {
    printf("no free node available\n");
    assert(0);
  }
  WP *ans = free_->next;
  free_->next = free_->next->next;

  WP *p = head;
  while (p->next != NULL && ans->NO > p->next->NO)
  {
    p = p->next;
  }
  ans->next = p->next;
  p->next = ans;
  return ans;
}

void free_wp(int num)
{
  WP *p = head;
  while (p->next != NULL && p->next->NO != num)
  {
    p = p->next;
  }
  assert(p->next != NULL); // p->next->no==num
  WP *wp = p->next;
  p->next = p->next->next;
  wp->next = free_->next;
  free_->next = wp;
  printf("delete watchpoint %d\n", num);
}
void displayWp()
{
  WP *p = head->next;
  if (p == NULL)
  {
    printf("No watchpoints.\n");
    return;
  }
  printf("Num\twhat\n");
  while (p != NULL)
  {
    printf("%d\t%s\n", p->NO, p->str);
    p = p->next;
  }
}
void setwp(char *args)
{
  WP *ret = new_wp();
  assert(strlen(args) < 32);
  memcpy(ret->str, args, strlen(args));
  bool success = true;
  ret->oldval = expr(ret->str, &success);
  printf("Watchpoint %d : %s\n", ret->NO, ret->str);
}
bool scanwp()
{
  bool changed = false;
  WP *p = head->next;
  while (p != NULL)
  {
    bool success = true;
    uint32_t newval = expr(p->str, &success);
    if (p->oldval != newval)
    {
      printf("\nWatchpoint %d: %s\n", p->NO, p->str);
      printf("old value = %u\n", p->oldval);
      printf("new value = %u\n", newval);
      changed = true;
      p->oldval = newval;
    }
    p = p->next;
  }
  return changed;
}
