#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <wait.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/syscall.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include "interceptor.h"

int main(){
	
	//Assume that kernl is already isntalled 
	printf("TESTING INTERCEPT 5 \n");
	syscall(MY_CUSTOM_SYSCALL, REQUEST_SYSCALL_INTERCEPT, 5); //Intercept system call no.5 
	printf("TESTING MONITORING getpid for syscall = 5\n");
	
	syscall(MY_CUSTOM_SYSCALL, REQUEST_START_MONITORING, 5, 0); //Monitors all pids
	
	printf("Calling system\n");
	
	syscall(5,1,2,3,4,5,6);

	syscall(MY_CUSTOM_SYSCALL, REQUEST_STOP_MONITORING, 5, getpid());


	
	printf("TESTING CALLING THE syscall being monitored \n ");
	syscall(5,1,2,3,4,5,6);
	printf("IF SCCUSS, we would see a log message with all the parameters \n");
	printf("Test for stop monitoring with monitored == 2 and pid = %d \n ", getpid());
	syscall(MY_CUSTOM_SYSCALL, REQUEST_STOP_MONITORING, 5, getpid() ); 
	syscall(5,1,2,3,4,5,6);
	
	
	printf("We are now removing ourself from the monitored");
	printf("Removing my_pid = %x \n", getpid() );
	syscall(MY_CUSTOM_SYSCALL, REQUEST_STOP_MONITORING ,5, getpid() );
	printf("Now we issue the system call again \n");
	syscall(5,1,2,3,4,5,6);
	printf("No more lines should be added to the dmesg \n");
	syscall(MY_CUSTOM_SYSCALL, REQUEST_SYSCALL_RELEASE, 5); 



	return 0;

}