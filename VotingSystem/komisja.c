#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "err.h"
#include "collective.h"

//Id kolejek.
int 	      pid, 
	      gsid, cgid, csid;
	      
long	      committee_nr;

void open_queues () {
    qget(&cgid, get, "Error in opening IPC queue 1");
    qget(&gsid, getspec, "Error in opening IPC queue 2");
    qget(&csid, comsend, "Error in opening IPC queue 3");
}

void send_initial_signals () { 
    send(cgid, &msg, COMSIGNAL, "Error in sending COMSIGNAL");
    send(gsid, &msg, committee_nr, "Error in sending committee nr");
}

//Sprawdza, czy nie było już poprzednio zapytania komisji o tym samym numerze.
void check_access () {
    if (msgrcv(csid, &msg, sizeof(msg.text), pid, 0) == -1)
	   syserr("Error in receiving message");

    switch (atoi(msg.text)) {
	case -1:
	    printf("Odmowa dostępu.\n");
	    exit(0);
	    break;
	case 0:
	    break;
    }
}

void send_final_signal () {
    int i;
    for (i = 0; i<3; i++)
	send(gsid, &msg, FINALSIG, "Error in sending FINALSIG");
}

void get_results (long* verses, long* valid_votes) {
    receive(csid, &msg, pid, verses, "Error in receiving verses");
    receive(csid, &msg, pid, valid_votes, "Error in receiving valid_votes");
}

void print_results (long* verses, long* voters, 
		    long* votes, long* valid_votes) {
    printf("Przetworzonych wpisów: %ld\n", *verses);
    printf("Uprawnionych do głosowania: %ld\n", *voters);
    printf("Głosów ważnych: %ld\n", *valid_votes);
    printf("Głosów nieważnych: %ld\n", *votes - *valid_votes);
    printf("Frekwencja w lokalu: %.2f%%\n", ((double)*votes / *voters) * 100);
}

int main (int argc, char *argv[]) {
    committee_nr = atoi(argv[1]);
    pid = getpid();
    msg.type = pid;

    open_queues();
    send_initial_signals();
    check_access();

    //input
    long voters, votes, 
	 list, candidate, n,
	 verses, valid_votes;
	
    scanf("%ld%ld", &voters, &votes);
    
    send(gsid, &msg, voters, "Error in sending voters nr");
    send(gsid, &msg, votes, "Error in sending votes nr");

    while (scanf("%ld%ld%ld", &list, &candidate, &n) != EOF) {
	send(gsid, &msg, list, "Error in sending list nr");
	send(gsid, &msg, candidate, "Error in sending candidate nr");
	send(gsid, &msg, n, "Error in sending nr of candidate votes");
    }

    send_final_signal();
    get_results(&verses, &valid_votes);
    print_results(&verses, &voters, 
		  &votes, &valid_votes);
    return 0;
}