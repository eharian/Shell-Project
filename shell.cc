#include "command.hh"
#include <signal.h>
#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <string.h>
#include <sys/wait.h>
int yyparse(void);

char* SimpleCommand::shellPath = NULL;
extern "C" void sigHandler(int sig) {
	printf("\n");
	Command::_currentCommand.clear();
	Command::_currentCommand.prompt();
}
extern "C" void sigChildHandler(int sig) {
	while (wait3(NULL, WNOHANG, NULL) > 0) {}
}
int main(int argc, char** argv) {
	struct sigaction sa;
	sa.sa_handler = sigHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	if (sigaction(SIGINT, &sa, NULL)) {
		perror("sigaction");
		exit(2);
	}

	struct sigaction sc;
	sc.sa_handler = sigChildHandler;
	sigemptyset(&sc.sa_mask);
	sc.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sc, NULL)) {
		perror("sigchild");
		exit(2);
	}
	Command::_currentCommand.runShellrc();	
	char buf[1024];
	SimpleCommand::shellPath = realpath(argv[0], buf);
	Command::_currentCommand.prompt();
	yyparse();
}
