#include <mpi.h>
#include <stdio.h>
#include <pthread.h>
#include <queue>
#include <vector>
#include <string>
#include <unistd.h>
#include <algorithm>
#include <cstdlib>

using namespace std;

#define KILL_REQUEST 1
#define WANT_REQUEST 2
#define REQUEST_ACCEPT 3
#define REQUEST_REJECT 4
#define WANT_A 5
#define OK_A 6
#define WANT_POISON 7
#define OK_POISON 8
#define REQUEST_FINISHED 9


int n; //liczba skrzatów - procesy utworzone
int z; //liczba zleceń
int a; //liczba agrafek
int t; //liczba trucizn

int real_status = 0; 

struct request{
    int id;
    int hamsters;
};

struct message{
    int type;
    int lamport_clock;
    int lamport_ack; // czas wirtualny wiadomości na którą odpowiadamy 
    int sender_id;
    int a_gr;
    int num_rq;
    int poison;
    request requests[3];
};


int tid; //Id procesu
int lamport_clock = 0; //zegar lamporta procesu

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
            //printf("PROCES: %d, DO: %d ,WIADOMOŚĆ: %d\n",tid,j, mes->type);
            MPI_Send( mes, sizeof(message), MPI_BYTE,j,tag, MPI_COMM_WORLD);
            }
        }
        
    }
    else{
        //printf("PROCES: %d, DO: %d ,WIADOMOŚĆ: %d\n",tid,dest, mes->type);
        MPI_Send(mes, sizeof(message),MPI_BYTE,dest,tag,MPI_COMM_WORLD);
    }
}
void brownie_loop(){
    message send;
    message receive;
    MPI_Status status;
    int rq_acc = 0;
    int agr_acc = 0;
    int toxic_acc = 0;
    int rq_not = 0;
    
    bool send_request = false;

    
    vector<pair<int,int>> not_give_a;
    vector<pair<int,int>> not_give_p;
    
    request received_requests[3];

    request my_request;
    
    vector<message> want_requests;

    int get_requests = 0; 
    
    int temporary_lamport = 0;
    
    int got_requests = 0;

    vector<message> matching;

    vector<message> temporary_vec;
    int free_poison = 0; //trucizna ze zwolnionych zleceń przed moim zleceniem w kolejce
    int after_me = 0; //trucizna ze zleceń za mną


    printf("CLOCK: %d PROCES:%d - Oczekuje na zlecenia\n",lamport_clock, tid);


    while(true){
        MPI_Recv(&receive,sizeof(message),MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        lamport_clock = max(lamport_clock, receive.lamport_clock)+1;
        switch (receive.type)
        {
        case KILL_REQUEST:
            MPI_Comm_size( MPI_COMM_WORLD, &n);
            a = receive.a_gr;
            z = receive.num_rq;
            t = receive.poison;
            copy(begin(receive.requests),end(receive.requests),begin(received_requests));

            rq_acc = 0;
            agr_acc = 0;
            toxic_acc = 0;
            rq_not = 0;
            after_me = 0;
                        
            free_poison = 0;

            get_requests = receive.lamport_clock;

            lamport_clock++;

            send.lamport_clock = lamport_clock;
            send.lamport_ack = get_requests;
            send.type = WANT_REQUEST;
            send.sender_id = tid;
            sent(&send,0,WANT_REQUEST,true);

            
            got_requests = lamport_clock; // moment wysłania WANT_REQUEST
            
            

            for(auto &i:want_requests){
                if(i.lamport_ack == get_requests){
                    matching.push_back(i);
                }
            }
            //printf("Moment odebrania KILL_REQUESTU :%d PROCES:%d\n",get_requests,tid);
            if(matching.size() == n-2){
                int pass = 0;
                for(auto &j:matching){
                    if(j.lamport_clock > got_requests){
                        pass++;
                    }
                    else if(j.lamport_clock == got_requests){
                        if(tid < j.sender_id){
                            pass++;
                        }
                    }

                }
                matching.clear();
                if(pass > n - 2 - z){
                    my_request = received_requests[n-2-pass];
                    printf("CLOCK: %d PROCES:%d ZADANIE:%d - Otrzymałem zadanie. Oczekuje na agrafkę\n",lamport_clock, tid, my_request.id);
                    lamport_clock++;

                    real_status = 1;
                    temporary_lamport = lamport_clock;
                    send.lamport_clock = lamport_clock;
                    send.type = WANT_A;
                    send.sender_id = tid;
                    
                    sent(&send,0,WANT_A,true);
                    
                    temporary_vec.clear();
                    //usuwam z vektora want_requests to co było w matching
                    for(auto &i:want_requests){
                        if(i.lamport_ack != get_requests){
                        temporary_vec.push_back(i);
                        }
                    }
                    want_requests.clear();
                    want_requests = temporary_vec;
                    temporary_vec.clear();




                }
            }
            matching.clear();





            

            
            break;
        case WANT_REQUEST:
            want_requests.push_back(receive);
            if(real_status == 0){
                //vector<message> matching;

                for(auto &i:want_requests){
                    if(i.lamport_ack == get_requests){
                    matching.push_back(i);
                }
            }
            //printf("Moment odebrania KILL_REQUESTU :%d PROCES:%d\n",get_requests,tid);
            if(matching.size() == n-2){
                int pass = 0;
                for(auto &j:matching){
                    
                    if(j.lamport_clock > got_requests){
                        pass++;
                    }
                    else if(j.lamport_clock == got_requests){
                        if(tid < j.sender_id){
                            pass++;
                        }
                    }

                }
                //matching.clear();
                if(pass > n - 2 - z){
                    my_request = received_requests[n-2-pass];
                    printf("CLOCK: %d PROCES:%d ZADANIE:%d - Otrzymałem zadanie. Oczekuje na agrafkę\n",lamport_clock, tid, my_request.id);
                    lamport_clock++;

                    real_status = 1;
                    temporary_lamport = lamport_clock;

                    send.lamport_clock = lamport_clock;
                    send.type = WANT_A;
                    send.sender_id = tid;
                    
                    sent(&send,0,WANT_A,true);

                    temporary_vec.clear();
                    //usuwam z vektora want_requests to co było w matching
                    for(auto &i:want_requests){
                        if(i.lamport_ack != get_requests){
                        temporary_vec.push_back(i);
                        }
                    }
                    want_requests.clear();
                    want_requests = temporary_vec;
                    temporary_vec.clear();
            }
                    





                }
            }
            matching.clear();
            
            break;
        case WANT_A:
            if(real_status == 1){
                if(temporary_lamport > receive.lamport_clock){
                    lamport_clock++;

                    send.type = OK_A;
                    send.sender_id = tid;
                    send.lamport_clock = lamport_clock;
                    send.lamport_ack = receive.lamport_clock;

                    sent(&send,receive.sender_id,OK_A,false);
                    
                }
                else if(temporary_lamport == receive.lamport_clock){
                    if(tid < receive.sender_id){
                        not_give_a.push_back(make_pair(receive.sender_id,receive.lamport_clock));


                    }
                    else{
                        lamport_clock++;

                        send.type = OK_A;
                        send.sender_id = tid;
                        send.lamport_clock = lamport_clock;
                        send.lamport_ack = receive.lamport_clock;

                        sent(&send,receive.sender_id,OK_A,false);
                    }
                }
                else{
                        not_give_a.push_back(make_pair(receive.sender_id,receive.lamport_clock));
                }
            }
            else if(real_status == 2){
                not_give_a.push_back(make_pair(receive.sender_id,receive.lamport_clock));
                //printf("%d\n",tid);
            }
            else{
                lamport_clock++;

                send.type = OK_A;
                send.sender_id = tid;
                send.lamport_clock = lamport_clock;
                send.lamport_ack = receive.lamport_clock;

                sent(&send, receive.sender_id, OK_A, false);
            }
            break;
        case OK_A:
            if(real_status == 1 && temporary_lamport == receive.lamport_ack){
                
                agr_acc++;

                

                if(agr_acc > n - 2 - a){
                    printf("CLOCK: %d PROCES:%d - Otrzymałem agrafkę. Oczekuje na truciznę\n",lamport_clock, tid);
                    real_status = 2;
                    lamport_clock++;

                    //sprawdzam czy do tej pory bylo dosc zwolnionej trucizny i ustawiam to co za mną

                    for(int i = 0; i < z; i++){
                        after_me+=received_requests[i].hamsters;
                        if(received_requests[i].id == my_request.id) break;
                    }
                    if(after_me - free_poison <= t){
                        printf("CLOCK: %d PROCES:%d - Otrzymałem truciznę. Wykonuję zadanie\n",lamport_clock, tid);
                        sleep(1);

                        lamport_clock++;

                        printf("CLOCK: %d PROCES:%d - Wykonałem zlecenie.\n",lamport_clock, tid);

                        real_status = 0;
                    
                        toxic_acc = 0;

                        agr_acc = 0;

                        rq_acc = 0;

                        after_me = 0;
                        
                        free_poison = 0;

                        lamport_clock++;

                        send.type = REQUEST_FINISHED;
                        send.sender_id = tid;
                        send.lamport_clock = lamport_clock;
                        send.lamport_ack = get_requests;
                        send.num_rq = my_request.id;

                        sent(&send,0,REQUEST_FINISHED,true);

                        lamport_clock++;
                    
                        for(auto &i:not_give_a){
                            send.type = OK_A;
                            send.sender_id = tid;
                            send.lamport_clock = lamport_clock;
                            send.lamport_ack = i.second;

                        sent(&send,i.first,OK_A,false);
                        }
                        not_give_a.clear();

                    }

                }
            }
            break;
        case WANT_POISON:
            
            break;
        case OK_POISON:
            if(real_status == 2 && temporary_lamport == receive.lamport_ack){
                toxic_acc++;
                if(toxic_acc > n - 2 - t){
                    printf("CLOCK: %d PROCES:%d - Otrzymałem truciznę. Wykonuję zadanie\n",lamport_clock, tid);

                    sleep(1);

                    lamport_clock++;

                    printf("CLOCK: %d PROCES:%d - Wykonałem zlecenie.\n",lamport_clock, tid);

                    real_status = 0;
                    
                    toxic_acc = 0;

                    agr_acc = 0;

                    rq_acc = 0;

                    //z--;

                    lamport_clock++;

                    send.type = REQUEST_FINISHED;
                    send.sender_id = tid;
                    send.lamport_clock = lamport_clock;
                    

                    sent(&send,0,REQUEST_FINISHED,true);

                    lamport_clock++;
                    
                    for(auto &i:not_give_a){
                        send.type = OK_A;
                        send.sender_id = tid;
                        send.lamport_clock = lamport_clock;
                        send.lamport_ack = i.second;

                        sent(&send,i.first,OK_A,false);
                    }

                    not_give_a.clear();

                    for(auto &i:not_give_p){
                        send.type = OK_POISON;
                        send.sender_id = tid;
                        send.lamport_clock = lamport_clock;
                        send.lamport_ack = i.second;
                        
                        sent(&send, i.first, OK_POISON, false);

                    }
                    
                    not_give_p.clear();
                    send_request = false;



                    

                }


            }
            break;
        case REQUEST_FINISHED:
            //printf("PROCES:%d REQUEST:%d\n",tid, receive.num_rq);
            if(real_status != 0){
                //printf("PROCES:%d REQUEST:%d\n",tid, receive.num_rq);
                if(receive.num_rq < my_request.id){
                    
                    free_poison += received_requests[receive.num_rq].hamsters;
                    
                    if(real_status == 2 && after_me - free_poison <= t ){
                        printf("CLOCK: %d PROCES:%d - Otrzymałem truciznę. Wykonuję zadanie\n",lamport_clock, tid);
                        sleep(1);

                        lamport_clock++;

                        printf("CLOCK: %d PROCES:%d - Wykonałem zlecenie.\n",lamport_clock, tid);

                        real_status = 0;
                    
                        toxic_acc = 0;

                        agr_acc = 0;

                        rq_acc = 0;

                        after_me = 0;
                        
                        free_poison = 0;

                        lamport_clock++;

                        send.type = REQUEST_FINISHED;
                        send.sender_id = tid;
                        send.lamport_clock = lamport_clock;
                        send.lamport_ack = get_requests;
                        send.num_rq = my_request.id;

                        sent(&send,0,REQUEST_FINISHED,true);

                        lamport_clock++;
                    
                        for(auto &i:not_give_a){
                            send.type = OK_A;
                            send.sender_id = tid;
                            send.lamport_clock = lamport_clock;
                            send.lamport_ack = i.second;

                        sent(&send,i.first,OK_A,false);
                        }
                        not_give_a.clear();
                    }
                }
            }
            break;
        default:
            break;
        }

    }

}

void president_loop(){
    int finished_quests = 0;
    message send;
    message receive;
    MPI_Status status;
    request created_requests[3];
    request request_template;

    

    while(true){
        MPI_Comm_size( MPI_COMM_WORLD, &n);
        z = 3; //zlecenia utworzone
        a = 2;  //agrafki
        t = 5;  //trucizny
        lamport_clock++;
        //twórz zlecenia
        request_template.id = 0;
        request_template.hamsters = 1;
        created_requests[0] = request_template;

        request_template.id = 1;
        request_template.hamsters = 2;
        created_requests[1] = request_template;

        request_template.id = 2;
        request_template.hamsters = 3;
        created_requests[2] = request_template;


        //
        printf("CLOCK: %d PROCES:0 - Utworzyłem zlecenia\n",lamport_clock);
        lamport_clock++;
        
        finished_quests = 0;
        copy(begin(created_requests),end(created_requests),begin(send.requests));
        //send.requests = created_requests;
        send.a_gr = a;
        send.lamport_clock = lamport_clock;
        send.num_rq = z;
        send.sender_id = tid;
        send.poison = t;
        send.type = KILL_REQUEST;

        sent(&send,0,KILL_REQUEST,true);

        while(finished_quests < z){
            MPI_Recv(&receive,sizeof(message),MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            lamport_clock = max(lamport_clock,receive.lamport_clock)+1;
            switch (receive.type)
            {
            case REQUEST_FINISHED:
                finished_quests++;
                //printf("lol\n");
                break;
            
            default:
                break;
            }
            
            
        }
        printf("CLOCK: %d PROCES:0 - Dostałem informację o zakończeniu wszystkich zleceń\n",lamport_clock);
        

    }

}

int main(int argc, char **argv){
    //printf("Hello world\n");
     int provided; //provided level of security MPI_INIT_THREAD;
    srand(time(NULL));
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    check_thread_support(provided);
    MPI_Status status;

    printf("Checking!\n");
    MPI_Comm_size( MPI_COMM_WORLD, &n); // liczba procesów = wszystkie skrzaty;
    //printf("%d",n);
    MPI_Comm_rank( MPI_COMM_WORLD, &tid); //mój id procesu

    //printf("Jestem skrzatem nr %d z %d\n",tid+1,n);

    if(tid == 0)president_loop();
    else brownie_loop();


}