#include <isa.h>
#include <string.h>
#include <memory/vaddr.h>
/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_EQ,

  /* TODO: Add more token types */
  TK_HEX, TK_DE, TK_REG, TK_NE, TK_AND,
  TK_NEG, TK_DEREF, TK_POS, 
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},                // spaces
  {"\\+", '+'},                     // plus
  {"==", TK_EQ},                    // equal
  {"0[x|X][0-9A-Fa-f]+", TK_HEX}, 	// hexadecimal-number 
	{"[0-9]+", TK_DE},		            // decimal
	{"\\$[\\$a-z0-9]{2,3}", TK_REG},	// regs' name
  {"==", TK_EQ},                    // ==
  {"!=", TK_NE},                    // !=
  {"&&", TK_AND},                   // &&
  {"-", '-'},                       // substract
  {"/", '/'},                       // divide
  {"\\*", '*'},                     // multiply  
  {"\\(", '('},
  {"\\)", ')'},  
};

#define NR_REGEX ARRLEN(rules)
#define MAX_BUF 31

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
        case TK_NOTYPE:
          // ignore spaces
          break;
        case TK_HEX:
        case TK_DE:
        case TK_REG:
          // 超过buf长度忽略前几位
          if (substr_len > MAX_BUF) {
            substr_start += (substr_len - MAX_BUF);
            substr_len = MAX_BUF;
          }
          strncpy(tokens[nr_token].str, substr_start, substr_len);
          tokens[nr_token].str[substr_len + 1] = '\0';
        default:
          tokens[nr_token++].type = rules[i].token_type;
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

/* 利用栈进行括号匹配 */
int pairs[MAX_BUF];            // 储存括号对
int stack[MAX_BUF], top = -1; // 栈

void make_parentheses(bool *success) {

  for (int i = 0; i < MAX_BUF; ++i) {
    pairs[i] = -1;
  }

  for(int i = 0; i < nr_token; ++i) {
    switch (tokens[i].type) {
    case '(':
      stack[++top] = i;
      break;
    case ')':
      if (top == -1) {
        success = false;
        return;
      }
      pairs[i] = stack[top];
      pairs[stack[top--]] = i;
      break;
    default:
      continue;
    }
  }
  if (top != -1) {
    success = false;
  }
}

bool check_parentheses(int p, int q) {
  return pairs[p] == q;
}

/* 取得token中数字值 */
word_t get_num(int p, bool* success) {
  word_t ret = 0;
  switch (tokens[p].type) {
    case TK_DE:
      for (int i = 0; tokens[p].str[i] != '\0'; ++i) {
        ret = ret * 10 + tokens[p].str[i] - '0';
      }
      return ret;
    case TK_HEX:
      for (int i = 0; tokens[p].str[i] != '\0'; ++i) {
        char c = tokens[p].str[i];
        if (c >= '0' && c <= '9') {
          ret = ret * 16 + c - '0';
        } else if (c >= 'a' && c <= 'f') {
          ret = ret * 16 + c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
          ret = ret * 16 + c - 'A' + 10;
        }
      }
      return ret;
    case TK_REG:
      return isa_reg_str2val(tokens[p].str, success);
  }
  *success = false;
  return 0;
}

/* 判断某个运算符是否为单目运算符 */
bool isUnaryOp(int p) {
  if (p == 0) {
    return true;
  }
  switch (tokens[p - 1].type) {
  case TK_DE:
  case TK_HEX:
  case TK_REG:
  case ')':
    return false;
  }

  return true;
}

/* 得到主操作符位置 */
#define MAX_PRI 3
#define NOT_FOUND 404
int getMainOp(int p, int q) {
  
  int priority[MAX_PRI];
  for (int i = 0; i < MAX_PRI; ++i) {
    priority[i] = NOT_FOUND;
  } 

  int index = p;
  while (index <= q) {
    switch (tokens[index].type) {

    case '(':
      // 跳过括号
      index = pairs[index] + 1;
      break;
    case TK_AND:
      // 优先级最低
      return p;
    case TK_EQ: case TK_NE:
      // 优先级次低
      if (priority[0] == NOT_FOUND) {
        priority[0] = index;
      }
      ++index;
      break;
    case '+': case '-':
      if (priority[1] == NOT_FOUND) {
        priority[1] = index;
      }
      ++index;
      break;
    case '*': case '/':
      // 优先级最高
      if (priority[2] == NOT_FOUND) {
        priority[2] = index;
      }
      ++index;
      break;
	default: 
      ++index; 
      break;
    }               
  }

  for(int i = 0; i < MAX_PRI; ++i) {
    if (priority[i] != NOT_FOUND) {
      return priority[i];
    }
  }

  return NOT_FOUND;
}

word_t eval(int p, int q, bool* success) {
  if (p > q) {

    *success = false;
    return 0;

  } else if (p == q) {

    return get_num(p, success);

  } else if (check_parentheses(p, q)) {

    return eval(p + 1, q - 1, success);

  } else if (getMainOp(p, q) != NOT_FOUND) {
    
    int mainPos = getMainOp(p, q);
    word_t left = eval(p, mainPos - 1, success);
    word_t right = eval(mainPos + 1, q, success);

    if (*success == false) {
      return 0;
    }

    switch (tokens[mainPos].type) {
      case '+': return left + right;
      case '-': return left - right;
      case '*': return left * right;
      case '/': return left / right;
      case TK_EQ: return left == right;
      case TK_NE: return left != right;
      case TK_AND: return left && right;
	  default: assert(0);				   
    }

    *success = false;
    return 0;

  } else if (tokens[p].type == TK_NEG) {

    return -eval(p + 1, q, success);

  } else if (tokens[p].type == TK_POS) {
    
    return eval(p + 1, q, success);

  } else if (tokens[p].type == TK_DEREF) {

    word_t addr = eval(p + 1, q, success);
    
    if (*success == false) {
      return 0;
    }
    return  vaddr_read(addr, 1);

  }

  *success = false;
  return 0;
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  make_parentheses(success);

  if (!*success) {
    printf("Wrong expression!\n");
    return 0;
  }

  // 预处理，提前发现单目运算符
  for (int i = 0; i < nr_token; ++i) {
    if (tokens[i].type == '*' && isUnaryOp(i)) {
      tokens[i].type = TK_DEREF;
    } else if (tokens[i].type == '-' && isUnaryOp(i)) {
      tokens[i].type = TK_NEG;
    } else if (tokens[i].type == '+' && isUnaryOp(i)) {
      tokens[i].type = TK_POS;
    }
  }
  
  word_t ret = eval(0, nr_token - 1, success);
  if (*success == true) {
    return ret;
  }
  return 0;
}
