//Compile with mpic++, run with mpirun -np <process_num> ./a.out

#include <mpi.h>
#include <stdio.h>
#include <pthread.h>
#include <queue>
#include <vector>
#include <string>
#include <unistd.h>
#include <algorithm>
#include <cstdlib>

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
    request request_1[100]; //request w przypadku wiadomości burmistrza
    int a_gr; //liczba agrafek
    int poison; //liczba trucizn
    int num_rq; //wybrany numer requesta
};

//message my_message;
int tid; //Id procesu
int lamport_clock = -1; //zegar lamporta procesu

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
void sent(message *mes, int dest, int tag, bool is_brodecast = false){
    if(is_brodecast){
        MPI_Bcast( &mes, sizeof(mes), MPI_BYTE,0,MPI_COMM_WORLD);
    }
}
request generate_request(int id){
    request final_request;
    final_request.id = id;
    final_request.hamster = rand() % 10 + 1;
    return final_request;
}
void president_loop(){
    int hamsters_to_kill = 0;
    message my_message;
    message recv_message;
    MPI_Status status;
    while(true){
        if(z == -1 or z == 0){
            //message my_message;
            z = rand() % 20 + 1;
            for(int i = 0; i < z; i++){
                request generated_request = generate_request(i);
                hamsters_to_kill += generated_request.hamster;
                my_message.request_1[i] = generated_request;
            }
            printf("Utworzyłem zlecenia. Jestem skrzatem %d o numerze zegara %d\n",tid,lamport_clock);

            lamport_clock++;
            my_message.type = 1;
            my_message.lamport_clock = lamport_clock;
            my_message.sender_id = tid;
            a = rand() % 20 + 1;
            my_message.a_gr = a;
            my_message.num_rq = z; // w tym wypadku czyli liczba zleceń bo nie wybieramy żadnego
            t = rand() % 10 + 10;
            my_message.poison = t;

            printf("Jestem skrzatem %d. Wysyłam zlecenia w chwili %d\n", tid, lamport_clock);

            sent(&my_message,0,KILL_REQUEST, true);

        }
        MPI_Recv(&recv_message,sizeof(recv_message), MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

    }

}
void brownie_loop(){
    message recvd; // miejsce na otrzymywaną wiadomość;
    message sentt; // miejsce na wysyłaną wiadomość;
    MPI_Status status;

    while(true){
        MPI_Recv(&recvd, sizeof(recvd), MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        switch(recvd.type){
            case KILL_REQUEST:
                lamport_clock = max(lamport_clock, recvd.lamport_clock)+1;
                printf("Jestem skrzatem %d. Otrzymałem listę zleceń od burmistrza w chwili %d\n",tid,lamport_clock);
        }
    }


}

int main(int argc, char **argv){
    int provided; //provided level of security MPI_INIT_THREAD;
    srand(time(NULL));
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    check_thread_support(provided);
    MPI_Status status;

    printf("Checking!\n");
    MPI_Comm_size( MPI_COMM_WORLD, &n); // liczba procesów = wszystkie skrzaty;
    MPI_Comm_rank( MPI_COMM_WORLD, &tid); //mój id procesu

    printf("Jestem skrzatem nr %d z %d\n",tid+1,n);

    if(tid == 0)president_loop();
    else brownie_loop();

    MPI_Finalize();
}
