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

void sleep(sysargs *args)
{
    driver_proc_ptr cur = &Driver_Table[getpid() % MAXPROC];

    int seconds = (int)args->arg1;
    if (seconds < 0)
    {
        dbg_print("Negative results\n");
        args->arg4 = (void *)-1;
        return;
    }

    // CRITICAL
    dbg_print("Semp_real %d\n", sys_main.sem_sleep);
    semp_real(sys_main.sem_sleep);

    // NEW ENTRY
    dbg_print("INSERTING\n");
    list_insert_sleep(cur);

    cur->wake_time = sys_clock();
    cur->wake_time = (seconds * 1000000) + cur->wake_time;

    // CRITICAL
    dbg_print("Semv_real %d\n", sys_main.sem_sleep);
    semv_real(sys_main.sem_sleep);

    dbg_print("Semp_real %d\n", cur->sem);
    semp_real(cur->sem);

    // Returning values
    args->arg4 = (void *)0;
}

void disk_read(sysargs *args)
{
    driver_proc_ptr cur = &Driver_Table[getpid() % MAXPROC];

    if (driver_write_read_check(args) < 0)
    {
        dbg_print("Error\n");
        args->arg1 = (void *)-1;
        args->arg4 = (void *)-1;
        return;
    }

    // CRITICAL
    dbg_print("Semp_real %d\n", sys_main.sem_IO);
    semp_real(sys_main.sem_IO);

    cur->operation = DISK_READ;
    cur->disk_buf = args->arg1;          // buffer
    cur->num_sectors = (int)args->arg2;  // num of sectors
    cur->track_start = (int)args->arg3;  // track
    cur->sector_start = (int)args->arg4; // sector
    cur->unit = (int)args->arg5;         // disk unit

    // INSERT ENTRY
    dbg_print("Inserting New Entry\n");
    list_insert_disk(cur);

    // CRITICAL
    dbg_print("Semv_real %d\n", sys_main.sem_IO);
    semv_real(sys_main.sem_IO);

    // NEW ENTRY
    dbg_print("Semv_real %d\n", sys_main.sem_disk);
    semv_real(sys_main.sem_disk);

    // DISK DRIVER
    dbg_print("Semp_real %d\n", cur->sem);
    semp_real(cur->sem);

    // Returning Values
    args->arg1 = (void *)cur->status;
    args->arg4 = (void *)0;
}

void disk_write(sysargs *args)
{
    driver_proc_ptr cur = &Driver_Table[getpid() % MAXPROC];

    if (driver_write_read_check(args) < 0)
    {
        dbg_print("Errror\n");
        args->arg1 = (void *)-1;
        args->arg4 = (void *)-1;
        return;
    }
    // CRITICAL
    dbg_print("Semp_real %d\n", sys_main.sem_IO);
    semp_real(sys_main.sem_IO);

    cur->operation = DISK_WRITE;
    cur->disk_buf = args->arg1;          // buffer
    cur->num_sectors = (int)args->arg2;  // num of sectors
    cur->track_start = (int)args->arg3;  // track
    cur->sector_start = (int)args->arg4; // sector
    cur->unit = (int)args->arg5;         // disk unit

    // INSERT NEW
    list_insert_disk(cur);

    // CRITICAL
    dbg_print("Semv_real %d\n", sys_main.sem_IO);
    semv_real(sys_main.sem_IO);

    // DISK
    semv_real(sys_main.sem_disk);

    // WAIT
    dbg_print("Semp_real %d\n", cur->sem);
    semp_real(cur->sem);

    args->arg1 = (void *)cur->status;
    args->arg4 = (void *)0;
}

void disk_size(sysargs *args)
{
    driver_proc_ptr cur = &Driver_Table[getpid() % MAXPROC];
    int unit = (int)args->arg1;

    if (unit < 0 || unit > 1)
    {
        dbg_print("Error\n");
        args->arg4 = (void *)-1;
        return;
    }

    // CRITICAL
    dbg_print("Semp_real %d\n", sys_main.sem_IO);
    semp_real(sys_main.sem_IO);

    // INSERT
    cur->operation = DISK_SIZE;
    list_insert_disk(cur);

    // CRITICAL
    dbg_print("Semv_real %d\n", sys_main.sem_IO);
    semv_real(sys_main.sem_IO);

    // ENTRY
    dbg_print("Semv_real %d\n", sys_main.sem_disk);
    semv_real(sys_main.sem_disk);

    // WAIT
    dbg_print("Semp_real %d\n", cur->sem);
    semp_real(cur->sem);

    args->arg1 = (void *)DISK_SECTOR_SIZE;
    args->arg2 = (void *)DISK_TRACK_SIZE;
    args->arg3 = (void *)num_tracks[unit];
    args->arg4 = (void *)0;
    return;
}

int driver_write_read_check(sysargs *args)
{
    int sectors = (int)args->arg2;
    int track = (int)args->arg3;
    int first = (int)args->arg4;
    int unit = (int)args->arg5;

    if (sectors < 0 || (track < 0 || track >= num_tracks[unit]) || (first < 0 || first > 15) || (unit < 0 || unit > 1))
    {
        return -1;
    }
    return 0;
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

int set_to_user_mode()
{
    psr_set(psr_get() & ~PSR_CURRENT_MODE);
    return 0;
}


