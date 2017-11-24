/* 
 * CS-252
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */
%code requires 
{
#include <string>
#include <iostream>
#if __cplusplus > 199711L
#define register      // Deprecated in C++11 so remove the keyword
#endif
}

%union
{
  char        *string_val;
  // Example of using a c++ type in yacc
  std::string *cpp_string;
}

%token <string_val> WORD
%token NOTOKEN GREAT NEWLINE GREATGREAT LESS GREATAMPERSAND GREATGREATAMPERSAND TWOGREAT PIPE AMPERSAND

%{
//#define yylex yylex
#include <cstdio>
#include <string.h>
#include <unistd.h>
#include <regex.h>
#include <sys/stat.h>
#include <dirent.h>
#include "command.hh"
void yyerror(const char * s);
int yylex();
void expandWildCardsIfNecessary(char* arg);
void expandWildCard(char* prefix, char* suffix);
int cmpfunc(const void* f1, const void* f2);
%}

%%

goal:
  commands
  ;
commands:
  command
  | commands command
  ;

command: simple_command
       ;

simple_command:	
  pipe_list io_modifier_list background_optional NEWLINE {
    //printf("   Yacc: Execute command\n");
    Command::_currentCommand.execute();
  }
  | NEWLINE {
    Command::_currentCommand.clear();
    Command::_currentCommand.prompt(); 
  }
  | error NEWLINE { yyerrok; }
  ;

command_and_args:
  command_word argument_list {
    Command::_currentCommand.
    insertSimpleCommand( Command::_currentSimpleCommand );
  }
  ;

argument_list:
  argument_list argument
  | /* can be empty */
  ;

io_modifier_list:
  io_modifier_list iomodifier_opt
  | /*empty*/
  ;

pipe_list:
  pipe_list PIPE command_and_args
  | command_and_args
  ;

background_optional:
  AMPERSAND {
    Command::_currentCommand._background = 1;
  }
  | /*empty*/
  ;

argument:
  WORD {
    //printf("   Yacc: insert argument \"%s\"\n", $1);
    expandWildCardsIfNecessary($1);
  }
  ;

command_word:
  WORD {
    //printf("   Yacc: insert command \"%s\"\n", $1);
    Command::_currentSimpleCommand = new SimpleCommand();
    Command::_currentSimpleCommand->insertArgument( $1 );
  }
  ;

iomodifier_opt:
  GREAT WORD {
    if (Command::_currentCommand._outFile) {
        std::cerr << "Ambiguous output redirect" << std::endl;
    } 
    else {
        Command::_currentCommand._outFile = $2;
    }
  }
  | GREATGREAT WORD {
    if (Command::_currentCommand._outFile) {
  	std::cerr << "Ambiguous output redirect" << std::endl;
    } 
    else {
        Command::_currentCommand._outFile = $2;
        Command::_currentCommand._append = 1;
    }
  }
  | GREATGREATAMPERSAND WORD {  
    if (Command::_currentCommand._outFile) {
  	std::cerr << "Ambiguous output redirect" << std::endl;
    } 
    else {
        Command::_currentCommand._outFile = $2;
        Command::_currentCommand._errFile = strdup($2);
        Command::_currentCommand._append = 1;
    }
  }
  | GREATAMPERSAND WORD {
    if (Command::_currentCommand._outFile) {
  	std::cerr << "Ambiguous output redirect" << std::endl;
    } 
    else {
        Command::_currentCommand._outFile = $2;
        Command::_currentCommand._errFile = strdup($2);
    }
  }
  | LESS WORD {
    if (Command::_currentCommand._outFile) {
  	std::cerr << "Ambiguous output redirect" << std::endl;
    } 
    else {
        Command::_currentCommand._inFile = $2;
    }
  }
  | TWOGREAT WORD {
    if (Command::_currentCommand._outFile) {
  	std::cerr << "Ambiguous output redirect" << std::endl;
    }
    else { 
        Command::_currentCommand._errFile = $2;
	Command::_currentCommand._errOnly = 1;
    }
  }
  ;

