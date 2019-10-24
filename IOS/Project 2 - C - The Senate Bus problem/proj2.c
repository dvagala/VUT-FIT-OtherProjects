#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>


#define MMAP(pointer) {(pointer) = mmap(NULL, sizeof(*(pointer)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);}
#define UNMAP(pointer) {munmap((pointer), sizeof((pointer)));}

#define SEMAPHORE1_NAME "/sem1"
#define SEMAPHORE2_NAME "/sem2"
#define SEMAPHORE3_NAME "/sem3"
#define SEMAPHORE4_NAME "/sem4"
#define SEMAPHORE5_NAME "/sem5"
#define SEMAPHORE6_NAME "/sem6"

sem_t *all_aboard = NULL;       // is 1 if all riders that wait on stop enter the bus
sem_t *mutex = NULL;            // mutex for riders that go to stop and for bus itself
sem_t *bus= NULL;               // lockable turnstile, when bus came it unlocks and let riders one by one enter
sem_t *multiplex= NULL;         // upper limit, how many riders fit in the bus, also how many can wait on stop
sem_t *bus_end= NULL;           // lockable turnstile, when bus print "end" it unlocks and all riders print "finish", one by one
sem_t *all_rid_finish= NULL;    // is 1 if all riders on bus end ride with bus, so if all of them print finish

int *share_A = NULL;
int *share_rid_waiting = NULL;
int *share_rid_on_bus = NULL;
int *share_all_riders_gone = NULL;

FILE *out_file;


int init(int bus_capacity)
{
    MMAP(share_A);
    MMAP(share_rid_waiting);
    MMAP(share_rid_on_bus);
    MMAP(share_all_riders_gone);

    setbuf(out_file, NULL);
    srand(time(NULL));

    *share_A = 0;
    *share_rid_waiting = 0;
    *share_rid_on_bus = 0;
    *share_all_riders_gone = 0;

    all_aboard = sem_open(SEMAPHORE1_NAME, O_CREAT | O_EXCL, 0666, 0);
    mutex = sem_open(SEMAPHORE2_NAME, O_CREAT | O_EXCL, 0666, 1);
    bus = sem_open(SEMAPHORE3_NAME, O_CREAT | O_EXCL, 0666, 0);
    multiplex = sem_open(SEMAPHORE4_NAME, O_CREAT | O_EXCL, 0666, bus_capacity);
    bus_end = sem_open(SEMAPHORE5_NAME, O_CREAT | O_EXCL, 0666, 0);
    all_rid_finish = sem_open(SEMAPHORE6_NAME, O_CREAT | O_EXCL, 0666, 0);

    if(errno != 0)
        return 1;

    return 0;
}

int clean_all()
{
    UNMAP(share_A);
    UNMAP(share_rid_waiting);
    UNMAP(share_rid_on_bus);
    UNMAP(share_all_riders_gone);

    sem_close(all_aboard);
    sem_close(mutex);
    sem_close(bus);
    sem_close(multiplex);
    sem_close(bus_end);
    sem_close(all_rid_finish);

    sem_unlink(SEMAPHORE1_NAME);
    sem_unlink(SEMAPHORE2_NAME);
    sem_unlink(SEMAPHORE3_NAME);
    sem_unlink(SEMAPHORE4_NAME);
    sem_unlink(SEMAPHORE5_NAME);
    sem_unlink(SEMAPHORE6_NAME);

    if(fclose(out_file) != 0)
        return 1;

    return 0;
}


void error_kill_n_clean()
{
    clean_all();
    while(wait(NULL) > 0);
    exit(1);
}


