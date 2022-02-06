
#include "kernel.h"

#ifndef __P1_H__
#define __P1_H__

typedef struct node
{
    proc_ptr value;
    void *next;
} node;
typedef struct nodelist
{
    node *head;
    node *tail;
    int length;
    /* Adds the value to the end of the list*/
    void (*fn_push)(struct nodelist *_list, proc_ptr value);
    /*Pushes a struct node to the end of the list*/
    void (*fn_push_node)(struct nodelist *_list, node *cur);
    /*Takes the first node and return the value ptr and then pushes to end of list*/
    proc_ptr (*fn_pop_push_end)(struct nodelist *_list);
    /*Removes the value from the list*/
    void (*fn_remove_value)(struct nodelist *_list, proc_ptr _value);
    void (*fn_clear_nodes)(struct nodelist *_list);
    /*clears the node and then free the list*/
    void (*fn_free_list)(struct nodelist *_list);
    /*Prints the entire nodelist out*/
    void (*fn_dbg_print)(struct nodelist *_list);
} nodelist;

void push_node(nodelist *_list, node *cur);
void push(nodelist *_list, proc_ptr _value);
proc_ptr pop_push_end(nodelist *_list);
void remove_value(nodelist *_list, proc_ptr _value);
void clear_nodes(nodelist *_list);
void free_list(nodelist *_list);
void dbg_list_print(nodelist *_list);
nodelist *init_nodelist();

typedef struct proc_list
{
    /*List size of nList*/
    uint listSize;
    /*Number of Processes*/
    uint processSize;
    /*This is array of List and has a list of the certain priority processes. nList[Priority]*/
    nodelist *nList[(MINPRIORITY + 2)];
    /* Goes through nList to find the perfect process to run*/
    proc_ptr (*fn_dispatcher)(struct proc_list *_self);
    /*inserts the processing in the correct location*/
    void (*fn_push_proc)(struct proc_list *_self, int _priority, proc_ptr _proc);
    /* Goes through nList and prints it out*/
    void (*fn_remove_proc)(struct proc_list *_self, proc_ptr _proc);
    void (*fn_dbg_print_nodelist)(struct proc_list *_self);

} proc_list;

void init_proc_list(proc_list *_self);
proc_ptr pl_dispatcher(proc_list *_self);
void pl_push_proc(proc_list *_self, int _priority, proc_ptr _proc);
void pl_remove_proc(proc_list *_self, proc_ptr _proc);
void pl_dbg_print_nodelist(proc_list *_self);
#endif