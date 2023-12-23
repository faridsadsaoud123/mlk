

// Realise par Zouaoui Mohamed Cherif 22313975
//, et SAD SAOUD Farid 22312283
// Groupe1
#define _XOPEN_SOURCE 500
#define MY_LITLE_KERNEL
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include "syscall.h"
#include <sys/wait.h>
// #include <semaphore>
#define MAX_PROCESS 99
int cp=0;           // current process number running
int nb_process=0;


static struct PCB { // Process Context Block 
        ucontext_t uc; // context of process
        char *stack;   // stack of process
        int ppid;      // Parent process
        clock_t clock; // total clock time 
        clock_t lclock; // clock value before process is scheduled
        clock_t qclock; // clock time used during scheduled time
        int quantum; // quantum for the process
        scc_t *scc; // system call context
        struct timeval alarm;
        int waiting;
        void* msg;
        int from,to,len,sendrecv;
} P[100];
static struct sema_t{
        int n;
}sema_t;



static int get_free_process_number() {
        return ++nb_process;
}

void (*principal)();
static  int spawn(char * command) {  // spawn new process, loaded but not launched (schedule job)
        void *handle;
        int p;
        char *error;

        handle = dlopen(command, RTLD_NOW);
        if ((error = dlerror()) != NULL)  {
                fprintf(stderr, "%s\n", error);
                if (!cp) exit(EXIT_FAILURE);
                else return EXIT_FAILURE;
        }

        principal = dlsym(handle, "main");
        if ((error = dlerror()) != NULL)  {
                fprintf(stderr, "%s\n", error);
                exit(EXIT_FAILURE);
        }

        p=get_free_process_number();

        P[p].stack=(char *)malloc(1024*1024);

        if (getcontext(&P[p].uc)) {
          perror("getcontext");
          exit(EXIT_FAILURE);
        }

        P[p].uc.uc_stack.ss_sp=P[p].stack;
        P[p].uc.uc_stack.ss_size=1024*1024;
        if (sigemptyset(&P[p].uc.uc_sigmask)) {
                fprintf(stderr, "sigemptyset error\n");
                exit(EXIT_FAILURE);
        }
        P[p].uc.uc_link=&P[0].uc; // when process finish, go back to scheduler
        makecontext(&P[p].uc,principal,0);
        return p;
}

static int mlk_clock() { // return clock for process
        return P[cp].clock + clock()-P[cp].lclock;
}

static int mlk_sleep(int s) {
        gettimeofday(&P[cp].alarm,NULL);
        P[cp].alarm.tv_sec+=s;
        return 0;
}

static int mlk_getpid() {
  return cp;
}

static int mlkprint(char* s){
        printf("bonjour ---- %s\n",s);
}

// Fonction mlk_wait
int mlk_wait() {
    
        P[cp].waiting=1;
        return 0;
}

// Fonction mlk_signal
int mlk_signal(int p) {

    P[p].waiting=0;
    return 0;
}

int mlk_send(void* mesg,int l, int p){
        if (P[cp].from==-1)
        {
                mlk_wait();
        }
        else{
                P[p].from=-1;
                mlkprint("message envoye\n");

        }
        
}
int mlk_recv(void* buff, int l){
        if(P[cp].from==-1){
                mlk_wait();
        }
        else{
                P[P[cp].from].to=-1;
                mlkprint("Message recu\n");
        }
}
sema_t sema_create(int n) {
        sema_t s;
        s.n=n;
        return s;
}
sema_wait(sema_t s) {
        if(s.n==0){
                mlk_wait();
        }
        else{
                s.n=s.n-1;
        }
}
sema_post(sema_t s) {
        
}
static void system_call() {
        int r;
        switch (system_call_ctx->number) {
                case 1: r=spawn(system_call_ctx->u.s);
                        system_call_ctx->result=r;
                        break;
                case 2: r=mlk_clock();
                        system_call_ctx->result=r;
                        break;
                case 3: r=mlk_sleep(system_call_ctx->u.i);
                        system_call_ctx->result=r;
                        break;
                case 4: r=mlk_getpid();
                        system_call_ctx->result=r;
                        break;
                case 5: r=mlkprint(system_call_ctx->u.s);
                        system_call_ctx->result=r;
                        break;	
                case 6: r=mlk_wait();
                        system_call_ctx->result=r;
                        break;
                case 7: r=mlk_signal();
                        system_call_ctx->result=r;
                        break;
                case 8: r=mlk_send();
                        system_call_ctx->result=r;
                        break;
                case 9: r=mlk_recv();
                        system_call_ctx->result=r;
                        break;
               default: system_call_ctx->result=-1;
        }
}


