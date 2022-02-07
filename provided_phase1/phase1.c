/* ------------------------------------------------------------------------
   phase1.c

   CSCV 452

   ------------------------------------------------------------------------ */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <phase1.h>

#include "kernel.h"

/* ------------------------- Prototypes ----------------------------------- */
int sentinel(char *dummy);
extern int start1(char *);
void dispatcher(void);
void launch();
// static void enableInterrupts();
static void check_deadlock();

void os_kernel_check(char *func_name);
int os_get_next_pid();
void dbg_print(char *_str, ...);
void enableInterrupts();
void disableInterrupts();

/* -------------------------- Globals ------------------------------------- */

/* Patrick's debugging global variable... */
int debugflag = 1;

/* the process table */
// proc_struct ProcTable[MAXPROC];
proc_list self;

int numProc = 0;

/* Process lists  */

/* current process ID */
proc_ptr Current;

/* the next pid to be assigned */
unsigned int next_pid = SENTINELPID;

/* -------------------------- Functions ----------------------------------- */
/* ------------------------------------------------------------------------
   Name - startup
   Purpose - Initializes process lists and clock interrupt vector.
         Start up sentinel process and the test process.
   Parameters - none, called by USLOSS
   Returns - nothing
   Side Effects - lots, starts the whole thing
   ----------------------------------------------------------------------- */
void startup()
{
    // int i;      /* loop index */
    int result; /* value returned by call to fork1() */
    init_proc_list(&self);

    /* initialize the process table */
    // memset(ProcTable, 0, sizeof(ProcTable));
    os_kernel_check("__START__");

    /* Initialize the Ready list, etc. */
    dbg_print("startup(): initializing the Ready & Blocked lists");
    // ReadyList = NULL;

    /* Initialize the clock interrupt handler */
    Current = NULL;
    /* startup a sentinel process */
    dbg_print("startup(): calling fork1() for sentinel");

    result = fork1("sentinel\0", sentinel, NULL, 2 * USLOSS_MIN_STACK,
                   SENTINELPRIORITY);
    if (result < 0)
    {
        dbg_print("startup(): fork1 of sentinel returned error, halting...");
        halt(1);
    }

    /* start the test process */
    dbg_print("startup(): calling fork1() for start1");

    result = fork1("start1\0", start1, NULL, 2 * USLOSS_MIN_STACK, 1);
    if (result < 0)
    {
        dbg_print("startup(): fork1 for start1 returned an error, halting...");
        halt(1);
    }

    // launch();

    dbg_print("startup(): Should not see this message! ");
    dbg_print("Returned from fork1 call that created start1");

    return;
} /* startup */

/* ------------------------------------------------------------------------
   Name - finish
   Purpose - Required by USLOSS
   Parameters - none
   Returns - nothing
   Side Effects - none
   ----------------------------------------------------------------------- */
void finish()
{
    dbg_print("in finish...");
} /* finish */

/* ------------------------------------------------------------------------
   Name - fork1
   Purpose - Gets a new process from the process table and initializes
             information of the process.  Updates information in the
             parent process to reflect this child process creation.
   Parameters - the process procedure address, the size of the stack and
                the priority to be assigned to the child process.
   Returns - the process id of the created child or -1 if no child could
             be created or if priority is not between max and min priority.
   Side Effects - ReadyList is changed, ProcTable is changed, Current
                  process information changed
   ------------------------------------------------------------------------ */