void rider_process(int rid_id)
{
    char *name = "RID";

    sem_wait(mutex);
    fprintf(out_file, "%d\t\t: %s %d\t\t: start\n", ++(*share_A), name, rid_id);
    sem_post(mutex);

    sem_wait(multiplex);
    sem_wait(mutex);
    (*share_rid_waiting)++;
    fprintf(out_file, "%d\t\t: %s %d\t\t: enter:  %d\n", ++(*share_A), name, rid_id, *share_rid_waiting);
    sem_post(mutex);

    sem_wait(bus);
    sem_post(multiplex);
    fprintf(out_file, "%d\t\t: %s %d\t\t: boarding\n", ++(*share_A), name, rid_id);
    (*share_rid_waiting)--;
    (*share_rid_on_bus)++;
    if(*share_rid_waiting == 0)
        sem_post(all_aboard);
    else
        sem_post(bus);

    sem_wait(bus_end);
    sem_wait(mutex);
    fprintf(out_file, "%d\t\t: %s %d\t\t: finish\n", ++(*share_A), name, rid_id);
    sem_post(mutex);
    (*share_rid_on_bus)--;
    if(*share_rid_on_bus == 0)
        sem_post(all_rid_finish);
    else
        sem_post(bus_end);
}

void bus_process(int ABT)
{
    char *name = "BUS";
    fprintf(out_file, "%d\t\t: %s\t\t: start\n", ++(*share_A), name );

    while(*share_all_riders_gone != 1)
    {
        sem_wait(mutex);
        fprintf(out_file, "%d\t\t: %s\t\t: arrival\n", ++(*share_A), name );

        if(*share_rid_waiting > 0)
        {
            fprintf(out_file, "%d\t\t: %s\t\t: start boarding:  %d\n", ++(*share_A), name, *share_rid_waiting );

            sem_post(bus);
            sem_wait(all_aboard);

            fprintf(out_file, "%d\t\t: %s\t\t: end boarding:  %d\n", ++(*share_A), name, *share_rid_waiting );
        }

        fprintf(out_file, "%d\t\t: %s\t\t: depart\n", ++(*share_A), name );
        sem_post(mutex);

        if(ABT != 0)
            usleep(rand() % ABT);

        sem_wait(mutex);
        fprintf(out_file, "%d\t\t: %s\t\t: end\n", ++(*share_A), name );
        sem_post(mutex);

        if(*share_rid_on_bus > 0)
        {
            sem_post(bus_end);
            sem_wait(all_rid_finish);
        }

    }

    fprintf(out_file, "%d\t\t: %s\t\t: finish\n", ++(*share_A), name );

}

int gen_riders(int R, int ART)
{
    signal(SIGTERM, error_kill_n_clean);

    for (int i = 1; i <= R; ++i)
    {
        /**Rider processes*/
        pid_t rider_proc = fork();
        if(rider_proc == 0)
        {
            rider_process(i);
            exit(EXIT_SUCCESS);
        } else if (rider_proc < 0)
        {
            kill(getppid(), SIGTERM);
            exit(EXIT_FAILURE);
        }

        if(ART != 0)
            usleep(rand() % ART);
    }

    while(wait(NULL) > 0);

    *share_all_riders_gone = 1;

    return 0;
}


int main(int argc, char *argv[]) {

    out_file = fopen("proj2.out", "w");
    if(out_file == NULL)
    {
        fprintf(stderr, "error!\n");
        return 1;
    }

    if(argc != 5)
    {
        fprintf(stderr, "wrong arguments!\n");
        return 1;
    }

    int R = (int) strtol(argv[1], NULL, 10);
    int C = (int) strtol(argv[2], NULL, 10);
    int ART = (int) strtol(argv[3], NULL, 10);
    int ABT = (int) strtol(argv[4], NULL, 10);

    if(ABT < 0 || ABT > 1000 || ART < 0 || ART > 1000 || C <= 0 || R <= 0)
    {
        fprintf(stderr, "wrong arguments!\n");
        return 1;
    }

    ABT *= 1000;        // I use usleep, so convert to micro
    ART *= 1000;

    if(init(C) == 1)
    {
        error_kill_n_clean();
    }


    /**Riders generator process */
    pid_t generator_proc = fork();
    if(generator_proc == 0)
    {
        gen_riders(R, ART);
        exit(EXIT_SUCCESS);
    } else if(generator_proc < 0)
    {
        error_kill_n_clean();
    }


    /**Bus process */
    pid_t bus_proc = fork();
    if(bus_proc == 0)
    {
        bus_process(ABT);
        exit(EXIT_SUCCESS);
    } else if(bus_proc < 0)
        error_kill_n_clean();

    while(wait(NULL) > 0);

    if(clean_all() == 1)
    {
        fprintf(stderr, "error!\n");
        return 1;
    }

    return 0;
}

