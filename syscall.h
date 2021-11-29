#include <signal.h>
#include <unistd.h>

struct SCC { // System Call Context
        int number;
        union {
                char * s;
                int i;

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
#endif
