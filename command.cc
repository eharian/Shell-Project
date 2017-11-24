/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <dirent.h>
#include <stdio.h>

#include "command.hh"
int Command::_src = 0;
int Command::_tmpstdin = -1;
int SimpleCommand::lastPID = 0;
int SimpleCommand::returnCode = 0;
Command::Command()
{
	// Create available space for one simple command
	_numOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc( _numOfSimpleCommands * sizeof( SimpleCommand * ) );

	_numOfSimpleCommands = 0;
	_outFile = 0;
	_inFile = 0;
	_errFile = 0;
	_background = 0;
	_append = 0;
	_errOnly = 0;
}

void Command::insertSimpleCommand( SimpleCommand * simpleCommand ) {
	if ( _numOfAvailableSimpleCommands == _numOfSimpleCommands ) {
		_numOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
			 _numOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}
	
	_simpleCommands[ _numOfSimpleCommands ] = simpleCommand;
	_numOfSimpleCommands++;
}

void Command:: clear() {
	for ( int i = 0; i < _numOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numOfArguments; j ++ ) {
			free ( _simpleCommands[ i ]->_arguments[ j ] );
		}
		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}

	if ( _outFile ) {
		free( _outFile );
	}

	if ( _inFile ) {
		free( _inFile );
	}

	if ( _errFile ) {
		free( _errFile );
	}

	_numOfSimpleCommands = 0;
	_outFile = 0;
	_inFile = 0;
	_errFile = 0;
	_background = 0;
	_append = 0;
	_errOnly = 0;
}

void Command::print() {
	
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");
	
	for ( int i = 0; i < _numOfSimpleCommands; i++ ) {
		printf("  %-3d ", i );
		for ( int j = 0; j < _simpleCommands[i]->_numOfArguments; j++ ) {
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[ j ] );
		}
	}

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background\n" );
	printf( "  ------------ ------------ ------------ ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
		_inFile?_inFile:"default", _errFile?_errFile:"default",
		_background?"YES":"NO");
	printf( "\n\n" );
	
}
void Command::runShellrc() {
	DIR* d = opendir(".");
	struct dirent *dir;
	if (d) {
  		while ((dir = readdir(d)) != NULL) {	
			if (!strcmp(dir->d_name, ".shellrc")) {
				Command::_src = 1;
				Command::_tmpstdin = dup(0);
				int file = open(".shellrc", O_RDONLY);
				dup2(file, 0);
				close(file);
				break;
			}
  	  	}
   	 closedir(d);
 	 }
}
int Command::checkBuiltIn(int i) {
	//built in functions
	int err;
	if (!strcmp(_simpleCommands[i]->_arguments[0], "quit")) {
		exit(0);
	}
	if (!strcmp(_simpleCommands[i]->_arguments[0], "exit")) {
		printf("Good bye!!\n");
		exit(0);
	}
	else if (!strcmp(_simpleCommands[i]->_arguments[0], "setenv")) {
		err = setenv(_simpleCommands[i]->_arguments[1], _simpleCommands[i]->_arguments[2], 1);	
		if (err == -1) 
			perror("setenv");
		return 1; 
	}
	else if (!strcmp(_simpleCommands[i]->_arguments[0], "unsetenv")) {
		err = unsetenv(_simpleCommands[i]->_arguments[1]);
		if (err == -1)
			perror("unsetenv");
		return 1;
	}
	else if (!strcmp(_simpleCommands[i]->_arguments[0], "source")) {
		if (_simpleCommands[i]->_arguments[1] == NULL) {
			perror("NULL");
		}
		Command::_src = 1;
		Command::_tmpstdin = dup(0);
		int file = open(_simpleCommands[i]->_arguments[1], O_RDONLY);
		dup2(file, 0);
		close(file);
		return 1;
	}
	else if (!strcmp(_simpleCommands[i]->_arguments[0], "cd")) {
		if(_simpleCommands[i]->_arguments[1] != NULL) {
			err = chdir(_simpleCommands[i]->_arguments[1]);	
		}
		else {
			err = chdir(getenv("HOME"));
		}
		if (err == -1)
			perror("chdir");
		return 1;
	}
	return 0;
}

void Command::execute() {
	// Don't do anything if there are no simple commands
	if ( _numOfSimpleCommands == 0 ) {
		prompt();
		return;
	}

	//Check for certain commands
	if (checkBuiltIn(0)) {
		clear();
		prompt();
		return;
	}
	// Print contents of Command data structure
//	print();


	// Add execution here
	// For every simple command fork a new process
	// Setup i/o redirection
	// and call exec
	int fdin, fdout, fderr, ret;
	int tmpin = dup(0);
	int tmpout = dup(1);
	int tmperr = dup(2);

	//set initial input
	if (_inFile) {
		fdin = open(_inFile, O_RDONLY);
	} 
	else {
		//use default input
		fdin = dup(tmpin);
	}
	
	for (int i = 0; i < _numOfSimpleCommands; i++) {
		//redirect input
		dup2(fdin, 0);
		close(fdin);
		//setup output
		if (i == _numOfSimpleCommands - 1) { //last command
			if (_outFile) {
				if (_append) {
					fdout = open(_outFile, O_CREAT | O_RDWR | O_APPEND, 0664);
				} 
				else {
					fdout = open(_outFile, O_CREAT | O_RDWR | O_TRUNC, 0664);
				}
			} 
			else {
				fdout = dup(tmpout);
			}

			if (_errFile && !_outFile) { //create errfile only
				fderr = open(_errFile, O_CREAT | O_RDWR | O_TRUNC, 0664);
			}
			else {
				fderr = dup(tmperr);
			}
		}
		else { //not last command so pipe
			int fdpipe[2];
			pipe(fdpipe);
			fdin = fdpipe[0];
			fdout = fdpipe[1];
		}
		//redirect output
		dup2(fdout, 1);
		//redirect error file if needed
		if(_errFile && !_errOnly) { //>& or >>&
			dup2(fdout, 2);
		}
		else { // 2>
			dup2(fderr, 2);
			close(fderr);	
		}
		close(fdout);
		
		//create child process
		ret = fork();
		if (ret == 0) { //in the child process
			if (!strcmp(_simpleCommands[i]->_arguments[0], "printenv")) {
				char** p = environ;
				while(*p != NULL) {
					printf("%s\n", *p);
					p++;
				}
				exit(0);
			}
			execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);
			perror("execvp");
			exit(1);
		}
		else if (ret < 0) {
			perror("fork");
			return;
	 	}
	}
	//restore in/out defaults
	dup2(tmpin, 0);
	dup2(tmpout, 1);
	dup2(tmperr, 2);
	close(tmpin);
	close(tmpout);
	close(tmperr);

	if(!_background) { //wait for last command
		int res;
		waitpid(ret, &res, 0);
		if (WIFEXITED(res)) {
			SimpleCommand::returnCode = WEXITSTATUS(res);
		}
	}
	else {
		SimpleCommand::lastPID = ret;
	}

			
	// Clear to prepare for next command
	clear();
	
	// Print new prompt
	prompt();
}

// Shell implementation

void Command::prompt() {
	if (isatty(0)) {
		printf("myshell>");
	}
	fflush(stdout);
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;

