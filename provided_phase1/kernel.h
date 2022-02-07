#pragma once
//#ifndef KERNEL_H__
//#define KERNEL_H__
#define DEBUG 0

typedef enum Status
{
   RUNNING,
   READY,
   BLOCKED,
   QUIT
} Status;

typedef struct proc_struct proc_struct;

typedef struct proc_struct *proc_ptr;
// extern struct nodelist;

struct proc_struct
{
   proc_ptr next_proc_ptr;    // OR THIS
   proc_ptr child_proc_ptr;   // TEACHER ask is we can make this a list.
   proc_ptr parent_proc_ptr;  // ADDED BY ME
   proc_ptr next_sibling_ptr; // WHY DO WE NEED THIS
   char name[MAXNAME];        /* process's name */
   char start_arg[MAXARG];    /* args passed to process */
   context state;             /* current context for process */
   short pid;                 /* process id */
   int priority;
   int (*start_func)(char *); /* function where process begins -- launch */
   char *stack;
   unsigned int stacksize;
   int status; /* READY, BLOCKED, QUIT, etc. */
   int quitCode;
   /* other fields as needed... */
   int runningTime;
   int zapped;
};

struct psr_bits
{
   unsigned int cur_mode : 1;
   unsigned int cur_int_enable : 1;
   unsigned int prev_mode : 1;
   unsigned int prev_int_enable : 1;
   unsigned int unused : 28;
};

union psr_values
{
   struct psr_bits bits;
   unsigned int integer_part;
};

/* Some useful constants.  Add more as needed... */
#define NO_CURRENT_PROCESS NULL
#define MINPRIORITY 5
#define MAXPRIORITY 1
#define SENTINELPID 1
#define SENTINELPRIORITY LOWEST_PRIORITY
// More...
#define TRUE 1
#define FALSE -1

//#endif