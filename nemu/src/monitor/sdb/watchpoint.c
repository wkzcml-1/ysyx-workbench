#include "sdb.h"
#include "common.h"

#define NR_WP 32
#define MAX_BUF 1024

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  
  char str[MAX_BUF];  // 存储表达式
  word_t old_val;

} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

// head's tail
static WP *tail = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

void new_up(char *arg, bool* success) {
  if (free_ == NULL) {
    // no available resource
    assert(0);
  }

  word_t val = expr(arg, success);
  if (*success == false) {
    printf("Expression error!\n");
    return;
  }

  WP* ret = free_;
  ret->old_val = val;
  strcpy(ret->str, arg);
  free_ = free_->next;
  ret->next = NULL;

  if (head == NULL) {
    head = tail = ret;
  } else {
    tail->next = ret;
    tail = ret;
  }

  printf("Watchpoint %d:\t%s\n", tail->NO, tail->str);

}

// 此处做一个修改，使用监视点序号标识监视点
void free_wp(int no) {
  /* 遍历使用中的监视点，寻找到标号为no的监视点与其前驱 */

  // head 为空 直接返回
  if (head == NULL) return;

  WP* to_free = NULL;

  // head即为待释放监视点
  if (head->NO == no) {
    to_free = head;
    head = head->next;
    if (head == NULL) {
      tail = NULL;
    }
  } else {
    
    // 待释放节点位于链表中间
    WP* pre = head;
    WP* p = head->next;
   
    while (p) {
      if (p->NO == no) {
        to_free = p;
        break;
      }
      p = p->next;
    }

    // 从head链表中删除
    if (p) {
      pre->next = p->next;
      if (tail == p) {
        tail = pre;
      }
    } else {
      return;
    }
  }
  
  // free_为空
  if (free_ == NULL) {
    free_ = to_free;
    free_->next = NULL;
  } else {
    // 头插法
    to_free->next = free_->next;
    free_->next = to_free;
  }

}

bool update_WP_state() {

  WP* p = head;
  bool state = true;

  while (p) {

    bool success = true;
    word_t new_val = expr(p->str, &success);

    // 表达式计算错误，程序直接退出
    if (success == false) {
      printf("Expression error!\n");
      break;
    }

    if (new_val != p->old_val) {
      state = false;
      printf("Watchpoint %d:\t%s\n", p->NO, p->str);
      printf("Old value = "FMT_WORD"\n", p->old_val);
      printf("New value = "FMT_WORD"\n\n", p->old_val = new_val);
    }

    p = p->next;
  }

  return state;

}

void print_WP_info() {
  
  if (head == NULL) {
    printf("No watchpoint now.\n");
    return;
  }

  printf("Num\tNowVal\t\t\tWhat\n");
  
  for (WP* p = head; p; p = p->next) {
    printf("%d\t"FMT_WORD"\t%s\n", p->NO, p->old_val, p->str);
  }

}