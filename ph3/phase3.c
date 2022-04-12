#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <usyscall.h>
#include <libuser.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "user.h"
#include "provided_prototypes.h"
#define DEBUG3 0
extern int start3(char *);
static void nullsys3(sysargs *args);
int spawn_launch();
int set_to_user_mode();
int kernel_check(char *procName);

void sysspawn(sysargs *args);
void syswait(sysargs *args);
void systerminate(sysargs *args);
void sysgettimeofday(sysargs *args);
void syscputime(sysargs *args);
void sysgetpid(sysargs *args);

int mbox;

struct proc ProcList[MAXPROC];

int start2(char *arg)
{
    int pid = 0;
    int status;
    mbox = MboxCreate(1, 0);
    int senderbox = MboxCreate(1, 0);

    for (int i = 0; i < MAXSYSCALLS; i++)
        sys_vec[i] = nullsys3;

    sys_vec[SYS_SPAWN] = sysspawn;
    sys_vec[SYS_WAIT] = syswait;
    sys_vec[SYS_TERMINATE] = systerminate;
    sys_vec[SYS_GETTIMEOFDAY] = sysgettimeofday;
    sys_vec[SYS_CPUTIME] = syscputime;
    sys_vec[SYS_GETPID] = sysgetpid;

    for (int i = 0; i < MAXPROC; i++)
    {
        proc_clear(&ProcList[i]);
    }

    proc_set(&ProcList[1], NULL, 1, "sentinel", NULL, NULL, 0, 6);
    proc_set(&ProcList[2], NULL, 2, "start1", NULL, NULL, 0, 1);
    ProcList[2].status = JOIN;

    proc_set(&ProcList[getpid()], &ProcList[2], getpid(), "start2", start2, arg, USLOSS_MIN_STACK, 1);

    pid = spawn_real("start3", start3, NULL, 4 * USLOSS_MIN_STACK, 3);
    pid = wait_real(&status);

    set_to_user_mode();

    dbg_print("Start Ending");
    Terminate(1);

    return 0;
}

void sysspawn(sysargs *args)
{
    char *name = args->arg5;
    int (*func)(char *) = args->arg1;
    char *arg = args->arg2;
    int stack_size = (int)args->arg3;
    int priority = (int)args->arg4;

    int pid = spawn_real(name, func, arg, stack_size, priority);

    args->arg1 = pid;

    set_to_user_mode();
    return;
}
int spawn_real(char *name, int (*func)(char *), char *arg, int stack_size, int priority)
{
    int pid = fork1(name, spawn_launch, arg, stack_size, priority);

    MboxSend(mbox, NULL, 0);
    proc_set(&ProcList[pid], &ProcList[getpid()], pid, name, func, arg, stack_size, priority);

    MboxReceive(mbox, NULL, 0);
    MboxCondSend(ProcList[pid].mbox, NULL, 0);

    return pid;
}

int spawn_launch()
{
    dbg_print("sending from spawn_launch");
    MboxSend(mbox, NULL, 0);
    int pid = getpid();
    if (ProcList[pid].pid != pid)
    {
        ProcList[pid].mbox = MboxCreate(0, sizeof(int) * 2);
        dbg_print("receiving from spawn_launch");
        MboxReceive(mbox, NULL, 0);
        dbg_print("private receiving from spawn_launch");
        MboxReceive(ProcList[pid].mbox, 0, 0);
    }
    else
    {
        dbg_print("receiving from spawn_launch");
        MboxReceive(mbox, NULL, 0);
    }

    if (is_zapped())
    {
        dbg_print("receiving from spawn_launch");
        set_to_user_mode();
        Terminate(1);
    }
    else
    {
        set_to_user_mode();
        int result = ProcList[pid].sFunc(ProcList[pid].arg);
        Terminate(result);
    }
}

void syswait(sysargs *args)
{

    MboxSend(mbox, NULL, 0);
    if (ProcList[getpid()].child == NULL) // Child Null
    {
        dbg_print("Called Wait with no Child");
        MboxReceive(mbox, NULL, 0);
    }
    else
    {
        dbg_print("Waiting for child");
        MboxReceive(mbox, NULL, 0);
    }

    int status;
    args->arg1 = (void *)wait_real(&status);
    args->arg2 = (void *)status;
    dbg_print("Child Term");
}