int fork1(char *name, int (*f)(char *), char *arg, int stacksize, int priority)
{
    dbg_print("fork1(): creating process %s\n", name);

    /* test if in kernel mode; halt if in user mode */
    os_kernel_check("__FORK1__");

    if (self.listSize > MAXPROC) // TO MANY PROCESS IN THE LIST
        return -1;

    if (arg == NULL)
        arg = "\0";
    if (strlen(name) >= MAXNAME - 1 || strlen(arg) >= (MAXARG - 1))
    {
        dbg_print("fork1(): Process name or Arg is too long.  Halting...\n");
        halt(1);
    }
    else if (stacksize < USLOSS_MIN_STACK) // Return if stack is to small TODO = CHeck if the stack is to big
        return -2;
    else if (priority < MAXPRIORITY || priority > MINPRIORITY) // PRIORITY NOT WITHIN RANGE
    {
        if (priority != SENTINELPRIORITY) // IF NOT SENTINEL
        {
            dbg_print("fork1: priority out of range");
            return -3;
        }
    }

    proc_ptr newProcess = init_proc_ptr(name, f, arg, stacksize, priority, &next_pid, launch);

    /*context_init(&(newProcess->state), psr_get(),
                 newProcess->stack,
                 newProcess->stacksize, launch);*/

    if (strcmp("sentinel", name) == 0) // IF SENTINEL SET CURRENT
    {
        Current = newProcess;
    }
    else if (strcmp("start1", name) == 0) // START1 SET CHILD OF SENTINEL AND SET CURRENT TO START1
    {
        Current->fn_child_add(Current, newProcess);
        newProcess->parent = Current;
        Current = newProcess;
    }
    else
    {
        Current->fn_child_add(Current, newProcess);
        newProcess->parent = Current;
    }

    self.fn_push_proc(&self, priority, newProcess); // Pushes new process onto the list
    /* for future phase(s) */
    // p1_fork(newProcess->pid);

    if (newProcess->pid != SENTINELPID) // Dont do sentinelPid
        dispatcher();

    return next_pid;
} /* fork1 */

/* ------------------------------------------------------------------------
   Name - launch
   Purpose - Dummy function to enable interrupts and launch a given process
             upon startup.
   Parameters - none
   Returns - nothing
   Side Effects - enable interrupts
   ------------------------------------------------------------------------ */
void launch()
{
    if (Current == NULL)
    {
        dbg_print("Launch: Current is NULL");
        halt(1);
    }

    int result;

    dbg_print("launch(): started\n");

    /* Enable interrupts */
    enableInterrupts();

    /* Call the function passed to fork1, and capture its return value */
    result = Current->start_func(Current->start_arg);

    dbg_print("Process %d returned to launch\n", Current->pid);
    dbg_print("results %d\n", result);

    quit(result);

} /* launch */

/* ------------------------------------------------------------------------
   Name - join
   Purpose - Wait for a child process (if one has been forked) to quit.  If
             one has already quit, don't wait.
   Parameters - a pointer to an int where the termination code of the
                quitting process is to be stored.
   Returns - the process id of the quitting child joined on.
        -1 if the process was zapped in the join
        -2 if the process has no children
   Side Effects - If no child process has quit before join is called, the
                  parent is removed from the ready list and blocked.
   ------------------------------------------------------------------------ */
int join(int *code)
{
    disableInterrupts();
    proc_ptr child = Current->fn_child_next(Current);
    if (child == NULL) // NO CHILD PROCESS if NULL
    {
        *code = -2;
        return -2;
    }
    else if (child->status == QUIT) // CHILD QUIT
    {
        Current->childJoinCount++;
        *code = child->quitCode;
        return child->pid;
    }

    Current->status = BLOCKED;
    dispatcher();

    enableInterrupts();
    Current->childJoinCount++;
    *code = child->quitCode;
    return child->pid;
} /* join */

/* ------------------------------------------------------------------------
   Name - quit
   Purpose - Stops the child process and notifies the parent of the death by
             putting child quit info on the parents child completion code
             list.
   Parameters - the code to return to the grieving parent
   Returns - nothing
   Side Effects - changes the parent of pid child completion status list.
   ------------------------------------------------------------------------ */
void quit(int code)
{
    os_kernel_check("quit");

    Current->status = QUIT;
    Current->quitCode = code;
    dbg_print("PID) %d just quit", Current->pid);

    if (Current->fn_is_zapped(Current) == TRUE)
    {
        Current->fn_unblock_zapped(Current);
    }
    else if (Current->parent->status == BLOCKED) // TODO MAYBE NEED TO CHANGE TO JOINED CODE
        Current->parent->status = READY;

    dispatcher();

} /* quit */

/* ------------------------------------------------------------------------
   Name - dispatcher
   Purpose - dispatches ready processes.  The process with the highest
             priority (the first on the ready list) is scheduled to
             run.  The old process is swapped out and the new process
             swapped in.
   Parameters - none
   Returns - nothing
   Side Effects - the context of the machine is changed
   ----------------------------------------------------------------------- */
