#include <sys/msg.h>
#include "err.h"

void qget (int* var, int qid, const char* s) {
    *var = msgget(qid, READ_AND_WRITE);
    if (*var == -1)
	syserr(s);
}

void send (int qid, struct message* msg, int arg, const char* s) {
    sprintf((*msg).text, "%d", arg);
    if (msgsnd(qid, msg, sizeof((*msg).text), 0) == -1)
	syserr(s);
}

void receive (int rsid, struct message* msg, int type, int* var, const char* s) {
    if (msgrcv(rsid, msg, sizeof((*msg).text), type, 0) == -1)
	   syserr(s);
    *var = atoi(msg.text);
}