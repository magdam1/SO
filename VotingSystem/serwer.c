#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <semaphore.h>
#include <pthread.h>
#include "err.h"
#include "collective.h"

int 	     	gid, gsid, csid, rsid;

//ATRYBUTY PTHREAD - ZARZĄDZANIE WĄTKAMI!
//SEMAFOR NA LICZBĘ WĄTKÓW!!!
//CTRL + C CO Z KLIENTAMI <- czy pamięć jest zwalniana przy DETACHED?

unsigned int  	 who[10000], //Tablica trzymająca informacje o numerach komisji, które przysłały już zapytanie,
		 lists, candidates,
		 committees;

pthread_t 	 newthread;
pthread_attr_t 	 attr;
sem_t 		 sem; 
pthread_mutex_t	 who_mutex;
pthread_rwlock_t data_rwlock;
pthread_rwlockattr_t 
		 attr_rw;

//Struktura trzymająca dane wspólne dla wszystkich wątków.
struct D {
long 		 all_votes, valid_votes,
		 voters, committees_done,
		 l_votes[100], 
		 c_votes[100][100];
} data;

//Inicjalizacja danych w strukturze.
void init (struct D * d) {
    (*d).all_votes = 0;
    (*d).valid_votes = 0;
    (*d).voters = 0;
    (*d).committees_done = 0;

    int i, j;
    for (i = 0; i < 100; i++)
	(*d).l_votes[i] = 0;

    for (i = 0; i < 100; i++)
	for (j = 0; j < 100; j++)
	    (*d).c_votes[i][j] = 0;
}

//Dodane obliczone lokalnie przez wątek do danych globalnych.
void add (struct D d2) {
    data.all_votes += d2.all_votes;
    data.valid_votes += d2.valid_votes;
    data.voters += d2.voters;
    data.committees_done++;
    
    int i, j;
    for (i = 1; i < 100; i++)
	data.l_votes[i] += d2.l_votes[i];
    
    for (i = 1; i < 100; i++)
	for (j = 1; j < 100; j++)
	    data.c_votes[i][j] += d2.c_votes[i][j];
}

//Kopiowanie danych globalnych do lokalnej struktury.
void copy (struct D * d) {
    (*d).all_votes = data.all_votes;
    (*d).valid_votes = data.valid_votes;
    (*d).voters = data.voters;
    (*d).committees_done = data.committees_done;

    int i, j;
    for (i = 0; i < 100; i++)
	(*d).l_votes[i] = data.l_votes[i];

    for (i = 0; i < 100; i++)
	for (j = 0; j < 100; j++)
	    (*d).c_votes[i][j] = data.c_votes[i][j];
}

void initialize_data () {
    init(&data);
    int i;
    for (i = 0; i< 10000; i++)
	who[i] = 0;
    if (sem_init(&sem, 1, 100) == -1)
	syserr("Error in initializing semaphore");
    if (pthread_mutex_init(&who_mutex, NULL))
	syserr("Error in initializing who_mutex");
    if (pthread_rwlockattr_init(&attr_rw))
	syserr("Error in initializing rwlock attribute");
    if (pthread_rwlockattr_setkind_np(&attr_rw, 
				      PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP))
	syserr("Error in setkind");
    if (pthread_rwlock_init(&data_rwlock, &attr_rw))
	syserr("Error in initializing data_rwlock");
    if (pthread_attr_init(&attr))
	    syserr("Error in initializating attr");
    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED))
	    syserr("Error in setting detachedstate");
}

void create_queues () {
    gid = msgget(get, IPC_CREAT|IPC_EXCL|READ_AND_WRITE);
    if (gid == -1)
	syserr("Error in creating IPC queue 1");

    gsid = msgget(getspec, IPC_CREAT|IPC_EXCL|READ_AND_WRITE);
    if (rsid == -1)
	syserr("Error in creating IPC queue 2");
    
    csid = msgget(comsend, IPC_CREAT|IPC_EXCL|READ_AND_WRITE);
    if (csid == -1)
	syserr("Error in creating IPC queue 3");
    
    rsid = msgget(repsend, IPC_CREAT|IPC_EXCL|READ_AND_WRITE);
    if (rsid == -1)
	syserr("Error in creating IPC queue 4");
}

