#ifndef _user_H
#define _user_H

typedef struct procSlot *procPtr;
typedef struct semaphore *semaphore;

int proc_init(procPtr proc);
void proc_set(procPtr proc, procPtr parent, int PID, char *name, int (*func)(char *), char *arg, int stack_size, int priority);

struct procSlot
{
    int pid;
    int parentPid;
    int (*start_func)(char *);
    char *name;
    int status;
    char *arg;
    int stack_size;
    int priority;
    procPtr nextChild;
    procPtr nextSib;
    procPtr parent;
    procPtr nextProc;
    int privateMbox;
    int termCode;
};

struct sem
{
    int value;
};

struct semaphore
{
    int value;
    int semID;
    int mboxID;
    procPtr nextBlockedProc;
};

#define READY 0
#define JOIN_BLOCKED 1
#define ZOMBIE 2
#define WAIT_BLOCKED 11
#define ZAP_BLOCKED 12

#endif