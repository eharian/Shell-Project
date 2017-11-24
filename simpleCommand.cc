#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <stdlib.h>
#include <sys/types.h>
#include <pwd.h>
#include <stdio.h>
#include "simpleCommand.hh"
SimpleCommand::SimpleCommand() {
	// Create available space for 5 arguments
	_numOfAvailableArguments = 5;
	_numOfArguments = 0;
	_arguments = (char **) malloc( _numOfAvailableArguments * sizeof( char * ) );
}
extern "C" char* lastCommand();
char * SimpleCommand::expandEnvironment( char * argument ) {
	char* arg = argument;
	char* dollar = strchr(arg, '$');
	if (dollar == NULL) {
		return NULL;
	}
	char* finalArg = (char*)malloc(1024);
	int i = 0; int j = 0; int k = 0;
	while (arg[i] != '\0') {
		if (arg[i] == '$') {
			if (arg[i+1] == '{') {
				char* env = (char*)malloc(strlen(arg));
				i += 2;
				while (arg[i] != '}') {
					env[j++] = arg[i++];
				}
				env[j] = '\0';
				j = 0;
				i++;
				if (!strcmp(env, "$")) {
					sprintf(env, "%d", getpid());
					return env;			
				}
				else if (!strcmp(env, "?")) {
					sprintf(env, "%d", returnCode);
					return env;
				}
				else if (!strcmp(env, "!")) {
					sprintf(env, "%d", lastPID);
					return env;
				}
				else if (!strcmp(env, "_")) {
					env = lastCommand();
					return env;
				}
				else if (!strcmp(env, "SHELL")) {
					env = strdup(shellPath);
					return env;
				}
				else {
					char* var = getenv(env);
					if(var != NULL) {
						strcat(finalArg, var);
						k += strlen(var);
						free(env);
					}
					else {
						free(env);
						free(var);
						return NULL;
					}
				}
			}		
		}
		else {
			finalArg[k++] = arg[i++];	
		}	
	}
	return finalArg;
}

char * SimpleCommand::expandTilde( char * argument ) {
	char* arg = argument;
	if(!strcmp(arg, "~")) {
		if (!strcmp(arg, "~/") || strlen(arg) == 1) {
			argument = strdup(getenv("HOME"));
			return argument;
		}
	}
	else if (arg[0] == '~') {
		char path[strlen(arg)*2];
		strcpy(path, "/homes/");
		arg++; //remove ~
		strcat(path, arg);
		argument = strdup(path);
		return argument;
	}		
	return NULL;
}
void SimpleCommand::insertArgument( char * argument ) {
	if ( _numOfAvailableArguments == _numOfArguments  + 1 ) {
		// Double the available space
		_numOfAvailableArguments *= 2;
		_arguments = (char **) realloc( _arguments,
				  _numOfAvailableArguments * sizeof( char * ) );
	}

	char* expand = expandEnvironment(argument);
	if (expand != NULL) {
		argument = strdup(expand);
	}
	expand = expandTilde(argument);
	if (expand != NULL) {
		argument = strdup(expand);
	}
	
	_arguments[ _numOfArguments ] = argument;

	// Add NULL argument at the end
	_arguments[ _numOfArguments + 1] = NULL;
	
	_numOfArguments++;
}
