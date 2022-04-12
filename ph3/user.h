#ifndef _user_H
#define _user_H

typedef struct proc *procPtr;

int proc_clear(procPtr proc);
void proc_set(procPtr proc, procPtr parent, int PID, char *name, int (*func)(char *), char *arg, int stack_size, int priority);
procPtr proc_wait(procPtr proc, int *status);

#define READY 196
#define JOIN 197
#define WAIT 198
#define ZAPPED 199
#define END_WAITING 200

struct proc
{
    int pid;
    int (*sFunc)(char *);
    char *name;
    int status;
    char *arg;
    int stack_size;
    int priority;
    int mbox;
    int qcode;

    procPtr parent;
    procPtr child;
    procPtr next;
};

#endif