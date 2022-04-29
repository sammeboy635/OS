#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <phase4.h>
#include <usyscall.h>
#include <libuser.h>
#include <provided_prototypes.h>
#include "driver.h"



struct main_struct sys_main;
struct driver_proc Driver_Table[MAXPROC];
int num_tracks[DISK_UNITS];

int start3(char *arg)
{
    char name[128];
    char termbuf[10];
    int i;
    int clockPID;
    int pid;
    int status;

    kernel_check("start3");

    init();

    clockPID = fork1("Clock driver", ClockDriver, NULL, USLOSS_MIN_STACK, 2);
    if (clockPID < 0)
    {
        console("start3(): Can't create clock driver\n");
        halt(1);
    }

    semp_real(sys_main.sem_running);

    for (i = 0; i < DISK_UNITS; i++)
    {
        sprintf(termbuf, "%d", i);
        sprintf(name, "DiskDriver%d", i);
        pid = fork1(name, DiskDriver, termbuf, USLOSS_MIN_STACK, 2);
        if (pid < 0)
        {
            console("start3(): Can't create disk driver %d\n", i);
            halt(1);
        }
    }

    semp_real(sys_main.sem_running);
    semp_real(sys_main.sem_running);

    dbg_print("STARTING Start4\n");

    pid = spawn_real("start4", start4, NULL, 8 * USLOSS_MIN_STACK, 3);
    pid = wait_real(&status);

    // NEW
    dbg_print("QUITING\n");
    dbg_print("ZAPING %d\n", clockPID);
    zap(clockPID);
    join(&status);

    sys_main.Quiting = TRUE;

    for (i = 0; i < DISK_UNITS; i++)
    {
        if (DEBUG4)
            dump_processes();
            
        dbg_print("Semv_real %d\n", sys_main.sem_disk);
        semv_real(sys_main.sem_disk);

        dbg_print("JOINING\n");
        join(&status);
    }

    // Finished
    quit(0);
    return 0;
}

int ClockDriver(char *arg)
{
    driver_proc_ptr proc, proc_to_wake;
    int status, current_time;

    dbg_print("READY\n");

    dbg_print("Semv_real %d\n", sys_main.sem_running);
    semv_real(sys_main.sem_running);

    psr_set(psr_get() | PSR_CURRENT_INT);

    while (!is_zapped())
    {
        if (waitdevice(CLOCK_DEV, 0, &status) != 0)
        {
            return 0;
        }

        current_time = sys_clock();
        proc = sys_main.Sleep_Proc;

        while (proc != NULL)
        {
            proc_to_wake = proc;
            proc = proc->next;

            if (current_time >= proc_to_wake->wake_time) // Check Time
            {
                dbg_print("Removing entry\n");
                list_remove_sleep(proc_to_wake);

                dbg_print("Waking %d\n", proc_to_wake->sem);
                semv_real(proc_to_wake->sem);
            }
        }
    }

    quit(0);
    return 0; // Get rid of error
}

