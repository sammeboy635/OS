#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <phase1.h>
#include "kernel.h"
#include "p1.h"

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
        free(cur);
    }
}
void clear_nodes(nodelist *_list)
{
    for (node *cur = _list->head; cur != NULL; cur = cur->next)
    {
        free(cur->value);
        free(cur);
    }
}
void free_list(nodelist *_list)
{
    clear_nodes(_list);
    free(_list);
    printf("freed list!\n");
}
void dbg_list_print(nodelist *_list)
{
    int i = 0;
    for (node *cur = _list->head; cur != NULL; cur = cur->next)
    {
        printf("%d) %p\n", i, cur);
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
    list->fn_pop_push_end = pop_push_end;
    list->fn_clear_nodes = clear_nodes;
    list->fn_free_list = free_list;
    list->fn_remove_value = remove_value;
    list->fn_dbg_print = dbg_list_print;
    return list;
}

void init_proc_list(proc_list *_self)
{
    _self->processSize = 0;
    _self->listSize = MINPRIORITY + 2;
    for (int i = 0; i < _self->listSize; i++)
        _self->nList[i] = init_nodelist();

    _self->fn_dispatcher = pl_dispatcher;
    _self->fn_push_proc = pl_push_proc;
    _self->fn_dbg_print_nodelist = pl_dbg_print_nodelist;
}

proc_ptr pl_dispatcher(proc_list *_self)
{
    for (int i = 0; i < _self->listSize; i++)
    {
        if (_self->nList[i]->length == 0) // Continue if there is none in this priority
            continue;

        for (int x = 0; x < _self->nList[i]->length; i++) // for length of processes search for a Ready one
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
void pl_dbg_print_nodelist(proc_list *_self)
{
    for (int i = 0; i < _self->listSize; i++)
    {
        printf("%d)\n", i);
        _self->nList[i]->fn_dbg_print(_self->nList[i]);
    }
}