void dispatcher(void)
{
    // self.fn_dbg_print_nodelist(&self);
    proc_ptr nextProcess = self.fn_dispatcher(&self); // TODO MAYBE NOT DO THIS.
    if (Current == NULL)                              // Something went really wrong.
    {
        dbg_print("DISPATCHER SOMETHING WENT WRONG");
        Current = self.fn_dispatcher(&self);
        Current->status = RUNNING;
        enableInterrupts();
        context_switch(NULL, &Current->state);
    }

    if (Current->status == READY) // Ready means its hasn't been run yet.
    {
        Current->status = RUNNING;
        context_switch(NULL, &Current->state);
    }
    else if (Current->status == BLOCKED) // Blocked means wating for another process to join.
    {
        proc_ptr old = Current;

        Current = nextProcess;
        Current->status = RUNNING;
        enableInterrupts();
        context_switch(&old->state, &Current->state);
    }
    else if (Current->status == QUIT) // Process has quit
    {
        proc_ptr old = Current; // Make sure dont set status for old

        Current = nextProcess;
        Current->status = RUNNING;
        enableInterrupts();
        context_switch(&old->state, &Current->state);
    }
    else if ((Current != nextProcess) & (Current->priority > nextProcess->priority))
    {
        proc_ptr old = Current;
        old->status = READY; // Set status back to ready for old

        Current = nextProcess;
        Current->status = RUNNING;
        enableInterrupts();
        context_switch(&old->state, &Current->state);
    }
    //
    return;
    // p1_switch(Current->pid, next_process->pid); NEED THIS LATER
} /* dispatcher */

/* ------------------------------------------------------------------------
   Name - sentinel
   Purpose - The purpose of the sentinel routine is two-fold.  One
             responsibility is to keep the system going when all other
         processes are blocked.  The other is to detect and report
         simple deadlock states.
   Parameters - none
   Returns - nothing
   Side Effects -  if system is in deadlock, print appropriate error
           and halt.
   ----------------------------------------------------------------------- */
int sentinel(char *dummy)
{
    dbg_print("sentinel(): called");

    while (1)
    {
        check_deadlock();
        waitint();
    }
    return 0;
} /* sentinel */

int zap(int pid)
{

    proc_ptr pidProc = self.fn_find_pid(&self, pid);
    if (pidProc == NULL)
        return -3;
    else if (pidProc->status == QUIT) // Process Quit already
        return -1;
    else if (pidProc == Current) // Trying to Zap itself
        return -2;
    else
        pidProc->fn_zap_add(pidProc, Current);

    dispatcher();

    return TRUE;
}
int is_zapped(void)
{
    return Current->fn_is_zapped(Current);
}
/* check to determine if deadlock has occurred... */
static void check_deadlock() // TEACHER ask if deadlock should look only for ready cpus. or maybe both blocked and ready.
{
    if (self.fn_deadlocked(&self) == TRUE) // WE HAVE DEADLOCK
    {
        dbg_print("CHECK_DEADLOCK Has DEADLOCK!\n");
        halt(1);
    }
}

void clock_handler()
{
    // TODO FINISH THIS
    // TEACHER ask for a example.
    if (Current->runningTime - sys_clock() > 80)
    {
        dbg_print("CLOCK_HANDLER: CURRENT > THAN 80 MS");
    }
}
int getpid()
{
    return Current->pid;
}
void dump_processes(void)
{
    self.fn_dbg_print_nodelist(&self);
}
int block_me(int block_status)
{
    Current->status = BLOCKED;
    return TRUE;
}
/*ENABLES INTERRUPTS*/
void enableInterrupts()
{
    // TEACHER ASK IF IM DOING THIS RIGHT.
    os_kernel_check("enableInterrupts");
    psr_set(psr_get() | PSR_CURRENT_INT);
}

/*
 * Disables the interrupts.
 */
void disableInterrupts()
{
    os_kernel_check("disableInterrupts");
    psr_set(psr_get() & ~PSR_CURRENT_INT);
} /* disableInterrupts */

/* A Check if its in kernal mode, Takes the name of the function*/
void os_kernel_check(char *func_name)
{
    union psr_values caller_psr;
    /* holds caller’s psr values */
    char buffer[200];

    dbg_print("check_kernel_mode(): called for function %s\n", func_name);
    /* test if in kernel mode;
    halt if in user mode */
    caller_psr.integer_part = psr_get();
    if (caller_psr.bits.cur_mode == 0)
    {
        sprintf(buffer, "%s(): called while in user mode, by process %d. Halting...\n", func_name, Current->pid);
        console("%s", buffer);
        halt(1);
    }
}

void dbg_print(char *_str, ...)
{
    if (DEBUG && debugflag)
    {
        va_list argptr;
        va_start(argptr, _str);
        printf("%s\n", _str);
        va_end(argptr);
    }
}
