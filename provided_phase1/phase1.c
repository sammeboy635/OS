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
void clock_handler(int dev, void *unit);

/* -------------------------- Globals ------------------------------------- */
// test03
// XXp1(): after zap'ing first child, status = 1

// test03 ms counter is not fully working
// test10 maybe?
// test11 should I not free if halted by kernel?
// test12 ?
// test17 global vars

proc_list self;                      // Processlist
proc_ptr Current;                    // Current Process
unsigned int next_pid = SENTINELPID; // Next pid

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
    Current = NULL;
    int result; // Returned from forks
    init_proc_list(&self);
    int_vec[CLOCK_INT] = clock_handler;

    os_kernel_check("__START__");
    dbg_print("startup(): initializing the Ready & Blocked lists");
    dbg_print("startup(): calling fork1() for sentinel");

    /* startup a sentinel process */
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
    disableInterrupts();
    os_kernel_check("__FORK1__"); // halts if not in kernel
    dbg_print("fork1(): creating process %s\n", name);

    if (self.processSize >= MAXPROC) // TO MANY PROCESS IN THE LIST
    {
        dbg_print("%s) Just tried forking with no processes avaible", Current->name);
        return -1;
    }

    if (arg == NULL)
        arg = "\0";
    if (strlen(name) >= MAXNAME - 1 || strlen(arg) >= (MAXARG - 1))
    {
        dbg_print("fork1(): Process name or Arg is too long.  Halting...\n");
        halt(1);
    }
    else if (stacksize < USLOSS_MIN_STACK) // Return if stack is to small TODO = CHeck if the stack is to big
    {
        dbg_print("%s) Stack Size Way to big", Current->name);
        return -2;
    }
    else if (priority < MAXPRIORITY || priority > MINPRIORITY) // PRIORITY NOT WITHIN RANGE
    {
        if (priority != SENTINELPRIORITY) // IF NOT SENTINEL
        {
            dbg_print("fork1: priority out of range");
            return -1;
        }
    }

    // Inits new process
    int currentPid = next_pid;
    proc_ptr newProcess = init_proc_ptr(name, f, arg, stacksize, priority, &next_pid, launch);

    if (strcmp("sentinel", name) == 0) // IF SENTINEL SET CURRENT
        Current = newProcess;
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

    if (newProcess->pid != SENTINELPID) // Dont do sentinelPid
        dispatcher();
    return currentPid;
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
    if (Current == NULL) // Just making sure its never NULL
    {
        dbg_print("Launch: Current is NULL");
        halt(1);
    }
    int result;

    dbg_print("launch(): started");

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
    proc_ptr child = Current->fn_child_next(Current); // Grabs the child in order they came in
    if (child == NULL)                                // NO CHILD PROCESS if NULL
    {
        dbg_print("%s) Tried to join with no child\n", Current->name);
        *code = -2;
        return -2;
    }
    else if (child->fn_is_zapped(child) == TRUE)
    {
        *code = child->quitCode;
        return -1;
    }
    else if (child->status == QUIT) // CHILD QUIT
    {
        // Current->childJoinCount++;
        Current->fn_child_remove(Current, child);
        self.fn_remove_proc(&self, child);
        *code = child->quitCode;
        return child->pid;
    }
    child->zapList->fn_push(child->zapList, Current); // adding parent to child list
    Current->status = BLOCKED;
    dispatcher();

    Current->fn_child_remove(Current, child);
    self.fn_remove_proc(&self, child);

    enableInterrupts();
    // Current->childJoinCount++;
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
    disableInterrupts();

    Current->fn_time_end_of_run_set(Current);

    Current->status = QUIT;
    Current->quitCode = code;
    dbg_print("PID) %d just quit", Current->pid);
    self.processSize--;

    if (Current->fn_child_active(Current) == TRUE) // Halt if there is active children
    {
        console("quit(): process %s quit with active children. Halting...", Current->name);
        halt(1);
    }

    // Testing out new code
    // if (Current->fn_is_zapped(Current) == TRUE)  // Teacher can a process be joined as well as zapped
    //     Current->fn_unblock_zapped(Current);     // Alerts other process of zapped process
    // else if (Current->parent->status == BLOCKED) // Determine if process parent was joined
    //     Current->parent->status = READY;         // CURRENT PROCESS IS JOINED
    if (Current->zapList->length > 0)
        Current->fn_unblock_zapped(Current);
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
    disableInterrupts();

    if (Current == NULL) // Something went really wrong.
    {
        dbg_print("DISPATCHER SOMETHING WENT WRONG");
        halt(1);
    }
    else if (Current->status == READY) // Ready means its hasn't been run yet.
    {
        Current->status = RUNNING;
        Current->fn_time_end_of_run_set(Current);
        context_switch(NULL, &Current->state);
    }
    else if (Current->status == BLOCKED) // Blocked means wating for another process to join.
    {                                    // MAKE SURE YOU DONT CHANGE THE BLOCK STATUS
        proc_ptr nextProcess = self.fn_dispatcher(&self);
        proc_ptr old = Current;
        old->fn_time_end_of_run_set(old);
        nextProcess->status = RUNNING;
        nextProcess->fn_time_start_set(nextProcess);
        Current = nextProcess;

        enableInterrupts();
        context_switch(&old->state, &nextProcess->state);
    }
    else if (Current->status == QUIT) // Process has quit
    {                                 // DONT CHANGE CURRENT STATUS
        proc_ptr nextProcess = self.fn_dispatcher(&self);
        proc_ptr old = Current;
        old->fn_time_end_of_run_set(old);
        nextProcess->status = RUNNING;
        nextProcess->fn_time_start_set(nextProcess);
        Current = nextProcess;

        enableInterrupts();
        context_switch(&old->state, &nextProcess->state);
    }
    proc_ptr nextProcess = self.fn_dispatcher(&self);
    if ((Current != nextProcess) & (Current->priority > nextProcess->priority))
    { // A BETTER PROCESS HAS BEEN FOUND
        proc_ptr old = Current;
        old->fn_time_end_of_run_set(old);
        old->status = READY;
        nextProcess->status = RUNNING;
        nextProcess->fn_time_start_set(nextProcess);
        Current = nextProcess;

        enableInterrupts();
        context_switch(&old->state, &nextProcess->state);
    }
    else if ((Current->fn_time_ready_to_quit(Current)) & (Current != nextProcess) & (Current->priority >= nextProcess->priority)) // Went over time
    {                                                                                                                             // TIME SLICED
        proc_ptr old = Current;
        old->fn_time_end_of_run_set(old);
        old->status = READY;
        nextProcess->status = RUNNING;
        nextProcess->fn_time_start_set(nextProcess);
        Current = nextProcess;

        enableInterrupts();
        context_switch(&old->state, &nextProcess->state);
    }
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
    {
        dbg_print("ZAP: NO process with that pid");
        console("zap(): process being zapped does not exist.  Halting...\n");
        halt(1);
    }
    else if (pidProc->status == QUIT) // Process Quit already
    {
        dbg_print("ZAP: %s: Process already QUIT", pidProc->name);
        return 0;
    }
    else if (pidProc->status == BLOCKED)
    {
        dbg_print("ZAP: %s: Tried zapping a blocked process", pidProc->name);
        pidProc->fn_zap_add(pidProc, Current);
    }
    else if (Current->parent == pidProc)
    {
        console("Trying to zap parent.");
        halt(1);
    }
    else if (pidProc->fn_is_zapped(pidProc) == FALSE & pidProc->fn_zap_count(pidProc) == 1)
    {
        pidProc->pid = -1;
        return 0;
    }
    else if (pidProc == Current) // Trying to Zap itself
    {
        console("zap(): process %d tried to zap itself.  Halting...", Current->pid);
        halt(1);
        return -2;
    }
    else
    {
        dbg_print("ZAP: %s: WAS ZAPPED", pidProc->name);
        pidProc->fn_zap_add(pidProc, Current);
    }
    dispatcher();

    return 0; // Changed from True
}
int is_zapped(void)
{
    return Current->fn_is_zapped(Current);
}

