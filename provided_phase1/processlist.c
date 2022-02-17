#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <phase1.h>
#include "kernel.h"

void init_proc_list(proc_list *_self)
{
    _self->processSize = 0;
    _self->listSize = MINPRIORITY + 2;
    for (int i = 0; i < _self->listSize; i++)
        _self->nList[i] = init_nodelist();

    _self->fn_free = pl_free;
    _self->fn_remove_proc = pl_remove_proc;
    _self->fn_dispatcher = pl_dispatcher;
    _self->fn_push_proc = pl_push_proc;
    _self->fn_dbg_print_nodelist = pl_dbg_print_nodelist;
    _self->fn_deadlocked = pl_deadlocked;
    _self->fn_find_pid = pl_find_pid;
}
void pl_free(proc_list *_self)
{
    printf("All processes completed.\n");
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
    _self->nList[_proc->priority]->fn_remove_value(_self->nList[_proc->priority], _proc, TRUE);
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
    printf("%s %12s %15s %10s %10s%10s%10s", "PID", "PRIORITY", "STATUS", "NAME", "Child", "TOTAL TIME MS", "NAME\n");
    for (int i = 0; i < _self->listSize; i++)
    {
        // printf("%d)\n", i);
        _self->nList[i]->fn_dbg_print(_self->nList[i]);
    }
}