static int choose_next_process() { // RR(1)
        struct timeval tv;
        if (!nb_process) return -1;
        int p=cp%nb_process;
        while (gettimeofday(&tv,NULL),P[p+1].alarm.tv_sec>tv.tv_sec && P[p+1].waiting) p=(p+1)%nb_process;
        P[p+1].quantum=1;
        return p+1;
}

static void scheduler() {
        struct itimerval itv;
        while (1) {
                cp=choose_next_process();

                if (cp==-1) {
                        printf("Plus de process\n");
                        exit(EXIT_SUCCESS);
                }

                P[cp].lclock=clock(); // 

                if (P[cp].quantum!=-1) { // set alarm for next preemtion
                        itv.it_value.tv_sec=P[cp].quantum; itv.it_value.tv_usec=0;
                        itv.it_interval.tv_sec=0; itv.it_interval.tv_usec=0;
        
                        if (setitimer(ITIMER_REAL,&itv,NULL)) {
                                printf("Error setitimer\n");
                                exit(2);
                        }
                }       
                swapcontext(&P[0].uc,&P[cp].uc);
        }
}


void sig_handler(int s, siginfo_t *si, void* uctx) {
  if (s!=SIGALRM && s!=SIGUSR1) {
    write(0,"SIGNAL PAS BON\n",15);
    exit(EXIT_FAILURE);
  }
  if (!cp) {
    write(0,"SIGNAL IN SCHEDULER !!!!\n",15);
    exit(EXIT_FAILURE);
  }
  P[cp].clock+=(P[cp].qclock=clock()-P[cp].lclock);  // update process time
  if (s==SIGUSR1) {
        P[cp].scc=system_call_ctx;
        system_call();
   }
  swapcontext(&P[cp].uc,&P[0].uc); // back to scheduler
}

int main()
{       sigset_t signal_set;
        struct sigaction sa;

        // Kernel may never be interrupted : set mask
        if (sigemptyset(&signal_set)) {
                fprintf(stderr, "sigemptyset error\n");
                exit(EXIT_FAILURE);
        }

        if (sigaddset(&signal_set,SIGALRM)) {
                fprintf(stderr, "sigaddset error\n");
                exit(EXIT_FAILURE);
        }

        if (sigaddset(&signal_set,SIGUSR1)) {
                fprintf(stderr, "sigaddset error\n");
                exit(EXIT_FAILURE);
        }

        if (sigprocmask(SIG_BLOCK,&signal_set,NULL)) {
                fprintf(stderr, "sigprocmask error\n");
                exit(EXIT_FAILURE);
        }

 
        sa.sa_sigaction=sig_handler;
        sa.sa_flags=SA_SIGINFO|SA_ONSTACK;
        if (sigaction(SIGALRM,&sa,NULL)) {
                fprintf(stderr, "sigaction error\n");
                exit(EXIT_FAILURE);
        }

        if (sigaction(SIGUSR1,&sa,NULL)) {
                fprintf(stderr, "sigaction error\n");
                exit(EXIT_FAILURE);
        }
        printf("Kernel Information : there is %lu clock per second\n",CLOCKS_PER_SEC);
        spawn("./init.so");
        scheduler();
	
        exit(EXIT_SUCCESS);
}
