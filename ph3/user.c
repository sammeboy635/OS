#include <stdio.h>
#include <stdlib.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <stddef.h>
#include "user.h"

int proc_clear(procPtr proc)
{
    proc->pid = -1;
    proc->priority = -1;
    proc->stack_size = -1;
    proc->mbox = -1;
    proc->qcode = -1;
    proc->status = -1;
    proc->sFunc = NULL;
    proc->name = NULL;
    proc->arg = NULL;
    proc->parent = NULL;
    proc->child = NULL;
    proc->next = NULL;
}

void proc_set(procPtr proc, procPtr parent, int pid, char *name, int (*func)(char *), char *arg, int stack_size, int priority)
{

    proc->pid = pid;
    proc->name = name;
    proc->sFunc = func;
    proc->status = READY;
    proc->stack_size = stack_size;
    proc->priority = priority;
    proc->qcode = -1;
    proc->parent = parent;
    proc->child = NULL;
    proc->next = NULL;

    if (proc->mbox == -1)
        proc->mbox = MboxCreate(0, 50);
    // proc->mbox = MboxCreate(0, sizeof(int[2]));

    if (arg == NULL)
        proc->arg = NULL;
    else
        proc->arg = &arg[0];

    if (parent != NULL)
    {
        // proc->parentPid = parent->pid;
        if (parent->child != NULL)
        {
            int i = 1;
            // there are other children in the list, add to end
            procPtr cur = parent->child;
            while (cur->next != NULL)
            {
                cur = cur->next;
                i++;
            }
            // cur now points to last sib in the list
            cur->next = proc;
        }
        else
        {
            parent->child = proc;
        }
    }
}
procPtr proc_wait(procPtr proc, int *status) // TESTING NOT TESTED
{
    if (proc->child == NULL)
        return NULL;

    procPtr cur = proc->child;
    while (cur != NULL)
    {
        if (cur->status == END_WAITING)
        {
            return cur;
        }
        cur = cur->next;
    }
    return cur;
}