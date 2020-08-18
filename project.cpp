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
#define WANT_REQUEST 2
#define REQUEST_ACCEPT 3
#define REQUEST_REJECT 4
#define WANT_A 5
#define OK_A 5


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
int lamport_clock = 0; //zegar lamporta procesu

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
        //MPI_Bcast( &mes, sizeof(mes), MPI_BYTE,0,MPI_COMM_WORLD);
        for(int j=0;j<n;j++){
            if(j != tid){
            MPI_Send( mes, sizeof(message), MPI_BYTE,j,tag, MPI_COMM_WORLD);
            }
        }
        
    }
    else{
        MPI_Send(mes, sizeof(message),MPI_BYTE,dest,tag,MPI_COMM_WORLD);
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

            

            sent(&my_message,0,KILL_REQUEST, true);
            printf("Jestem skrzatem %d. Wysyłam zlecenia w chwili %d\n", tid, lamport_clock);

        }
        MPI_Recv(&recv_message,sizeof(message), MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

    }

}
void brownie_loop(){
    message recvd; // miejsce na otrzymywaną wiadomość;
    message sentt; // miejsce na wysyłaną wiadomość;
    MPI_Status status;
    request requests[100];
    request my_actual_request;
    int y_req = 0;
    int y_req_bool = false;
    my_actual_request.id = -1;
    srand(tid);
    while(true){
        MPI_Recv(&recvd, sizeof(message), MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        switch(recvd.type){
            case KILL_REQUEST:
                lamport_clock = max(lamport_clock, recvd.lamport_clock)+1;
                printf("Jestem skrzatem %d. Otrzymałem listę zleceń od burmistrza w chwili %d\n",tid,lamport_clock);


                //Tutaj : Do zaimplementowania sekwencja wyboru zadania i oczekiwania na odpowiedzi
                
                
                //Zapisuje dane o zleceniach agrafkach truciźnie lokalnie
                z = recvd.num_rq;
                a = recvd.a_gr;
                t = recvd.poison;
                MPI_Comm_size( MPI_COMM_WORLD, &n); //liczba wszystkich procesów
                copy(begin(recvd.request_1),end(recvd.request_1),begin(requests));

                
                //Wybieram zlecenie
                
                int choice = rand()%(z+1);
                my_actual_request = requests[choice];
                requests[choice].id = -1;
                
                

                lamport_clock++;

                printf("Jestem skrzatem %d i wybrałem zlecenie %d w chwili %d\n",tid,my_actual_request.id,lamport_clock);

                //Wysyłam wiadomość WANT_REQUEST pozostałym skrzatom
                
                lamport_clock++;
                sentt.type=WANT_REQUEST;
                sentt.sender_id=tid;
                sentt.num_rq=my_actual_request.id;
                sentt.lamport_clock=lamport_clock;
                sent(&sentt,0,WANT_REQUEST,true);
                y_req = 0; //potwierdzenia że mogę wziąć dany request, wszyscy pozostali oprócz burmistrza muszą się zgodzić
                
                int temp_lamport = lamport_clock;
                while((y_req < (n-2) and y_req_bool == false)){
                //Odbieramy wiadomości zgody/niezgody bądź inne rządania

                MPI_Recv(&recvd, sizeof(message), MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                switch(recvd.type){
                    case WANT_REQUEST:
                        lamport_clock = max(lamport_clock,recvd.lamport_clock)+1;
                        printf("Jestem skrzatem %d. Odebrałem od skrzata %d informację o chęci wzięcia zlecenia %d w chwili %d\n",tid,recvd.sender_id,recvd.num_rq,lamport_clock);
                        if(recvd.num_rq != my_actual_request.id){
                            lamport_clock++;
                            requests[recvd.num_rq].id = -1;
                            sentt.type = REQUEST_ACCEPT;
                            sentt.lamport_clock = lamport_clock;
                            sentt.num_rq = recvd.num_rq;
                            sentt.sender_id = tid;
                            sent(&sentt,recvd.sender_id,REQUEST_ACCEPT,false);
                        }
                        else{
                            if(temp_lamport<recvd.lamport_clock){
                                //odsyłam rejecta
                                lamport_clock++;
                                sentt.type = REQUEST_REJECT;
                                sentt.lamport_clock = lamport_clock;
                                sentt.num_rq = recvd.num_rq;
                                sentt.sender_id = tid;
                                sent(&sentt,recvd.sender_id,REQUEST_REJECT,false); 
                                
                            }
                            else if (temp_lamport == recvd.lamport_clock)
                            {
                                if(tid < recvd.sender_id){
                                    //odsyłam rejecta
                                    lamport_clock++;
                                    sentt.type = REQUEST_REJECT;
                                    sentt.lamport_clock = lamport_clock;
                                    sentt.num_rq = recvd.num_rq;
                                    sentt.sender_id = tid;
                                    sent(&sentt,recvd.sender_id,REQUEST_REJECT,false); 
                                }
                                else{
                                    lamport_clock++;
                                    sentt.type = REQUEST_ACCEPT;
                                    sentt.lamport_clock = lamport_clock;
                                    sentt.num_rq = recvd.num_rq;
                                    sentt.sender_id = tid;
                                    sent(&sentt,recvd.sender_id,REQUEST_ACCEPT,false);

                                    
                                    //odsyłam accepta -> zmieniam zlecenie ->zmieniam zegar -> jeżeli brak zleceń to sie wycofuje

                                    //
                                    y_req = 0;
                                    my_actual_request.id = -1;
                                    for(int i=0; i<z; i++){
                                        if(requests[i].id != -1){
                                            my_actual_request = requests[i];
                                            break;
                                        }
                                    }
                                    if(my_actual_request.id = -1){
                                        y_req_bool = true;
                                    }
                                    else{
                                        lamport_clock++;
                                        printf("Jestem procesem %d. Pobrałem nowe zadanie %d w chwili %d",tid,my_actual_request.id,lamport_clock);
                                        lamport_clock++;
                                        temp_lamport = lamport_clock;
                                        sentt.lamport_clock = lamport_clock;
                                        sentt.num_rq = my_actual_request.id;
                                        sentt.sender_id = tid;
                                        sentt.type = WANT_REQUEST;
                                        sent(&sentt,recvd.sender_id,WANT_REQUEST,true);

                                    }


                                }
                            }
                            else{
                                    lamport_clock++;
                                    sentt.type = REQUEST_ACCEPT;
                                    sentt.lamport_clock = lamport_clock;
                                    sentt.num_rq = recvd.num_rq;
                                    sentt.sender_id = tid;
                                    sent(&sentt,recvd.sender_id,REQUEST_ACCEPT,false);
                                //odsyłam accepta -> zmieniam zlecenie -> zmieniam zegar -> jeżeli brak zleceń to się wycofuje
                                y_req = 0;
                                    my_actual_request.id = -1;
                                    for(int i=0; i<z; i++){
                                        if(requests[i].id != -1){
                                            my_actual_request = requests[i];
                                            break;
                                        }
                                    }
                                    if(my_actual_request.id = -1){
                                        y_req_bool = true;
                                    }
                                    else{
                                        lamport_clock++;
                                        printf("Jestem procesem %d. Pobrałem nowe zadanie %d w chwili %d",tid,my_actual_request.id,lamport_clock);
                                        lamport_clock++;
                                        temp_lamport = lamport_clock;
                                        sentt.lamport_clock = lamport_clock;
                                        sentt.num_rq = my_actual_request.id;
                                        sentt.sender_id = tid;
                                        sentt.type = WANT_REQUEST;
                                        sent(&sentt,recvd.sender_id,WANT_REQUEST,true);

                                    }
                            }
                            
                        }
                        break;
                    case REQUEST_ACCEPT:
                        //odbieram request + potwierdzam ze otrzymałem zgodę
                        lamport_clock = max(lamport_clock,recvd.lamport_clock)+1;
                        y_req++;
                        printf("Jestem skrzatem %d. Dostałem zgodę od skrzata %d na wykonanie zlecenia %d w chwili %d\n",tid,recvd.sender_id, recvd.num_rq, lamport_clock);
                        break;
                    case REQUEST_REJECT:
                        //odbieram request + aktualizuje zegar + zmieniam zlecenie + jeżeli brak zleceń to wracam do czekania na KILL_REQUEST
                        lamport_clock = max(lamport_clock,recvd.lamport_clock)+1;
                        printf("Jestem skrzatem %d. Dostałem odmowę od skrzata %d na wykonanie zlecenia %d w chwili %d\n",tid,recvd.sender_id, recvd.num_rq, lamport_clock);
                        break;

                }
                }
                if(y_req_bool==false){
                    //jeżeli mamy zgody to działamy po agrafkę w innym wypadku wyskakujemy i czekamy na nowe zlecenia
                    temp_lamport = lamport_clock;
                    lamport_clock++;
                    sentt.lamport_clock = lamport_clock;
                    sentt.sender_id=tid;
                    sentt.type=WANT_A;
                    sent(&sentt,tid,WANT_A,true);
                    int want_a_counter = 0;
                    while(want_a_counter < n-1-a){

                    }
                    

                }


                

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