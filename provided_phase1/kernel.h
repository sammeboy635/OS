#pragma once

#ifndef KERNEL_H__
#define KERNEL_H__
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
typedef struct nodelist nodelist;
// extern struct nodelist;

typedef struct os_time
{
   long startTime;
   long processTime;
   long totalRunTime;
} os_time;

struct proc_struct
{
   int childJoinCount;
   nodelist *child;
   // int zappedCount;
   nodelist *zapList;
   proc_ptr parent;        // ADDED BY ME
   char name[MAXNAME];     /* process's name */
   char start_arg[MAXARG]; /* args passed to process */
   context state;          /* current context for process */
   short pid;              /* process id */
   int priority;
   int (*start_func)(char *); /* function where process begins -- launch */
   char *stack;
   unsigned int stacksize;
   int status; /* READY, BLOCKED, QUIT, etc. */
   int quitCode;
   /* other fields as needed... */
   os_time time;

   /* Function to handle freeing everything inside the VARIABLE*/
   void (*fn_free)(proc_ptr _self);
   /* Adds a child process to the current process*/
   void (*fn_child_add)(proc_ptr _self, proc_ptr _newChild);
   /* Returns the child cound*/
   int (*fn_child_count)(proc_ptr _self);
   /* Return the next child that join is refering to*/
   proc_ptr (*fn_child_next)(proc_ptr _self);

   /* Adds the process that zapped it and also sets block*/
   void (*fn_zap_add)(proc_ptr _self, proc_ptr _zapProc);
   /* Returns zap count*/
   int (*fn_zap_count)(proc_ptr _self);
   /* Return TRUE OR FALSE if process is zapped*/
   int (*fn_is_zapped)(proc_ptr _self);
   /* Unblocks all zapped process*/
   void (*fn_unblock_zapped)(proc_ptr _self);

   /* Sets the time after the process switches from run to ready*/
   void (*fn_time_end_of_run_set)(proc_ptr _self);
   /* Sets the time for the time when the process starts*/
   void (*fn_time_start_set)(proc_ptr _self);
   /* Determines if the process is ready to be run returns TRUE OR FALSE*/
   int (*fn_time_ready_to_run)(proc_ptr _self);
   /* Determines if the process went over the time allowed return TRUE OR FALSE*/
   int (*fn_time_ready_to_quit)(proc_ptr _self);
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

proc_ptr init_proc_ptr(char *name, int (*f)(char *), char *arg, int stacksize, int priority, unsigned int *next_pid, void(*launch));
void *_free(void *ptr);
void _process_switch(proc_ptr *Current, proc_ptr newProc);
void p_free(proc_ptr _self);

void p_child_add(proc_ptr _self, proc_ptr _newChild);
int p_child_count(proc_ptr _self);
proc_ptr p_child_next(proc_ptr _self);

void p_zap_add(proc_ptr _self, proc_ptr _zapProc);
int p_zap_count(proc_ptr _self);
int p_is_zapped(proc_ptr _self);
void p_unblock_zapped(proc_ptr _self);

void p_time_end_of_run_set(proc_ptr _self);
void p_time_start_set(proc_ptr _self);
int p_time_ready_to_run(proc_ptr _self);
int p_time_ready_to_quit(proc_ptr _self);

/* Some useful constants.  Add more as needed... */
#define NO_CURRENT_PROCESS NULL
#define MINPRIORITY 5
#define MAXPRIORITY 1
#define SENTINELPID 1
#define SENTINELPRIORITY LOWEST_PRIORITY
// More...
#define TRUE 1
#define FALSE 0

/*-----------------------P1.c------------------------*/

typedef struct node
{
   proc_ptr value;
   void *next;
} node;
typedef struct nodelist
{
   node *head;
   node *tail;
   /* Length as in size so if one is in it will be one*/
   int length;
   /* Adds the value to the end of the list*/
   void (*fn_push)(struct nodelist *_list, proc_ptr value);
   /*Pushes a struct node to the end of the list*/
   void (*fn_push_node)(struct nodelist *_list, node *cur);
   /* pops the first proclist*/
   proc_ptr (*fn_pop)(struct nodelist *_list);
   /*Takes the first node and return the value ptr and then pushes to end of list*/
   proc_ptr (*fn_pop_push_end)(struct nodelist *_list);
   /*Get the index of item like a array, returns null if out of index.*/
   proc_ptr (*fn_get_index)(struct nodelist *_list, int _index);
   /*Removes the value from the list*/
   void (*fn_remove_value)(struct nodelist *_list, proc_ptr _value);
   /* Clears all the nodes and frees the values DONOTCALL IN ANY OTHER OTHER THEN MAIN SELF*/
   void (*fn_clear_nodes)(struct nodelist *_list, int clearValuesTF);
   /*clears the node and then free the list DONOTCALL IN ANY OTHER OTHER THEN MAIN SELF*/
   void (*fn_free)(struct nodelist *_list);
   /*Prints the entire nodelist out*/
   void (*fn_dbg_print)(struct nodelist *_list);
} nodelist;

void push_node(nodelist *_list, node *cur);
void push(nodelist *_list, proc_ptr _value);
proc_ptr pop(nodelist *_list);
proc_ptr pop_push_end(nodelist *_list);
proc_ptr fn_get_index(nodelist *_list, int _index);
void remove_value(nodelist *_list, proc_ptr _value);
void clear_nodes(nodelist *_list, int clearValuesTF);
void free_list(nodelist *_list);
void dbg_list_print(nodelist *_list);
nodelist *init_nodelist();

typedef struct proc_list
{
   /*List size of nList*/
   int listSize;
   /*Number of Processes*/
   int processSize;
   /*This is array of List and has a list of the certain priority processes. nList[Priority]*/
   nodelist *nList[(MINPRIORITY + 2)];
   /* Goes through nList to find the perfect process to run*/
   proc_ptr (*fn_dispatcher)(struct proc_list *_self);
   /* frees all the list */
   void (*fn_free)(struct proc_list *_self);
   /*inserts the processing in the correct location*/
   void (*fn_push_proc)(struct proc_list *_self, int _priority, proc_ptr _proc);
   /* Goes through nList and prints it out*/
   void (*fn_remove_proc)(struct proc_list *_self, proc_ptr _proc);
   /*Iterates through all the nlist and checks if there is still processes ready. returns TRUE if there is;*/
   int (*fn_deadlocked)(struct proc_list *_self);
   /* Finds the selected pid from the processlist*/
   proc_ptr (*fn_find_pid)(struct proc_list *_self, int _pid);
   /* Prints memory Address of all the nodelist*/
   void (*fn_dbg_print_nodelist)(struct proc_list *_self);

} proc_list;

void init_proc_list(proc_list *_self);
void pl_free(proc_list *_self);
proc_ptr pl_dispatcher(proc_list *_self);
void pl_push_proc(proc_list *_self, int _priority, proc_ptr _proc);
void pl_remove_proc(proc_list *_self, proc_ptr _proc);
int pl_deadlocked(proc_list *_self);
proc_ptr pl_find_pid(proc_list *_self, int _pid);
void pl_dbg_print_nodelist(proc_list *_self);
//#endif

#endif