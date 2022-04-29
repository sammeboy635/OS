#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <phase4.h>
#include <usyscall.h>
#include <provided_prototypes.h>
#include "driver.h"

extern struct main_struct sys_main;
extern struct driver_proc Driver_Table[MAXPROC];
extern int num_tracks[DISK_UNITS];

void driver_proc_init(driver_proc_ptr proc)
{
    proc->next = NULL;
    proc->next_IO = NULL;

    proc->wake_time = 0;
    proc->been_zapped = 0;

    proc->operation = 0;
    proc->unit = 0;
    proc->track_start = 0;
    proc->sector_start = 0;
    proc->num_sectors = 0;
    proc->disk_buf = NULL;

    proc->sleep_time = 0;
    proc->sem = semcreate_real(0);
    proc->status = 0;
}

void list_insert_sleep(driver_proc_ptr proc)
{
    driver_proc_ptr cur = sys_main.Sleep_Proc;

    if (cur == NULL) // Head
    {
        sys_main.Sleep_Proc = proc;
        sys_main.num_sleep++;
    }
    else
    {
        while (cur->next != NULL) // Find end
        {
            cur = cur->next;
        }
        cur->next = proc;
        sys_main.num_sleep++;
    }
    dbg_print("Entrys %d\n", sys_main.num_sleep);
}

void list_remove_sleep(driver_proc_ptr proc)
{

    driver_proc_ptr cur = sys_main.Sleep_Proc, last = NULL;

    // CRITICAL
    dbg_print("Semp_real %d\n", sys_main.sem_sleep);
    semp_real(sys_main.sem_sleep);

    if (sys_main.num_sleep == 0)
    {
        dbg_print("EMPTY\n");
    }
    else if (sys_main.num_sleep == 1)
    {
        dbg_print("ONE\n");
        sys_main.Sleep_Proc = NULL;
        sys_main.num_sleep--;
    }
    else if (sys_main.num_sleep > 1)
    {
        dbg_print("GREATER THAN ONE\n");
        if (sys_main.Sleep_Proc == proc)
        {
            sys_main.Sleep_Proc = cur->next;
            proc->next = NULL;
            sys_main.num_sleep--;
        }
        else // Find Sleeping_proc
        {
            while (cur != proc) // Find proc
            {
                last = cur;
                cur = cur->next;
            }

            last->next = cur->next;
            cur->next = NULL;
            sys_main.num_sleep--;
        }
    }

    // CRITICAL
    dbg_print("Semv_real %d\n", sys_main.sem_sleep);
    semv_real(sys_main.sem_sleep);
}

void list_insert_disk(driver_proc_ptr proc)
{
    driver_proc_ptr cur = sys_main.IO_Proc;
    if (cur == NULL) // Head
    {
        sys_main.IO_Proc = proc;
        sys_main.num_IO++;
    }
    else
    {
        while (cur->next_IO != NULL) // FIND TAIL
        {
            cur = cur->next_IO;
        }
        cur->next_IO = proc;
        sys_main.num_IO++;
    }
    dbg_print("ENTRYS %d\n", sys_main.num_IO);
}

void list_remove_disk(driver_proc_ptr proc)
{
    driver_proc_ptr cur = sys_main.IO_Proc;

    if (sys_main.num_IO == 1) // Head
    {
        sys_main.IO_Proc = NULL;
        sys_main.num_IO--;
    }
    else 
    {
        sys_main.IO_Proc = cur->next_IO;
        proc->next_IO = NULL;
        sys_main.num_IO--;
    }
    dbg_print("ENTRYS %d\n", sys_main.num_IO);
}




