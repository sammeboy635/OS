#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <phase1.h>
#include "kernel.h"

void p1_fork(int pid)
{
}

void p1_switch(int olds, int news)
{
}

void p1_quit(int pid)
{
}

void push_node(nodelist *_list, node *cur)
{
    cur->next = NULL;

    if (_list->head == NULL)
    {
        _list->head = cur;
        _list->tail = cur;
        _list->length = 1;
        return;
    }

    _list->tail->next = cur;
    _list->tail = cur;
    _list->length++;
    return;
}
void push(nodelist *_list, proc_ptr _value)
{

    node *newNode = (node *)malloc(sizeof(node));
    newNode->value = _value;
    push_node(_list, newNode);
}
proc_ptr pop(nodelist *_list)
{
    if (_list->head == NULL)
    {
        return NULL;
    }
    return _list->head->value;
}
proc_ptr pop_push_end(nodelist *_list)
{
    node *tmp;
    if (_list->head == NULL)
        return NULL;

    tmp = _list->head;
    _list->head = _list->head->next;
    _list->length--;

    push_node(_list, tmp);

    return tmp->value;
}
proc_ptr get_index(nodelist *_list, int _index)
{
    if (_list->length < _index)
    {
        return NULL;
    }

    int index = 0;
    for (node *cur = _list->head; cur != NULL; cur = cur->next)
    {
        if (_index == index)
        {
            return cur->value;
        }
        index++;
    }
    printf("GET_INDEX: REACHED END ERROR");
    return NULL;
}
void remove_value(nodelist *_list, proc_ptr _value)
{
    node *last = _list->head;
    node *cur = last;
    for (; (cur->value != _value) & (cur->next != NULL); cur = cur->next)
    {
        last = cur;
    }
    if (cur->value == _value)
    {
        if (_list->head == cur && _list->tail == cur)
        {
            _list->head = NULL;
            _list->tail = NULL;
        }
        else if (_list->head == cur)
        {
            _list->head = _list->head->next;
        }
        else if (_list->tail == cur)
        {
            _list->tail = last;
            last->next = cur->next;
        }
        else
        {
            last->next = cur->next;
        }

        _list->length--;
        _free(cur->value);
        _free(cur);
    }
}
void clear_nodes(nodelist *_list, int clearValuesTF)
{
    node *last;
    node *cur = _list->head;
    while (cur != NULL)
    {
        last = cur;
        cur = cur->next;
        if (clearValuesTF == TRUE)
            last->value->fn_free(last->value);
        _free(last);
    }
}
void free_list(nodelist *_list)
{
    _list->fn_clear_nodes(_list, TRUE);
    _free(_list);
}
void dbg_list_print(nodelist *_list)
{
    const char *STATUSNAME[] = {"RUNNING", "READY", "BLOCKED", "QUIT"};
    int i = 0;
    for (node *cur = _list->head; cur != NULL; cur = cur->next)
    {
        printf("%d %10d %20s %10s\n", cur->value->pid, cur->value->priority, STATUSNAME[cur->value->status], cur->value->name); //
        i++;
    }
}

nodelist *init_nodelist()
{
    nodelist *list = (nodelist *)malloc(sizeof(nodelist));
    list->head = NULL;
    list->tail = NULL;
    list->length = 0;
    list->fn_push = push;
    list->fn_push_node = push_node;
    list->fn_pop = pop;
    list->fn_pop_push_end = pop_push_end;
    list->fn_clear_nodes = clear_nodes;
    list->fn_free = free_list;
    list->fn_remove_value = remove_value;
    list->fn_dbg_print = dbg_list_print;
    list->fn_get_index = get_index;
    return list;
}

void init_proc_list(proc_list *_self)
{
    _self->processSize = 0;
    _self->listSize = MINPRIORITY + 2;
    for (int i = 0; i < _self->listSize; i++)
        _self->nList[i] = init_nodelist();

    _self->fn_free = pl_free;
    _self->fn_dispatcher = pl_dispatcher;
    _self->fn_push_proc = pl_push_proc;
    _self->fn_dbg_print_nodelist = pl_dbg_print_nodelist;
    _self->fn_deadlocked = pl_deadlocked;
    _self->fn_find_pid = pl_find_pid;
}
void pl_free(proc_list *_self)
{
    printf("freeing ProcessLIST!!\n");
    for (int i = 0; i < _self->listSize - 1; i++)
    {
        _self->nList[i]->fn_free(_self->nList[i]);
    }
}

proc_ptr pl_dispatcher(proc_list *_self)
{
    for (int i = 0; i < _self->listSize; i++)
    {
        if (_self->nList[i]->length == 0) // Continue if there is none in this priority
            continue;

        for (int x = 0; x < _self->nList[i]->length; x++) // for length of processes search for a Ready one
        {
            proc_ptr selectProcess = _self->nList[i]->fn_pop_push_end(_self->nList[i]); // Pop process off
            if (selectProcess->status == READY)
                return selectProcess;
        }
    }
    printf("PL_DISPATCHER: MAJOR ERROR SHOULD NOT REACH");
    exit(-1);
}
void pl_push_proc(proc_list *_self, int _priority, proc_ptr _proc)
{
    _self->nList[_priority]->fn_push(_self->nList[_priority], _proc);
    _self->processSize++;
    return;
}
void pl_remove_proc(proc_list *_self, proc_ptr _proc)
{
    _self->nList[_proc->priority]->fn_remove_value(_self->nList[_proc->priority], _proc);
    _self->processSize--;
    return;
}
int pl_deadlocked(proc_list *_self)
{
    for (int i = 0; i < (_self->listSize - 2); i++) // NEED -1 to exclude sentinel process.
    {
        for (int x = 0; x < _self->nList[i]->length; x++) // for length of processes search for a Ready one
        {
            proc_ptr selectProcess = _self->nList[i]->fn_get_index(_self->nList[i], x); // Pop process off
            if (selectProcess->status == READY)
                return FALSE;
        }
    }
    return TRUE;
}
proc_ptr pl_find_pid(proc_list *_self, int _pid)
{
    for (int i = 0; i < (_self->listSize - 1); i++)
    {
        for (int x = 0; x < _self->nList[i]->length; x++) // for length of processes search for a Ready one
        {
            proc_ptr selectProcess = _self->nList[i]->fn_get_index(_self->nList[i], x); // Pop process off
            if (selectProcess->pid == _pid)
                return selectProcess;
        }
    }
    return NULL;
}
void pl_dbg_print_nodelist(proc_list *_self)
{
    printf("%s %12s %15s %10s", "PID", "PRIORITY", "STATUS", "NAME\n");
    for (int i = 0; i < _self->listSize; i++)
    {
        // printf("%d)\n", i);
        _self->nList[i]->fn_dbg_print(_self->nList[i]);
    }
}