int block_me(int block_status) // Teacher What is this status for?
{
    if (Current == NULL)
        dbg_print("BLOCK_ME: RECIEVED NULL CURRENT");

    Current->quitCode = block_status;
    Current->status = BLOCKED;

    dispatcher();

    if (Current->fn_is_zapped(Current) || Current->fn_zap_count(Current) != 0)
        return -1;
    return 0;
}
int unblock_proc(int pid) // Teacher what do I return here?
{

    proc_ptr pidProc = self.fn_find_pid(&self, pid);
    if (pidProc == NULL) // NO SUCH PROCESS
        return -3;

    // MAYBE NEED IS ZAPPED

    switch (pidProc->status)
    {
    case READY:
        dbg_print("UNBLOCK_PROC: CANT UNBLOCK THE READY PROCESSES");
        return -3;
        break;
    case RUNNING:
        dbg_print("UNBLOCK_PROC: TRYING TO UNBLOCK YOURSELF I SEE :)");
        return -3;
        break;
    case BLOCKED:
        dbg_print("UNBLOCK_PROC: UNBLOCKING PROCESS");
        pidProc->status = READY;
        break;
    case QUIT:
        return -3;
        dbg_print("UNBLOCK_PROC: PROCESS ALREADY QUIT");
        break;
    default:
        dbg_print("UNBLOCK_PROC: HIT DEFAULT");
        return -3;
        break;
    }
    // if (pidProc->fn_is_zapped(pidProc) || pidProc->fn_zap_count(pidProc) != 0)

    dispatcher();
    return 0;
}

/* check to determine if deadlock has occurred... */
static void check_deadlock() // TEACHER ask if deadlock should look only for ready cpus. or maybe both blocked and ready.
{
    if (self.fn_deadlocked(&self) == TRUE) // WE HAVE DEADLOCK
    {
        dbg_print("CHECK_DEADLOCK: Has DEADLOCK!\n");
        if (self.processSize > 1)
        {
            console("check_deadlock(): num_proc = %d\n", self.processSize);
            console("check_deadlock(): processes still present.  Halting...\n");
            halt(1);
        }
        self.fn_free(&self);
        halt(1);
    }
}

void clock_handler(int dev, void *unit)
{
    if (Current->fn_time_ready_to_quit(Current))
    {
        dbg_print("CLOCK_HANDLER: CURRENT > THAN 80 MS");
        dispatcher();
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
/*ENABLES INTERRUPTS*/
void enableInterrupts()
{
    os_kernel_check("enableInterrupts");
    psr_set(psr_get() | PSR_CURRENT_INT);
}

/*Disables the interrupts.*/
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

    // dbg_print("check_kernel_mode(): called for function %s", func_name);

    /* test if in kernel mode; halt if in user mode */
    caller_psr.integer_part = psr_get();
    if (caller_psr.bits.cur_mode == 0)
    {
        sprintf(buffer, "%s(): called while in user mode, by process %d. Halting...\n", func_name, Current->pid);
        console("%s", buffer);
        // self.fn_free(&self);
        // psr_set(psr_get() | PSR_CURRENT_MODE);
        halt(1);
    }
}
void dbg_print(char *_str, ...)
{
    if (DEBUG == 1) // TEACHER ASK ABOUT VARS NOT PRINTING RIGHT
    {
        va_list argptr;
        va_start(argptr, _str);
        console(_str, argptr);
        va_end(argptr);
    }
}