int wait_real(int *status)
{
    dbg_print("in waiting state wait_real");
    dbg_print("Sending from wait_real");
    MboxSend(mbox, NULL, 0);

    int results[] = {0, 0};
    int pid = getpid();
    if (ProcList[pid].child != NULL)
    {
        procPtr cur = ProcList[pid].child;
        while (cur != NULL)
        {
            if (cur->status == END_WAITING)
            {
                dbg_print("reveive from wait_real");
                MboxReceive(mbox, NULL, 0);

                dbg_print("reveive from wait_real");
                MboxReceive(cur->mbox, results, sizeof(int) * 2);

                *status = results[1];
                dbg_print("End_waiting returning");
                int stats;
                join(&stats);

                return results[0];
            }
            cur = cur->next;
        }
    }
    dbg_print("receive from wait_real");
    MboxReceive(mbox, NULL, 0);

    ProcList[pid].status = WAIT;

    dbg_print("receive from wait_real");
    MboxReceive(ProcList[pid].mbox, results, sizeof(int) * 2);

    dbg_print("send from wait_real");
    MboxSend(mbox, NULL, 0);

    ProcList[pid].status = READY;

    dbg_print("receive from wait_real");
    MboxReceive(mbox, NULL, 0);

    *status = results[1];

    return results[0];
}
void systerminate(sysargs *args)
{
    dbg_print("Sending from terminate");
    MboxSend(mbox, NULL, 0);

    dbg_print("Receiving from terminate");
    MboxReceive(mbox, NULL, 0);

    int qcode = (int)args->arg1;
    int pid = getpid();

    dbg_print("Sending from terminate");
    MboxSend(mbox, NULL, 0);

    if (ProcList[pid].child == NULL) // No Child
    {
        if (ProcList[pid].parent->status == ZAPPED)
        {
            dbg_print("Zapped while quiting!");
            proc_clear(&ProcList[pid]);

            dbg_print("receiving from terminate");
            MboxReceive(mbox, NULL, 0);
            quit(1);
        }
        else if (ProcList[pid].parent->status == WAIT) // Parent is wait blocked
        {
            dbg_print("Parent was waiting");
            int message[] = {pid, qcode}; // build message

            dbg_print("receiving from terminate");
            MboxReceive(mbox, NULL, 0);

            dbg_print("sending from terminate");
            MboxSend(ProcList[ProcList[pid].parent->pid].mbox, message, sizeof(message));
        }
        else if (ProcList[pid].parent->status == READY)
        {
            ProcList[pid].status = END_WAITING; // Put current proc into waiting mode
            int message[] = {pid, qcode};

            dbg_print("receiving from terminate");
            MboxReceive(mbox, NULL, 0);

            dbg_print("private sending from terminate");
            MboxSend(ProcList[pid].mbox, message, sizeof(message));
        }
    }
    else // Everything else
    {
        dbg_print("receiving from terminate");
        MboxReceive(mbox, NULL, 0);

        dbg_print("sending from terminate");
        MboxSend(mbox, NULL, 0);

        procPtr cur = ProcList[pid % MAXPROC].child;

        dbg_print("receiving from terminate");
        MboxReceive(mbox, NULL, 0);

        procPtr next = NULL;
        while (cur != NULL)
        {
            dbg_print("Sending from terminate");
            MboxSend(mbox, NULL, 0);
            if (cur->next == NULL)
                next = NULL;
            else
                next = cur->next;

            if (cur->status == END_WAITING)
            {
                int result[] = {-1, -1};
                // Waking up waiting process
                dbg_print("receiving from terminate");
                MboxReceive(mbox, NULL, 0);
                dbg_print("private from terminate");
                MboxReceive(cur->mbox, result, sizeof(int) * 2);
            }
            else // ZAPPING CHILD
            {
                ProcList[pid].status = ZAPPED;

                dbg_print("receiving from terminate");
                MboxReceive(mbox, NULL, 0);

                zap(cur->pid);
                ProcList[pid].status = READY;
            }
            cur = next;
        }

        dbg_print("sending from terminate");
        MboxSend(mbox, NULL, 0);

        if (ProcList[pid].pid == 3) // SPECIAL CASE FOR START2
        {
            dbg_print("receiving from terminate");
            MboxReceive(mbox, NULL, 0);
            quit(qcode);
        }
        else if (ProcList[pid].parent->status == WAIT) // PARENT IN WAIT TODO SEGFAULT HERE
        {
            int message[] = {pid, qcode};

            dbg_print("receiving from terminate");
            MboxReceive(mbox, NULL, 0);

            dbg_print("private sending results from terminate");
            MboxSend(ProcList[ProcList[pid].parent->pid].mbox, message, sizeof(message));
        }
        else if (ProcList[pid].parent->status == READY) // If the parent hasn't called wait, then put process into wait
        {

            ProcList[pid].status = END_WAITING; // Set to waiting status
            int message[] = {pid, qcode};

            dbg_print("receiving from terminate");
            MboxReceive(mbox, NULL, 0);

            dbg_print("private sending from terminate");
            MboxSend(ProcList[pid].mbox, message, sizeof(message));
        }

        dbg_print("cond receiving from terminate");
        MboxCondReceive(mbox, NULL, 0);
    }

    dbg_print("Sending from terminate");
    MboxSend(mbox, NULL, 0);

    if (ProcList[getpid()].next == NULL)
        ProcList[getpid()].parent->child = NULL;
    else
        ProcList[getpid()].parent->child = ProcList[getpid()].next;

    proc_clear(&ProcList[getpid()]);

    dbg_print("receiving from terminate");
    MboxReceive(mbox, NULL, 0);

    quit(qcode);
}

int set_to_user_mode()
{
    psr_set(psr_get() & ~PSR_CURRENT_MODE);
}

void sysgettimeofday(sysargs *args)
{
    args->arg1 = (void *)clock();
    set_to_user_mode();
}
int gettimeofday_real(int *time)
{
    time = clock();
    return 1;
}
void syscputime(sysargs *args)
{
    // args->arg1 = readtime();
    args->arg1 = 5;
    // readtime();
}
void sysgetpid(sysargs *args)
{
    args->arg1 = getpid();
    set_to_user_mode();
}
int kernel_check(char *procName)
{
    if ((PSR_CURRENT_MODE & psr_get()) == 0)
    {
        console("Not in Kernal Mode\n");
        halt(1);
        return 0;
    }
    return 1;
}

static void nullsys3(sysargs *args_ptr)
{
    console("nullsys3(): Invalid syscall %d : Process %d terminating\n", args_ptr->number, getpid());
    systerminate(1);
}

void dbg_print(char *_str, ...)
{
    if (DEBUG3 == 1) // TEACHER ASK ABOUT VARS NOT PRINTING RIGHT
    {
        va_list argptr;
        va_start(argptr, _str);
        console(_str, argptr);
        va_end(argptr);
    }
}