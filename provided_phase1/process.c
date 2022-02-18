#include <stdio.h>
#include <stdlib.h>
#include <phase1.h>
#include "kernel.h"
#include <string.h>

proc_ptr init_proc_ptr(char *name, int (*f)(char *), char *arg, int stacksize, int priority, unsigned int *next_pid, void(*launch))
{

    /* fill-in entry in process table */
    proc_ptr newProcess = (proc_ptr)malloc(sizeof(proc_struct));

    strcpy(newProcess->name, name);                // name
    newProcess->start_func = f;                    // function
    strcpy(newProcess->start_arg, arg);            // function args
    newProcess->state.start = (void (*)())f;       // Set process start address
    newProcess->stacksize = stacksize;             // Set Process Stack Size
    newProcess->stack = (char *)malloc(stacksize); // Allocate Memory for Process Stack
    newProcess->status = READY;                    // Set Process Status READY
    newProcess->priority = priority;               // Set Process priority
    newProcess->pid = (*next_pid)++;               // Check this later

    newProcess->time.startTime = sys_clock();
    newProcess->time.processTime = sys_clock();
    newProcess->time.totalRunTime = 0;

    // Other processes info

    newProcess->childJoinCount = 0;
    newProcess->child = init_nodelist();

    newProcess->isZappedCount = FALSE;
    newProcess->zapList = init_nodelist();
    newProcess->parent = NULL;

    newProcess->fn_free = p_free;

    newProcess->fn_child_add = p_child_add;
    newProcess->fn_child_remove = p_child_remove;
    newProcess->fn_child_count = p_child_count;
    newProcess->fn_child_next = p_child_next;
    newProcess->fn_child_active = p_child_active;

    newProcess->fn_zap_add = p_zap_add;
    newProcess->fn_zap_count = p_zap_count;
    newProcess->fn_is_zapped = p_is_zapped;
    newProcess->fn_unblock_zapped = p_unblock_zapped;

    newProcess->fn_time_end_of_run_set = p_time_end_of_run_set;
    newProcess->fn_time_start_set = p_time_start_set;
    newProcess->fn_time_ready_to_run = p_time_ready_to_run;
    newProcess->fn_time_ready_to_quit = p_time_ready_to_quit;

    context_init(&(newProcess->state), psr_get(),
                 newProcess->stack,
                 newProcess->stacksize, launch);

    return newProcess;
}

void *_free(void *ptr)
{
    if (ptr == NULL)
    {
        printf("Tried to free NULL!\n");
        return NULL;
    }
    free(ptr);
    ptr = NULL; // NEW
    return NULL;
}
void _process_switch(proc_ptr *Current, proc_ptr newProc)
{
    proc_ptr old = *Current;
    old->status = READY;
    old->fn_time_end_of_run_set(old);
    newProc->status = RUNNING;
    newProc->fn_time_start_set(newProc);
    *Current = newProc;
    context_switch(&old->state, &newProc->state);
}
/*free everything except the pointer to it*/
void p_free(proc_ptr _self)
{
    _self->child->fn_clear_nodes(_self->child, FALSE);
    _self->zapList->fn_clear_nodes(_self->zapList, FALSE); // Freeing Zap
    _free(_self->child);
    _free(_self->zapList);
    if (_self->stack != NULL)
        _free(_self->stack); // TEACHER ask test11 Start1 error;
}

void p_child_add(proc_ptr _self, proc_ptr _newChild)
{
    _self->child->fn_push(_self->child, _newChild);
    _newChild->parent = _self;
}
void p_child_remove(proc_ptr _self, proc_ptr _oldchild)
{
    _self->child->fn_remove_value(_self->child, _oldchild, FALSE);
}
int p_child_count(proc_ptr _self)
{
    return _self->child->length;
}
proc_ptr p_child_next(proc_ptr _self)
{
    int count = _self->fn_child_count(_self);
    if (count == 0 || count == _self->childJoinCount)
        return NULL;
    else
        return _self->child->fn_get_index(_self->child, _self->childJoinCount);
}
int p_child_active(proc_ptr _self)
{
    if (_self->child->length == 0)
        return FALSE;
    for (node *cur = _self->child->head; cur != NULL; cur = cur->next)
    {
        if (cur->value->status != QUIT)
            return TRUE;
    }
    return FALSE;
}

void p_zap_add(proc_ptr _self, proc_ptr _zapProc)
{
    _zapProc->status = BLOCKED;
    _self->isZappedCount += 1;
    _self->zapList->fn_push(_self->zapList, _zapProc);
}
int p_zap_count(proc_ptr _self)
{
    return _self->isZappedCount;
}
int p_is_zapped(proc_ptr _self)
{
    return ((_self->isZappedCount == 0) ? FALSE : TRUE);
}
void p_unblock_zapped(proc_ptr _self)
{
    // if (_self->fn_is_zapped(_self) == FALSE) // This also returns parents from joins
    for (int i = 0; i < _self->zapList->length; i++)
    {
        proc_ptr cur = _self->zapList->fn_get_index(_self->zapList, i);

        if (cur->status != BLOCKED)
        {
            printf("p_unblock_zapped: Had a not block process in zap");
            halt(1);
        }
        cur->status = READY;
    }
    _self->isZappedCount = FALSE;
    // TODO maybe clear list
    //_self->zapList->fn_clear_nodes(_self->zapList,FALSE);
}

void p_time_end_of_run_set(proc_ptr _self)
{
    long newTime = sys_clock();
    _self->time.totalRunTime += newTime - _self->time.processTime; // TEACHER ASK ABOUT TIMING
    _self->time.processTime = newTime;
}
void p_time_start_set(proc_ptr _self)
{
    _self->time.processTime = sys_clock();
}
int p_time_ready_to_run(proc_ptr _self)
{
    return (_self->time.processTime - (sys_clock()) > 80) ? FALSE : TRUE;
}
int p_time_ready_to_quit(proc_ptr _self)
{
    return (_self->time.processTime - (sys_clock()) > 80) ? FALSE : TRUE;
}