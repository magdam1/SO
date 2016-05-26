#ifndef COLLECTIVE
#define COLLECTIVE

//Klucze kolejek.
const key_t get = 1234L,
	    getspec = 1470L,
	    comsend = 4567L, 
	    repsend = 7890L;

const long   READ_AND_WRITE = 0666,
	     COMSIGNAL = 0,
	     REPSIGNAL =  1,
	     NOARGSIG = 2,
	     ARGSIGNAL = 3,
	     FINALSIG = -1;

struct message {
    long    type;
    char    text[20];
} msg;

//Pomocnicza funkcja do uzyskania dostępu do istniejącej kolejki.
void qget (int* var, int qid, const char* s) {
    *var = msgget(qid, READ_AND_WRITE);
    if (*var == -1)
	syserr(s);
}

//Pomocnicza funkcja do wysyłania na kolejkę.
void send (int qid, struct message* msg, 
	   long arg, const char* s) {
    sprintf((*msg).text, "%ld", arg);
    if (msgsnd(qid, msg, sizeof((*msg).text), 0) == -1)
	syserr(s);
}

//Pomocnicza funkcja do odbierania z kolejki.
void receive (int qid, struct message* msg, 
	      int type, long* var, const char* s) {
    int n;
    n = msgrcv(qid, msg, sizeof((*msg).text), type, 0);
    if (n == -1)
	   syserr(s);
    (*msg).text[n] = '\0';
    *var = atoi((*msg).text);
}
#endif