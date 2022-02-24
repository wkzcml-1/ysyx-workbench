#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#define MAX_TOKEN_BUFF 31

enum {
  TK_NOTYPE = 256, TK_EQ,

  /* TODO: Add more token types */
  TK_DE, TK_HEX,
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"==", TK_EQ},        // equal
  {"0[x|X][0-9A-Fa-f]+", TK_HEX}, 
                        // hexadecimal number
  {"[0-9]+", TK_DE},    // decimal
  {"-", '-'},           // subtract
  {"\\*", '*'},         // multiply
  {"/", '/'},           // divide
  {"\\(", '('},         // left parenthesis
  {"\\)", ')'},         // right parenthesis
};

#define NR_REGEX ARRLEN(rules)

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
            break;                // ignore spaces
          case TK_DE: case TK_HEX:
            int ignoreLen = 0;    // exceed the buff limit
            
            if(substr_len > MAX_TOKEN_BUFF) {
              ignoreLen = substr_len - MAX_TOKEN_BUFF;
            }

            int strIndex = 0;
            int index = position + ignoreLen - substr_len;
            while(index < position) {
              tokens[nr_token].str[strIndex++] = e[index++];
            }
            tokens[nr_token].str[strIndex] = '\0';

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

/* get parentheses pairs  */
int stack[32], top = -1;
const int NOT_PAIR = -2;
int parentheses_pair[32];

void search_parentheses( bool *success ) {
  //initial
  top = -1;
  // search 
  for(int i = 0; i < nr_token; ++i) {
    if (tokens[i].type == '(') {
      stack[++top] = i;
    } else if (tokens[i].type == ')') {
      parentheses_pair[i] = stack[top];
      parentheses_pair[stack[top--]] = i;
    } else {
      parentheses_pair[i] = NOT_PAIR;
    }
  }
  // judge
  if(top > -1) *success = false;
}

bool check_parentheses( int p, int q ) {
  return q == parentheses_pair[p];
}

int get_main_op (int p, int q) {
  
  int ret = -1, pos = p;

  while ( pos <= q ) {
    switch ( tokens[pos].type ) {
      case '+': case '-':
        return pos;
      case '*': case '/':
        ret = ret == -1? pos++: -1;
        break;
      case '(':
        pos = parentheses_pair[p] + 1;
        break;
      default: ++pos;
    }
  }

  return ret;
}

int minus_times = 0;

word_t get_number(int p) {
  word_t ret = 0;
  switch (tokens[p].type) {
  case TK_DE:
    for(int i = 0; tokens[p].str[i] != '\0'; ++i) {
      ret = ret * 10 + tokens[p].str[i] - '0';
    }
    break;
  
  case TK_HEX:
    for(int i = 2; tokens[p].str[i] != '\0'; ++i) {
      char c = tokens[p].str[i];
      if (c >= '0' && c <= '9') {
        ret = ret * 16 + c - '0';
      } else if (c >= 'a' && c <= 'f') {
        ret = ret * 16 + c - 'a' + 10;
      } else {
        ret = ret * 16 + c - 'A' + 10;
      }
    }
    break;
  }
  return ret;
} 

word_t eval(int p, int q, bool *success) {
  if ( p > q ) {
    // error
    *success = false;
    return 0;

  } else if ( p == q ) {
    // get a num
    word_t ret = get_number(p);
    
    if (minus_times > 0) {
      if (minus_times % 2) {
        minus_times = 0;
        return -ret;
      }
      minus_times = 0;
    }

    return ret;
  } else if ( check_parentheses(p, q) ) {
    // surrounded by a matched pair of parentheses.
    return eval(p + 1, q - 1, success);
  } else if (tokens[p].type == '-') {
    ++minus_times;
    return eval(p + 1, q, success);
  }
  
  int op_pos = get_main_op(p, q);
  
  if(op_pos == -1) {
    *success = false;
    return 0;
  }
  
  word_t left = eval(p, op_pos - 1, success);
  word_t right = eval(op_pos + 1, q, success);

  if (minus_times > 0) {
    if (minus_times % 2) {
      left = -left;
    }
    minus_times = 0;
  }

  if(*success == false) {
    return 0;
  }

  switch ( tokens[op_pos].type ) {
    case '+': return left + right;
    case '-': return left - right;
    case '*': return left * right;
    case '/':
      if(right == 0) {
        *success = false;
        return 0;
      } else {
        return left / right;
      }
  }

  return 0;
}

word_t expr(char *e, bool *success) {
  
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  search_parentheses( success );
  
  if( *success  == false ) return 0;
  
  return eval(0, nr_token - 1, success); 

}