int DiskDriver(char *arg)
{
    dbg_print("STARTED\n");
    device_request my_request;
    int unit = atoi(arg);
    int status;
    int counter;
    int track;
    int sector;

    psr_set(psr_get() | PSR_CURRENT_INT);

    driver_proc_ptr current_req;

    my_request.opr = DISK_TRACKS;
    my_request.reg1 = &num_tracks[unit];

    if (device_output(DISK_DEV, unit, &my_request) != DEV_OK)
    {
        console("DiskDriver %d: did not get DEV_OK on DISK_TRACKS call\n", unit);
        console("DiskDriver %d: is the file disk%d present???\n", unit, unit);
        halt(1);
    }

    waitdevice(DISK_DEV, unit, &status);
    dbg_print("%d%d\n", unit, num_tracks[unit]);

    semv_real(sys_main.sem_running);
    dbg_print("ENTERING LOOP\n");

    while (sys_main.Quiting == FALSE)
    {
        // Wait
        dbg_print("Semp_real %d\n", sys_main.sem_disk);
        semp_real(sys_main.sem_disk);

        dbg_print("setting current_req = IO.\n");
        current_req = sys_main.IO_Proc;

        if (current_req == NULL)
        {
            dbg_print("current_req == Null");
            continue;
        }

        if (current_req->operation == DISK_SIZE)
        {
            dbg_print("Operation Disk_SIZE\n");
            // CRITICAL
            dbg_print("Semp_real %d\n", sys_main.sem_IO);
            semp_real(sys_main.sem_IO);

            list_remove_disk(current_req);

            // Critical
            dbg_print("Semv_real %d\n", sys_main.sem_IO);
            semv_real(sys_main.sem_IO);

            // Private
            dbg_print("Semv_real %d\n", current_req->sem);
            semv_real(current_req->sem);
        }
        else
        {
            dbg_print("Operation Else\n");
            // Critical IO
            dbg_print("Semp_real %d\n", sys_main.sem_IO);
            semp_real(sys_main.sem_IO);

            list_remove_disk(current_req);

            // Critical IO
            dbg_print("Semv_real %d\n", sys_main.sem_IO);
            semv_real(sys_main.sem_IO);

            // SYNC
            dbg_print("Semp_real %d\n", sys_main.sem_SY);
            semp_real(sys_main.sem_SY);

            my_request.opr = DISK_SEEK;
            my_request.reg1 = (void *)current_req->track_start;

            if (device_output(DISK_DEV, current_req->unit, &my_request) != DEV_OK)
            {
                console("DiskDriver (%d): did not get DEV_OK on 1st DISK_SEEK call\n", unit);
                console("DiskDriver (%d): is the file disk%d present???\n", unit, current_req->unit);
                halt(1);
            }

            waitdevice(DISK_DEV, (int)current_req->unit, (int *)&status);

            track = current_req->track_start;
            sector = current_req->sector_start;
            counter = 0;

            while (counter != current_req->num_sectors)
            {
                my_request.opr = (int)current_req->operation;
                my_request.reg1 = (void *)sector;
                my_request.reg2 = (void *)(&current_req->disk_buf[counter * DISK_SECTOR_SIZE]);

                if (device_output(DISK_DEV, current_req->unit, &my_request) != DEV_OK)
                {
                    console("DiskDriver (%d): did not get DEV_OK on %d call\n", unit, current_req->operation);
                    console("DiskDriver (%d): is the file disk%d present???\n", unit, current_req->unit);
                    halt(1);
                }

                waitdevice(DISK_DEV, current_req->unit, &status);
                current_req->status = status;

                counter++;
                if (counter != current_req->num_sectors)
                {

                    sector++;

                    if (sector >= DISK_TRACK_SIZE) // Out
                    {
                        track = (track + 1) % num_tracks[unit];
                        sector = 0;

                        my_request.opr = DISK_SEEK;
                        my_request.reg1 = (void *)track;

                        if (device_output(DISK_DEV, current_req->unit, &my_request) != DEV_OK)
                        {
                            console("DiskDriver (%d): did not get DEV_OK on DISK_SEEK call\n", unit);
                            console("DiskDriver (%d): is the file disk%d present???\n", unit, current_req->unit);
                            halt(1);
                        }

                        waitdevice(DISK_DEV, current_req->unit, &status);
                    }
                }
            }

            // CRITICAL
            dbg_print("Semv_real %d\n", sys_main.sem_SY);
            semv_real(sys_main.sem_SY);
        }
        // Private
        dbg_print("Semv_real %d\n", current_req->sem);
        semv_real(current_req->sem);
    }

    dbg_print("QUITING\n");
    quit(0);
    return 0; // Get rid of error
}

void init()
{
    // Function set in sys_vec
    sys_vec[SYS_SLEEP] = sleep;
    sys_vec[SYS_DISKREAD] = disk_read;
    sys_vec[SYS_DISKWRITE] = disk_write;
    sys_vec[SYS_DISKSIZE] = disk_size;

    // Process Table
    for (int i = 0; i < MAXPROC; i++)
    {
        driver_proc_init(&Driver_Table[i]);
    }
    // INIT SYS
    sys_main.Quiting = FALSE;
    sys_main.num_sleep = 0;
    sys_main.num_IO = 0;
    // Proc
    sys_main.IO_Proc = NULL;
    sys_main.Sleep_Proc = NULL;
    // Sem
    sys_main.sem_sleep = semcreate_real(1);
    dbg_print("sys_main.sem_sleep = %d\n", sys_main.sem_sleep);
    sys_main.sem_disk = semcreate_real(0);
    dbg_print("sys_main.sem_disk = %d\n", sys_main.sem_disk);
    sys_main.sem_IO = semcreate_real(1);
    dbg_print("sys_main.sem_IO = %d\n", sys_main.sem_IO);
    sys_main.sem_running = semcreate_real(0);
    dbg_print("sys_main.sem_running = %d\n", sys_main.sem_running);
    sys_main.sem_SY = semcreate_real(1);
    dbg_print("sys_main.sem_SY = %d\n", sys_main.sem_SY);
}
