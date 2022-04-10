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
#define DEBUG3 0
extern int start3(char *);
static void nullsys3(sysargs *args);
int spawn_launch();
int setToUserMode();
int inKernelMode(char *procName);
int spawn_real(char *name, int (*func)(char *), char *arg, int stack_size, int priority);
int wait_real(int *status);
void terminate_real(int exit_code);
void spawn();

int PID;
int mbox;
int debugflag3 = 0;

// table of semaphores
struct semaphore semTable[MAXSEMS];
struct procSlot procTable[MAXPROC];

int start2(char *arg)
{
    int pid = 0;
    int status;
    /*
     * Check kernel mode here.
     */
    // Need sys_vec
    /*
     * Data structure initialization as needed...
     */
    for (int i = 0; i < MAXPROC; i++)
    {
        proc_init(&procTable[i]);
    }
    mbox = MboxCreate(0, 5);
    mbox = MboxCreate(0, 5);

    PID = getpid();

    for (int i = 0; i < MAXSYSCALLS; i++)
        sys_vec[i] = nullsys3;

    sys_vec[SYS_SPAWN] = spawn;
    // sys_vec[SYS_WAIT] = wait_real;
    // sys_vec[SYS_TERMINATE] = terminate_real;
    /*
     * Create first user-level process and wait for it to finish.
     * These are lower-case because they are not system calls;
     * system calls cannot be invoked from kernel mode.
     * Assumes kernel-mode versions of the system calls
     * with lower-case names.  I.e., Spawn is the user-mode function
     * called by the test cases; spawn is the kernel-mode function that
     * is called by the syscall_handler; spawn_real is the function that
     * contains the implementation and is called by spawn.
     *
     * Spawn() is in libuser.c.  It invokes usyscall()
     * The system call handler calls a function named spawn() -- note lower
     * case -- that extracts the arguments from the sysargs pointer, and
     * checks them for possible errors.  This function then calls spawn_real().
     *
     * Here, we only call spawn_real(), since we are already in kernel mode.
     *
     * spawn_real() will create the process by using a call to fork1 to
     * create a process executing the code in spawn_launch().  spawn_real()
     * and spawn_launch() then coordinate the completion of the phase 3
     * process table entries needed for the new process.  spawn_real() will
     * return to the original caller of Spawn, while spawn_launch() will
     * begin executing the function passed to Spawn. spawn_launch() will
     * need to switch to user-mode before allowing user code to execute.
     * spawn_real() will return to spawn(), which will put the return
     * values back into the sysargs pointer, switch to user-mode, and
     * return to the user code that called Spawn.
     */
    pid = spawn_real("start3", start3, NULL, 4 * USLOSS_MIN_STACK, 3);
    pid = wait_real(&status);
    setToUserMode();
    return 0;
} /* start2 */
  /* start2 */

void spawn(sysargs *args)
{
    if (DEBUG3 && debugflag3)
        USLOSS_Console("spawn(): at beginning\n");
    /* extract args and check for errors */
    // get address of function to spawn
    int (*func)(char *) = args->arg1;
    // get function name
    char *name = args->arg5;
    // get argument passed to spawned function
    char *arg = args->arg2;
    // get stack size
    int stack_size = (int)args->arg3;
    // get priority
    int priority = (int)args->arg4;

    // return if name is an illegal value
    if (name == NULL)
    {
        if (DEBUG3 && debugflag3)
            console("spawn(): illegal value for name! Returning\n");
        args->arg1 = (void *)-1;
        args->arg4 = (void *)-1;
        return;
    }
    // return if priority is an illegal value
    if (priority < 1 || priority > 5)
    {
        if (DEBUG3 && debugflag3)
            console("spawn(): illegal value for priority! Returning\n");
        args->arg1 = (void *)-1;
        args->arg4 = (void *)-1;
        return;
    }
    // return if stack size is and illegal value
    if (stack_size < USLOSS_MIN_STACK)
    {
        if (DEBUG3 && debugflag3)
            console("spawn(): illegal value for stack size! Returning\n");
        args->arg1 = (void *)-1;
        args->arg4 = (void *)-1;
        return;
    }
    // return if name is an illegal value
    if (strlen(name) > MAXNAME)
    {
        if (DEBUG3 && debugflag3)
            console("spawn(): illegal value for name! Returning\n");
        args->arg1 = (void *)-1;
        args->arg4 = (void *)-1;
        return;
    }
    // return if arg is an illegal value
    if (arg != NULL && strlen(arg) > MAXARG)
    {
        if (DEBUG3 && debugflag3)
            console("spawn(): illegal value for arg! Returning\n");
        args->arg1 = (void *)-1;
        args->arg4 = (void *)-1;
        return;

        if (DEBUG3 && debugflag3)
            console("spawn(): At end\n");
    }
    // arguments are legal, give them to spawnReal, pass arg1 for pid
    int kpid = spawn_real(name, func, arg, stack_size, priority);

    // check to make sure spawnReal worked
    if (kpid == -1)
    {
        if (DEBUG3 && debugflag3)
            console("spawn(): fork1 failed to create process, returning -1\n");
        args->arg4 = (void *)-1;
    }

    // assign pid to proper spot of arg struct
    args->arg1 = kpid;
    // no errors at this point, arg4 can be set to 0
    args->arg4 = 0;
    // will return to user level function, set to user mode!
    setToUserMode();
    return;
}
int spawn_real(char *name, int (*func)(char *), char *arg, int stack_size, int priority)
{
    int kpid = fork1(name, spawn_launch, arg, stack_size, priority);
    proc_set(&procTable[PID + 1], &procTable[PID], PID, name, func, arg, stack_size, priority);
    MboxSend(mbox, "", 0);

    return kpid;
}

int spawn_launch()
{

    MboxReceive(mbox, NULL, 0);
    procPtr p = &procTable[getpid()];
    setToUserMode();

    p->start_func(p->arg);
    return 0;
}

int wait_real(int *status)
{
    procPtr p = &procTable[getpid()];
    MboxReceive(p->nextChild->privateMbox, "", 0);
    setToUserMode();
    return 0;
}

void terminate_real(int exit_code)
{
    procPtr p = &procTable[getpid()];
    p->termCode = exit_code;
    MboxReceive(p->privateMbox, NULL, 0);
    // Terminate the rest of the processes somehow
}

void syscall_handler(int dev, void *unit)
{
    sysargs *sys_ptr;
    sys_ptr = (sysargs *)unit;
    /* Sanity check : if the interrupt is not SYSCALL_INT, halt(1) */
    // more checking
    /* Now it is time to call the appropriate system call handler*/
    sys_vec[sys_ptr->number](sys_ptr);
}
int setToUserMode()
{
    psr_set(psr_get() & ~PSR_CURRENT_MODE);
}

int inKernelMode(char *procName)
{
    if ((PSR_CURRENT_MODE & psr_get()) == 0)
    {
        console("Kernel Error: Not in kernel mode, may not run %s()\n", procName);
        halt(1);
        return 0;
    }
    else
    {
        return 1;
    }
}
static void nullsys3(sysargs *args_ptr)
{
    printf("nullsys3(): Invalid syscall %d\n", args_ptr->number);
    printf("nullsys3(): process %d terminating\n", PID);
    terminate_real(1);
} /* nullsys3 */