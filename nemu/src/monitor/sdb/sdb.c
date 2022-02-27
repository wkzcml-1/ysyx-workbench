#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"

#include <memory/vaddr.h>

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  nemu_state.state = NEMU_QUIT;
  return -1;
}

static int cmd_help(char *args);

// si
static int cmd_si(char *args);

// info
static int cmd_info(char *args);

// x
static int cmd_x(char *args);

// p
static int cmd_p(char *args);

// w
static int cmd_w(char *args);

// d
static int cmd_d(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },

  /* TODO: Add more commands */
  { "si", "Pause execution after single-stepping N instructions, "
    "when N is not given, the default is 1", cmd_si },
  { "info", "[r]: display the value of regs\n""[w]: display"
    " the information of watchpoints", cmd_info },
  { "x", "Starting from the starting memory address, output consecutive "
    "N 4-bytes in hexadecimal form", cmd_x },
  { "p", "Get the value of the expression", cmd_p },
  { "w", "Set a watchpoint based on the value of the expression", cmd_w },
  { "d", "Delete watchpoint N", cmd_d }, 
};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

// si
uint64_t str2num( const char *args ) {
  uint64_t ans = 0;
  // iterate over the string
  for(int i = 0; args[i] != '\0'; ++i) {
    unsigned num = args[i] - '0';
    if(num >= 0 && num <= 9) {
      // 0 ~ 9
      ans = ans * 10 + num;
    } else if(args[i] == ' ') {
      // ignore space
      continue;
    } else {
      // not 0 ~ 9
      return 0;
    }
  }
  return ans;
}

static int cmd_si(char *args) {
  // get token
  char *arg = strtok(NULL, " ");

  if (arg == NULL) {
    // no argument
    cpu_exec(1);
  } else {
    // execute # times
    cpu_exec(str2num(arg));
  }

  return 0;
} // si end

// info
static int cmd_info( char *args ) {
  // get token
  char *arg = strtok(NULL, " ");
  
  if (arg == NULL) {
    // no argument
    printf("Please provide an argument!\n");

  } else if (strcmp(arg, "r") == 0) {
    // display regs' information
    isa_reg_display();
  
  } else if (strcmp(arg, "w") == 0) {
    // display watchpoints' information
    print_WP_info();

  } else {
    // argument error
    printf("Could not find relevant information.\n");

  }

  return 0;
} // info end


// x: scan memory
static int cmd_x(char *args) {

  // get N and expr
  char* num = strtok(NULL, " ");
  char* e = num + strlen(num) + 1;

  int len = atoi(num);
  bool success = true;
  word_t addr = expr(e, &success);

  if (!success) {
    printf("Please give a correct address!\n");
    return 0;
  }

  for (int i = 0; i < len; ++i) {
    printf(FMT_WORD":\t"FMT_WORD"\n", addr, vaddr_read(addr, 4));
    addr += 4;
  }

  return 0;
} // x

// p: print expression
static int cmd_p(char *args) {

  bool success = true;
  word_t val = expr(args, &success);

  if (success) {
    printf("%s :\t"FMT_WORD"\n", args, val);
  } else {
    printf("Expression error!\n");
    return -1;
  }

  return 0;
}

// w: set watchpoint
static int cmd_w(char *args) {
  
  bool success = true;

  new_up(args, &success);

  if (success == false) {
    printf("Fail to set watchpoint!\n");
    return -1;
  } 
  
  return 0;

}

// d: delete watchpoint N
static int cmd_d(char *args) {
  
  int num = atoi(args);

  free_wp(num);

  return 0;

}


void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