%%
int nEntries;
int maxEntries;
char ** entries;
int cmpfunc(const void* file1, const void* file2) {
	char* f1 = *(char**)file1;
	char* f2 = *(char**)file2;
	return strcmp(f1, f2);
}
void expandWildCardsIfNecessary(char* arg) {
	if (strchr(arg, '?') != NULL || strchr(arg, '*') != NULL) {
		nEntries = 0;
		maxEntries = 20;
		entries = (char**)malloc(sizeof(char*) * maxEntries);
		expandWildCard((char*)"", arg);
		if (*entries == NULL) {
			Command::_currentSimpleCommand->insertArgument(arg);
			return;
		}
		for(int i = 0; i < nEntries; i++) {
			Command::_currentSimpleCommand->insertArgument(entries[i]);
		}
		free(entries);
	}
	else {
		Command::_currentSimpleCommand->insertArgument(arg);
		return;
	}
}

void expandWildCard(char* prefix, char* suffix) {
	if (suffix[0] == 0) {
		return;
	}
	char* s = strchr(suffix, '/');
	char component[1024] = "";
	if (s != NULL) {
		strncpy(component, suffix, s-suffix);
		suffix = s+1;
	}
	else {
		strcpy(component, suffix);
		suffix += strlen(suffix);
	}
	char newPrefix[1024];
	if (strchr(component, '*') == NULL && strchr(component, '?') == NULL) {
		if (*prefix == '/' && strlen(prefix) == 1) {
			sprintf(newPrefix, "/%s", component);
		}
		else {
			sprintf(newPrefix, "%s/%s", prefix, component);
		}
		
		expandWildCard(newPrefix, suffix);
		return;
	}
	char * reg = (char*)malloc(2*strlen(component)+10);
	char * a = component;
	char * r = reg;
	*r = '^'; r++;
	while (*a) {
		if (*a == '*') {*r='.'; r++; *r='*'; r++;}
		else if (*a == '?') {*r = '.'; r++;}
		else if (*a == '.') {*r = '\\'; r++; *r = '.'; r++;}
		else {*r = *a; r++;}
		a++;
	}
	*r='$'; r++; *r = 0;
	regex_t re;
	int expbuff = regcomp(&re, reg, REG_EXTENDED|REG_NOSUB);
	if (expbuff != 0) {
		perror("compile");
		return;
	}
	char* open = strdup((*prefix == 0)?".":prefix);
	DIR * dir = opendir(open);
	if (dir == NULL) {
		return;
	} 
	struct dirent * ent;
	regmatch_t match;
	while ((ent = readdir(dir)) != NULL) {
		if (regexec(&re, ent->d_name,1, &match, 0) == 0) {
			if (*prefix == 0 || strlen(prefix) == 0) {
				sprintf(newPrefix, "%s", strdup(ent->d_name));
			}
			else if (*prefix == '/' && strlen(prefix) == 1) {
				sprintf(newPrefix, "/%s", strdup(ent->d_name));
			}
			else {
				sprintf(newPrefix, "%s/%s", prefix, strdup(ent->d_name));
			}
			expandWildCard(strdup(newPrefix), strdup(suffix));
			if (nEntries == maxEntries) {
				maxEntries *= 2;
				entries = (char**)realloc(entries, maxEntries * sizeof(char*));
			}
			if (*ent->d_name == '.') {
				if (*component == '.') {
					entries[nEntries++] = strdup(ent->d_name);
				}
			}
			else {
				if (suffix[0] == 0) {
					entries[nEntries++] = strdup(newPrefix);
				}
			} 
		}
	}
	closedir(dir);
	qsort(entries, nEntries, sizeof(char *), cmpfunc);
}
void
yyerror(const char * s)
{
  fprintf(stderr,"%s", s);
}

#if 0
main()
{
  yyparse();
}
#endif
