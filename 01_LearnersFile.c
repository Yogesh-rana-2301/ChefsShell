
#include "01_main.h"   
//Include Format		Used For			Searches Where?
/*#include "..."		Local/project files	 	Current directory, then system
include <...>			System headers			Only system paths
*/

int main(int ac, char **av){   //ac is for argument counter and av is the argument vector, 
			       //for exmaple in am in right now <> ./main.c ls -l then the ac is 3 and the av is {ls, -l, NULL}
	(void)ac; // otherwise will give me warnign of unused. 
	int status;
	
	//child process
	if (fork ()==0){
		  
	execvp(av[1],av+1);  // execvp (command , pointer to the av of {ls,-l,NULL});
			     //
	}
	wait (&status);
	return EXIT_SUCCESS;  //macro , this means #define EXIT_SUCCESS 0;
}




//i would be implementing the repl (read evaluate print loop )structure now 
//
//
//
//
//
#include "01_main.h"   //Include Format	Used For	Searches Where?
/*#include "..."	Local/project files	
Current directory, then system
#include <...>	System headers	Only system paths
*/

int main(int ac, char **av){   //ac is for argument counter and av is the argument vector, for exmaple in am in right now <> ./main.c ls -l then the ac is 3 and the av is {ls, -l, NULL}
	(void)ac; // otherwise will give me warnign of unused. 
	int status;
	
	//child process
	if (fork ()==0){
		  
	execvp(av[1],av+1);  // execvp (command , pointer to the av of {ls,-l,NULL});
			     //
	}
	wait (&status);
	return EXIT_SUCCESS; //macro , this means #define EXIT_SUCCESS 0;
}



