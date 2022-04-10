#include <stdio.h>
#include <stdlib.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <stddef.h>
#include "user.h"

int proc_init(procPtr proc)
{
    proc->pid = -1;
    proc->parentPid = -1;
    proc->nextChild = NULL;
    proc->nextSib = NULL;
    proc->parent = NULL;
    proc->priority = -1;
    proc->start_func = NULL;
    proc->name = NULL;
    proc->arg = NULL;
    proc->stack_size = -1;
    proc->privateMbox = -1;
    proc->termCode = -1;
    proc->status = -1;
    proc->nextProc = NULL;
}
void proc_set(procPtr proc, procPtr parent, int PID, char *name, int (*func)(char *), char *arg, int stack_size, int priority)
{
    parent->nextChild = proc; // Needs to be a list

    proc->pid = PID + 1;
    proc->parentPid = PID; // start1 PID
    proc->start_func = func;
    proc->name = name;
    proc->status = READY;
    proc->arg = arg;
    proc->stack_size = USLOSS_MIN_STACK;
    proc->priority = priority;
    proc->nextChild = NULL;
    proc->nextSib = NULL;
    proc->parent = parent;
    proc->privateMbox = MboxCreate(0, sizeof(int[2]));
    proc->termCode = -1;
}