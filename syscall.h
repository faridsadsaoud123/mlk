#include <signal.h>
#include <unistd.h>

struct SCC { // System Call Context
        int number;
        union {
                char * s;
                int i;
                int p;
        } u;
        int result;
}; // process fill a SCC struct and set pointer before syscall

typedef struct SCC scc_t;

#ifdef MY_LITLE_KERNEL
extern scc_t * system_call_ctx; // process fill a SCC struct and set pointer before syscall
#else
extern scc_t * system_call_ctx; // process fill a SCC struct and set pointer before syscall

scc_t scc;

int spawn(char *c) {
        scc.number=1;
        scc.u.s=c;
        system_call_ctx=&scc;
        kill(getpid(),SIGUSR1);
        return scc.result;
}

int mlk_clock() {
        scc.number=2;
        system_call_ctx=&scc;
        kill(getpid(),SIGUSR1);
        return scc.result;
}

int mlk_sleep(int s) {
        scc.number=3;
        scc.u.i=s;
        system_call_ctx=&scc;
        kill(getpid(),SIGUSR1);
        return scc.result;
}

int mlk_getpid() {
        scc.number=4;
        system_call_ctx=&scc;
        kill(getpid(),SIGUSR1);
        return scc.result;
}
int mlkprint(char* s){
        scc.u.s= s;
        scc.number=5;
        system_call_ctx=&scc;
        kill(getpid(),SIGUSR1);
        return scc.result;
}
int mlk_wait(){
        scc.number=6;
        system_call_ctx=&scc;
        kill(getpid(),SIGUSR1);
        return scc.result;
}
int mlk_signal(int p){
        scc.u.i=p;
        scc.number=7;
        system_call_ctx=&scc;
        kill(getpid(),SIGUSR1);
        return scc.result;
}

int mlk_send(char* msg,int l, int p){
        scc.u.s=msg;
        scc.u.p=p;
        scc.number=8;
        system_call_ctx=&scc;
        kill(getpid(),SIGUSR1);
        return scc.result;
}
int mlk_recv(char* buff, int l){
        scc.u.s=buff;
        scc.number=9;
        system_call_ctx=&scc;
        kill(getpid(),SIGUSR1);
        return scc.result;
}



#endif
