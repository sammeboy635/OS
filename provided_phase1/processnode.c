#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <phase1.h>
#include "kernel.h"

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
void remove_value(nodelist *_list, proc_ptr _value, int clearValueTF)
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
        if (clearValueTF == TRUE)
        {
            _free(cur->value);
        }
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
        printf("%d %10d %20s %10s %10d %10ld %10s\n", cur->value->pid, cur->value->priority, STATUSNAME[cur->value->status], cur->value->name, cur->value->child->length, cur->value->time.totalRunTime, cur->value->name); //
        i++;
    }
}
