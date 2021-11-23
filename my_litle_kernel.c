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


#define MAX_PROCESS 99
int cp=0;           // current process number running
int nb_process=0;


static struct PCB { // Process Context Block 
        ucontext_t uc; // context of process
        char *stack;   // stack of process
        clock_t clock; // total clock time 
        clock_t lclock; // clock value before process is scheduled
        int quantum; // quantum for the process
        scc_t *scc; // system call context
} P[100];



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
	//        dlclose(handle);
        return p;
}

static void system_call() {
        int r;
        switch (system_call_ctx->number) {
                case 1: r=spawn(system_call_ctx->u.s);
                        system_call_ctx->result=r;
                        break;
                default: system_call_ctx->result=-1;
        }
}


static int choose_next_process() { // RR(1)
        if (!nb_process) return -1;
        int p=cp%nb_process+1; 
        P[p].quantum=1;
        return p;
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
  P[cp].clock+=clock()-P[cp].lclock;  // update process time
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

        spawn("./shell.so");
        scheduler();
	
        exit(EXIT_SUCCESS);
}