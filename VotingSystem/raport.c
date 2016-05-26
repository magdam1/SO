#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "err.h"
#include "collective.h"

int 	      pid,
	      gsid, rgid, rsid; //Id kolejek.
long	      list_nr;

void open_queues () {
    qget(&rgid, get, "Error in opening IPC queue 1");
    qget(&gsid, getspec, "Error in opening IPC queue 2");
    qget(&rsid, repsend, "Error in opening IPC queue 3");
}

void send_initial_signals (int argc) {
    send(rgid, &msg, REPSIGNAL, "Error in sending REPSIGNAL");
    switch (argc) {
	case 1:
	    send(gsid, &msg, NOARGSIG, "Error in sending NOARGSIG");
	break;
	case 2:
	    send(gsid, &msg, ARGSIGNAL, "Error in sending ARGSIGNAL");
	    send(gsid, &msg, list_nr, "Error in sending ARGSIGNAL");
	break;
    }
}

void read_results (long* committees_done, long* voters, long* valid_votes, 
		   long* invalid_votes, long* lists, long* candidates, long* committees) {
    receive(rsid, &msg, pid, 
	    committees_done, "Error in receiving done committees");
    receive(rsid, &msg, pid, 
	    voters, "Error in receiving nr of voters");
    receive(rsid, &msg, pid, 
	    valid_votes, "Error in receiving nr of valid votes");
    receive(rsid, &msg, pid, 
	    invalid_votes, "Error in receiving nr of invalid votes");
    receive(rsid, &msg, pid, 
	    lists, "Error in receiving nr of lists");
    receive(rsid, &msg, pid, 
	    candidates, "Error in receiving nr of candidates");
    receive(rsid, &msg, pid, 
	    committees, "Error in receiving committees");
}

void print_results (int argc, long* committees_done, long* voters, long* valid_votes, 
		    long* invalid_votes, long* lists, long* candidates, long* committees) {
    printf("Przetworzonych komisji: %ld / %ld\n", *committees_done, *committees);
    printf("Uprawnionych do głosowania: %ld\n", *voters);
    printf("Głosów ważnych: %ld\n", *valid_votes);
    printf("Głosów nieważnych: %ld\n", *invalid_votes);
    
    if (*voters == 0)
	printf("Frekwencja: 0%%\n");
    else
	printf("Frekwencja: %.2f%%\n", ((double)(*invalid_votes + *valid_votes) / *voters) * 100);
    printf("Wyniki poszczególnych list:\n");
    int i, j;
    long tmp;
    switch (argc) {
	case 1:
	    for (i = 0; i < *lists; i++) {
		for (j = 0; j < *candidates+2; j++) {
		    receive(rsid, &msg, pid, 
			    &tmp, "Error in receiving result");
		    printf("%ld ", tmp);
		}
		printf("\n");
	    }
	break;
	case 2:
	    for (i = 0; i < *candidates+2; i++) {
		receive(rsid, &msg, pid, 
		    &tmp, "Error in receiving result");
		printf("%ld ", tmp);
	    }
	    printf("\n");
	break;
    }
}

int main (int argc, char *argv[]) {
    long lists, candidates,
	 committees, committees_done,
	 voters, 
	 valid_votes, invalid_votes;
	 
    pid = getpid();
    msg.type = pid;
    if (argc == 2)
	list_nr = atoi(argv[1]);
    open_queues();
    send_initial_signals(argc);
 
    read_results(&committees_done, &voters, &valid_votes, 
		 &invalid_votes, &lists, &candidates, &committees);
    print_results(argc, &committees_done, &voters, &valid_votes, 
		  &invalid_votes, &lists, &candidates, &committees);
    return 0;
}