//Funkcja dla wątków obsługujących komisję.
void * committee (void * type) {
    long committee_nr, sig,
	 verses = -1,
	 tmp_list = 0, tmp_candidate = 0, tmp_votes = 0;
	
    long * int_type = (long *)type;
    struct message com;
    com.type = (*int_type);

// *** Sprawdzanie, czy nie wystąpił poprzednio dany nr komisji
    receive(gsid, &com, (*int_type), &committee_nr, "Error in receiving committee_nr");
    
    if(pthread_mutex_lock(&who_mutex))
	syserr("Error in locking who_mutex");
    sig = (who[committee_nr] == 0 ? 0 : -1);
    who[committee_nr] = 1;
    if(pthread_mutex_unlock(&who_mutex))
	syserr("Error in unlocking who_mutex");
 
    send(csid, &com, sig, "Error in sending feedback");
    if (sig == -1)
	pthread_exit(0);
// ***
    
    struct D local_data;
    init(&local_data);
    receive(gsid, &com, (*int_type), &(local_data.voters), "Error in receiving committee_nr");
    receive(gsid, &com, (*int_type), &(local_data.all_votes), "Error in receiving committee_nr");
    
    while ((tmp_list != -1)) {
	verses++;
	local_data.valid_votes += tmp_votes;
	local_data.l_votes[tmp_list] += tmp_votes;
	local_data.c_votes[tmp_list][tmp_candidate] += tmp_votes;
	receive(gsid, &com, (*int_type), &tmp_list, "Error in receiving committee_nr");
	receive(gsid, &com, (*int_type), &tmp_candidate, "Error in receiving committee_nr");
	receive(gsid, &com, (*int_type), &tmp_votes, "Error in receiving committee_nr");
    }
    
// *** Uaktualnianie wspólnych danych
    if(pthread_rwlock_wrlock(&data_rwlock))
	syserr("Error in locking data_rwlock");
    add(local_data);
    if(pthread_rwlock_unlock(&data_rwlock))
	syserr("Error in unlocking data_rwlock");
// ***
    
    send(csid, &com, verses, "Error in sending nr of verses");
    send(csid, &com, local_data.valid_votes, "Error in sending nr of all votes");
    
    if (sem_post(&sem))
	syserr("Error in sem_post");
    
    return NULL;
}

//Funkcja dla wątków obsługujących raport.
void * report (void * type) {
    long mode, list_nr, i, j;
    long * int_type = (long *)type;
    struct message rep;
    struct D local_data;
    rep.type = *int_type;
    
    receive(gsid, &rep, *int_type, &mode, "Error in receiving mode");
    if (mode == 3)
	receive(gsid, &rep, *int_type, &list_nr, "Error in receiving list nr");
    
// *** Kopiowanie wspólnych danych do lokalnej struktury
    if(pthread_rwlock_rdlock(&data_rwlock))
	syserr("Error in locking data_rwlock");
    copy(&local_data);
    if(pthread_rwlock_unlock(&data_rwlock))
	syserr("Error in unlocking data_rwlock");
// ***
    
    send(rsid, &rep, local_data.committees_done, "Error in sending nr of done committees");
    send(rsid, &rep, local_data.voters, "Error in sending nr of voters");
    send(rsid, &rep, local_data.valid_votes, "Error in sending nr of valid votes");
    send(rsid, &rep, local_data.all_votes - data.valid_votes, "Error in sending nr of invalid votes");
    send(rsid, &rep, lists, "Error in sending nr of lists");
    send(rsid, &rep, candidates, "Error in sending nr of candidates");
    send(rsid, &rep, committees, "Error in sending nr of committees");
    
    switch (mode) {
	case 2:
	    for (i = 1; i <= lists; i++) {
		send(rsid, &rep, i, "Error in sending list nr");
		send(rsid, &rep, local_data.l_votes[i], "Error in sending nr of votes for list");
		for (j = 1; j <= candidates; j++)
		    send(rsid, &rep, local_data.c_votes[i][j], "Error in sending nr of votes for candidate");
	    }
	break;
	case 3:
	    send(rsid, &rep, list_nr, "Error in sending list nr");
	    send(rsid, &rep, local_data.l_votes[list_nr], "Error in sending nr of votes for list");
	    for (i = 1; i <= candidates; i++)
		send(rsid, &rep, local_data.c_votes[list_nr][i], "Error in sending nr of votes for candidate");
	break;
    }
    
    if (sem_post(&sem))
	syserr("Error in sem_post");
    
    return NULL;
}

void close_queues () {
    if (msgctl(gid, IPC_RMID, NULL) == -1)
	syserr("Error in removing queue 1");
    if (msgctl(gsid, IPC_RMID, NULL) == -1)
	syserr("Error in removing queue 2");
    if (msgctl(csid, IPC_RMID, NULL) == -1)
	syserr("Error in removing queue 3");
    if (msgctl(rsid, IPC_RMID, NULL) == -1)
	syserr("Error in removing queue 4");
    if (pthread_mutex_destroy(&who_mutex))
	syserr("Error in destroying who_mutex");
    if (pthread_rwlock_destroy(&data_rwlock))
	syserr("Error in destroying data_rwlock");
    if (pthread_rwlockattr_destroy(&attr_rw))
	syserr("Error in destroying rwlock attr");
    if (pthread_attr_destroy(&attr))
	syserr("Error in destroying thread attr");
    if (sem_destroy(&sem))
	syserr("Error in destroying semaphore");
    printf("\n");
    exit(0);
}

int main (int argc, char *argv[]) {
    if (signal(SIGINT, close_queues) == SIG_ERR)
	syserr("Error in signal");

    lists = atoi(argv[1]); 
    candidates = atoi(argv[2]);
    committees = atoi(argv[3]);
	
    //Typ klienta - raport lub komisja.
    long client;

    initialize_data();
    create_queues(); 

    while (1) {
	receive(gid, &msg, 0, &client, "Error in receiving type");
	
	if (sem_wait(&sem))
	    syserr("Error in sem_wait");
	
	switch (client) {
	    case 0:
		if (pthread_create(&newthread, &attr, &committee, &msg.type))
		    syserr("Error in creating new committee thread");
	    break;
	    case 1:
		if (pthread_create(&newthread, &attr, &report, &msg.type))
		    syserr("Error in creating new report thread");
	    break;
	}
    }
    return 0;
}