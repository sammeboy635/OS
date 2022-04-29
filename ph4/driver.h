#define DISK_SIZE 8
#define TRUE 1
#define FALSE 0
#define DEBUG4 0
#define dbg_print(fmt, ...)                                                                  \
    do                                                                                       \
    {                                                                                        \
        if (DEBUG4 == 1)                                                                     \
            fprintf(stderr, "%s:%d:%s | " fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
    } while (0)

typedef struct driver_proc * driver_proc_ptr;

struct driver_proc 
{
   int   wake_time;
   int   been_zapped;

   /* Used for disk requests */
   int   operation;    /* DISK_READ, DISK_WRITE, DISK_SEEK, DISK_TRACKS */
   int   unit;
   int   track_start;
   int   sector_start;
   int   num_sectors;
   void  *disk_buf;

   //more fields to add
   int   sem;
   int   status;
   int   sleep_time;
   driver_proc_ptr next;
   driver_proc_ptr next_IO;
};

struct main_struct
{
   //System
   int sem_running;
   int sem_SY;
   int Quiting;

   //Disk
   int sem_disk;

   //Sleeping
   driver_proc_ptr Sleep_Proc;
   int num_sleep;
   int sem_sleep;

   //IO
   driver_proc_ptr IO_Proc;
   int num_IO;
   int sem_IO;
};

extern int ClockDriver(char *);
extern int DiskDriver(char *);
extern void sleep(sysargs *);
extern void disk_read(sysargs *);
extern void disk_write(sysargs *);
extern void disk_size(sysargs *);
extern void list_insert_sleep(driver_proc_ptr);
extern void list_remove_sleep(driver_proc_ptr);
extern void list_insert_disk(driver_proc_ptr);
extern void list_remove_disk(driver_proc_ptr);
extern void init();
extern int driver_write_read_check(sysargs *args);
extern void driver_proc_init(driver_proc_ptr proc);
extern int kernel_check(char *procName);