#define _XOPEN_SOURCE 500
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <ucontext.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/time.h>

static volatile sig_atomic_t back_from_signal;
sigset_t signal_set;
int current_process;

struct PCB {
        ucontext_t uc;
        char *stack;
} P[10];



void sig_handler(int s, siginfo_t *si, void* uctx) {
  if (s!=SIGALRM) {
    write(0,"SIGNAL PAS BON\n",15);
    exit(EXIT_FAILURE);
  }
  back_from_signal=1;
  swapcontext(&P[current_process].uc,&P[0].uc); 
}


int principal(int);
int main()
{       struct itimerval itv;
        void (*principal)(void);
        int i=0;
        void *handle;
        char * error;
        struct sigaction sa;
        stack_t sigstack;
        void *adrsigstack;


        if (sigemptyset(&signal_set)) {
                fprintf(stderr, "sigemptyset error\n");
                exit(EXIT_FAILURE);
        }

        if (sigaddset(&signal_set,SIGALRM)) {
                fprintf(stderr, "sigaddset error\n");
                exit(EXIT_FAILURE);
        }

        if (sigprocmask(SIG_BLOCK,&signal_set,NULL)) {
                fprintf(stderr, "sigprocmask error\n");
                exit(EXIT_FAILURE);
        }

        if (sigemptyset(&sa.sa_mask)) {
                fprintf(stderr, "sigemptyset error\n");
                exit(EXIT_FAILURE);
        }

        adrsigstack =malloc(SIGSTKSZ);

        sa.sa_sigaction=sig_handler;
        sa.sa_flags=SA_SIGINFO|SA_ONSTACK;
        if (sigaction(SIGALRM,&sa,NULL)) {
                fprintf(stderr, "sigaction error\n");
                exit(EXIT_FAILURE);
        }

        handle = dlopen("./helloworld.so", RTLD_NOW);
        if (!handle) {

                fprintf(stderr, "%s\n", dlerror());
                exit(EXIT_FAILURE);
        }


        principal = dlsym(handle, "main");
        if ((error = dlerror()) != NULL)  {
                fprintf(stderr, "%s\n", error);
                exit(EXIT_FAILURE);
        }

        P[1].stack=(char *)malloc(1024*1024);

	if (getcontext(&P[1].uc)) {
	  perror("getcontext");
	  exit(EXIT_FAILURE);
	}
	
        P[1].uc.uc_stack.ss_sp=P[1].stack;
        P[1].uc.uc_stack.ss_size=1024*1024;
        if (sigemptyset(&P[1].uc.uc_sigmask)) {
                fprintf(stderr, "sigemptyset error\n");
                exit(EXIT_FAILURE);
        }
        P[1].uc.uc_link=&P[0].uc;
        makecontext(&P[1].uc,principal,0);
	current_process=1;

	while (1) {
	  sigstack.ss_sp=adrsigstack;
	  sigstack.ss_size=SIGSTKSZ;
	  sigstack.ss_flags=0;
	  if (sigaltstack(&sigstack,NULL)==-1) {
		 perror("sigaltstack");
		 fprintf(stderr,"Error sigaltstack\n");
		 exit(EXIT_FAILURE);
		 }
	  
	  itv.it_value.tv_sec=1; itv.it_value.tv_usec=100;
	  itv.it_interval.tv_sec=0; itv.it_interval.tv_usec=0;
	  if (setitimer(ITIMER_REAL,&itv,NULL)) {
	    printf("Error setitimer\n");
	    exit(2);
	  }
	  
	  printf("valeur : %d\n",i++);
	      
	  back_from_signal=0;
	  P[1].uc.uc_link=&P[0].uc;
	  if (swapcontext(&P[0].uc,&P[1].uc)==-1) {
	    printf("Error swapcontext\n");
	    exit(2);
	  }
	  if (!back_from_signal) {
	    printf("Le programme a fini\n");
	    exit(EXIT_SUCCESS);
	  }
	  else printf("Interruption timer\n");
        }
	
	

        dlclose(handle);
        exit(EXIT_SUCCESS);
}
