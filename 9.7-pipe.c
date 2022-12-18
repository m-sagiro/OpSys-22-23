/**
 * Author:    Mücahit
 * Created:   12.12.2022
 **/
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

// Variable for the infinite loop - set to 0 when CTRL+C pressed
static volatile sig_atomic_t keep_running = 1;

// Signal handler to detect if CTRL+C is pressed and to exit programm. Also to free memory etc.
static void sig_handler(int _) {
    (void)_;
    // setting variable to 0 to end infinite loops
    keep_running = 0;
    // sending all processes user-defined signals to continue and end
    kill(0, SIGUSR1);
    kill(0, SIGUSR2);
}

// Empty handler for user-defined signal. Lets programm continuing
void handler(int sig) {
}

// Main function
int main(void) {
	// Setting signal to detect CTRL+C by signal SIGINT
	signal(SIGINT, sig_handler);
	// Setting signal to detect user-defined signal SIGUSR1
	signal(SIGUSR1, handler);
	// Setting signal to detect user-defined signal SIGUSR2
	signal(SIGUSR2, handler);
	
	// 3 pipes. One for conv --> log and conv --> stat and for stat --> report messaging
	int pipe1[2];
	int pipe2[2];
	int pipe3[2];
	
	// Variables for child PID's
    int convID, logID, statID, reportID;
    
    // Setting up the pipes by using pipe() function. Also catching errors
    int p1, p2, p3;
    p1 = pipe(pipe1);
    p2 = pipe(pipe2);
    p3 = pipe(pipe3);
  	if (p1 < 0 && p2 < 0 && p3 < 0) {
    	printf("Error setting up pipe.\n");
    	exit(1);
  	} else {
    	printf("Pipes are set\n");
  	}	
  	
    // First fork
    convID = fork();
  	// #1 Child --> Conv
    if (convID == 0) {
    	// Waiting first to fork all processes. fflush() needed for sleep
    	fflush(stdout);
    	sleep(4);
        printf("Child conv --> pid = %d and parent pid = %d\n", getpid(), getppid());
        // Closing pipe ends for reading. Conv is only writing information.
        close(pipe1[0]);
		close(pipe2[0]);
		close(pipe3[0]);
		// Inifinite loop
        while (keep_running) {
        	// Waiting to prevent spam. fflush() needed for sleep
        	fflush(stdout);
    		sleep(5);
        	
        	// Here comes all the things done in Conv. More precise its writing A/D values to both pipes
        	char message[] = "Test pipe1";
        	write(pipe1[1], &message, sizeof(message));
        	
        	char message2[] = "Test pipe2";
        	write(pipe2[1], &message2, sizeof(message2));
        	
    		// Here we send the user-defined signals to the process log and stat
    		// to invoke them (Synchronization takes place).
    		printf("Conv has finished and gave signal!\n");
    		kill(logID, SIGUSR1);
    		kill(statID, SIGUSR1);
    		
    		// Waiting to prevent spam. fflush() needed for sleep
    		fflush(stdout);
    		sleep(5);
		}
    }
    else {
    	// Second fork
    	logID = fork();
    	if (logID == 0) {
    		// Waiting first to fork all processes. fflush() needed for sleep
    		fflush(stdout);
    		sleep(3);
    		printf("Child log --> pid = %d and parent pid = %d\n", getpid(), getppid());
    		// Closing pipe ends for not necessary ends of pipes. Log is only reading from pipe1[0].
			close(pipe1[1]);
			close(pipe2[1]);
			close(pipe3[1]);
			close(pipe2[0]);
			close(pipe3[0]);
			// Inifinite loop
    		while (keep_running) {
    			// Creating the mask for which the process should wait/listen to
    			sigset_t mask;
      			sigemptyset(&mask);
      			sigaddset(&mask, SIGUSR1);

      			// This process (child) is waiting for the signal from conv (USRSIG1)
      			printf("Log is waiting for signal...\n");
      			int sig;
				sigwait(&mask, &sig);
      			//Here the process continues when it gets the signal
      			
      			// If we get the correct signal (USRSIG1 or 10) the logic of log is executed
      			// Here comes all the things done in log. More precise its reading and writing A/D values into a local file
	  			if (sig == 10) {
					char puffer [80];
					int len = read(pipe1[0], puffer, sizeof(puffer));
					if (len < 0) {
						perror("read error");
					} else {
						printf("Log recived: %s\n", puffer);
					}
				
				}

			}
    	}
    	else {
    		// Third fork
    		statID = fork();
    		if (statID == 0) {
    			// Waiting first to fork all processes. fflush() needed for sleep
    			fflush(stdout);
    			sleep(2);
    			printf("Child stat --> pid = %d and parent pid = %d\n", getpid(), getppid());
    			// Closing pipe ends for not necessary ends of pipes. Stat is only reading from pipe2[0] and writing to pipe3[1].
				close(pipe1[0]);
				close(pipe1[1]);
				close(pipe2[1]);
				close(pipe3[0]);
				// Inifinite loop
    			while (keep_running) {
    				// Creating the mask for which the process should wait/listen to
					sigset_t mask;
		  			sigemptyset(&mask);
		  			sigaddset(&mask, SIGUSR1);
					
					// This process (child) is waiting for the signal from conv (USRSIG1) (same as log!)
		  			printf("Stat is waiting for signal...\n");
		  			int sig;
		  			int res = sigwait(&mask, &sig);
		  			//Here the process continues when it gets the signal
		  			
		  			// If we get the correct signal (USRSIG1 or 10) the logic of stat is executed
      				// Here comes all the things done in stat. More precise its reading and calculating A/D values and writes it into the third pipe pipe3[1]
		  			if (sig == 10) {
						char puffer [80];
						// Fehler abfangen
						int len = read(pipe2[0], puffer, sizeof(puffer));
						if (len < 0) {
							perror("read error");
						} else {
							printf("Stat recived: %s\n", puffer);
						}
						
						char message[] = "Test pipe3";
        				write(pipe3[1], &message, sizeof(message));
        				
        				// Here we send the user-defined signals to the process report
						// to invoke it (Synchronization takes place).
						printf("Stat has finished and gave signal!\n");
						kill(reportID, SIGUSR2);
					
					}
				}
    		}
    		else {
    			// Fourth fork
    			reportID = fork();
    			if (reportID == 0){
					// Waiting first to fork all processes. fflush() needed for sleep
					fflush(stdout);
					sleep(2);
    				printf("Child report --> pid = %d and parent pid = %d\n", getpid(), getppid());
    				// Closing pipe ends for not necessary ends of pipes. Stat is only reading from pipe2[0] and writing to pipe3[1].
					close(pipe1[0]);
					close(pipe1[1]);
					close(pipe2[1]);
					close(pipe2[0]);
					close(pipe3[1]);
					// Inifinite loop
    				while (keep_running) {
    					// Creating the mask for which the process should wait/listen to
						sigset_t mask;
			  			sigemptyset(&mask);
			  			sigaddset(&mask, SIGUSR2);
						
						// This process (child) is waiting for the signal from stat (USRSIG2)
			  			printf("Report is waiting for signal...\n");
			  			int sig;
			  			int res = sigwait(&mask, &sig);
			  			//Here the process continues when it gets the signal
			  			
			  			// If we get the correct signal (USRSIG2 or 12) the logic of report is executed
		  				// Here comes all the things done in report. More precise its logging the results from stat. Its reading from the pipe3[0]
			  			if (sig == 12) {
							char puffer [80];
							// Fehler abfangen
							int len = read(pipe3[0], puffer, sizeof(puffer));
							if (len < 0) {
								perror("read error");
							} else {
								printf("Report recived: %s\n", puffer);
							}
						
						}
    					
    					
					}
    			} else{
    				// parent process
					fflush(stdout);
					sleep(5);
					printf("parent --> pid = %d\n", getpid());

					while (keep_running) {
						// nothing to do here
					}
    			}
    		}
    	}
    }

	// Ende main() Funktion
    printf("Programm beendet. PID = %d\n", getpid());
    fflush(stdout);
    sleep(7);
    return EXIT_SUCCESS;
}