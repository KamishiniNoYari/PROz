//Compile with mpic++, run with mpirun -np <process_num> ./a.out

#include <mpi.h>
#include <stdio.h>
#include <pthread.h>
#include <queue>
#include <vector>
#include <string>
#include <unistd.h>
#include <algorithm>

//POSSIBLE MESSAGES
#define KILL_REQUEST 1

using namespace std;

int n; //liczba skrzatów - procesy utworzone
int z = -1; //liczba zleceń
int a = -1; //liczba agrafek
int t = -1; //liczba trucizn

struct request{
    int id; //id zlecenia
    int hamster; //liczba chomików to zabicia
};

struct message{
    int type; // typ wiadomości np KILL_REQUEST
    int lamport_clock; //wartość zegara lamporta
    int sender_id; //id wysyłającego
    request request_1; //request w przypadku wiadomości burmistrza
    int a_gr; //liczba agrafek
    int poison; //liczba trucizn
    int num_rq; //wybrany numer requesta
};
int tid; //Id procesu
int lamport_clock; //zegar lamporta procesu

//Sprawdz support MPI
void check_thread_support(int provided){
    switch (provided){
        case MPI_THREAD_SINGLE:
            printf("No thread support!\n");
            fprintf(stderr, "No thread support - Finalize");
            MPI_Finalize();
            exit(-1);
            break;
        case MPI_THREAD_FUNNELED:
            printf("Only threads that did mpi_init can call MPI library\n");
            break;
        case MPI_THREAD_SERIALIZED:
            printf("Only one thread at a time can make MPI library calls\n");
            break;
        case MPI_THREAD_MULTIPLE: printf("Full thread support\n"); /* Want this */
            break;
        default:
            printf("No info about thread support\n");
    }
}
void president_loop(){

    while(true){
        if(z == -1 or z == 0){

        }
    }

}
void brownie_loop(){

}

int main(int argc, char **argv){
    int provided; //provided level of security MPI_INIT_THREAD;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    check_thread_support(provided);
    MPI_Status status;

    printf("Checking!\n");
    MPI_Comm_size( MPI_COMM_WORLD, &n); // liczba procesów = wszystkie skrzaty;
    MPI_Comm_rank( MPI_COMM_WORLD, &tid); //mój id procesu
    printf("Jestem skrzatem nr %d z %d\n",tid,n);
    MPI_Finalize();
}
