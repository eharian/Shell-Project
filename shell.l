
/*
 *
 * CS-252 Fall 2017
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%{
#include <string>
#include <cstring>
#include <string.h>
#include "y.tab.hh"
#include <unistd.h>
#include "command.hh"
static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
  unput(c);
}

extern "C" char* read_line();
int mygetc(FILE* f) {
  static char* p;
  char ch;
  
  if (!isatty(0)) {
     return getc(f);
  }
  if (p == NULL || *p == 0) {
     char * s = read_line();
     p = s;
  }
  ch = *p;
  p++;
  return ch;
}
#undef getc
#define getc(f) mygetc(f)

%}

%%

<<EOF>> {
  if (Command::_src) { 
	dup2(Command::_tmpstdin, 0);
	close(Command::_tmpstdin);
  	Command::_src = 0;
	Command::_currentCommand.prompt();
	yyrestart(stdin);
  }  
  else {
    	yyterminate();
  }
}

\n {
  return NEWLINE;
}

[ \t] {
  /* Discard spaces and tabs */
}

">" {
  return GREAT;
}

"|" {
  return PIPE;
}

"<" {
  return LESS;
}

"&" {
  return AMPERSAND;
}

">>" {
  return GREATGREAT;
}

"2>" {
  return TWOGREAT;
}

">&" {
  return GREATAMPERSAND;
}

">>&" {
  return GREATGREATAMPERSAND;
}

[\"][^\n\"]*[\"] { //Implement quotes. Checks that there is max 2 quotes
  char* text = strdup(yytext);
  text = text + 1; //remove first quote
  text[strlen(yytext) - 2] = '\0'; //remove last quote
  yylval.string_val = strdup(text);
  return WORD;
}

[^ \t\n>|<&]*[\\][^ \t\n]* { /*Implement escape characters. Cannot include tokens above before \ */
  char str[(strlen(yytext))];
  char* text = yytext;
  int i = 0;
  
  while (*text != '\0') {
	if (*text != '\\') {
		str[i++] = *text;
	}
	else { /*user wants to enter escape character*/
		if (*(text + 1) != '\\') { //inserting a token
			text++; 
			str[i++] = *(text);
		}
		else { //multiple escape characters
			text += 2;
			str[i++] = '\\';
		}
	}
	text++;
  }
  str[i] = '\0';
  yylval.string_val = strdup(str);
  return WORD;
}

[`][^`\n]*[`] { //subshell
	int tmpin = dup(0);
	int tmpout = dup(1);
	int fdpipe_in[2];
	int fdpipe_out[2];
	int ret;
  	
	char* text = strdup(yytext);
  	text = text + 1; //remove first backtick
  	text[strlen(yytext) - 2] = '\0'; //remove last backtick
	
	pipe(fdpipe_in);
	pipe(fdpipe_out);
	
	//write command in backticks to the pipe
	write(fdpipe_in[1], text, strlen(text));
	write(fdpipe_in[1], "\nquit\n", 6);
	close(fdpipe_in[1]);
	
	dup2(fdpipe_in[0],0);
	dup2(fdpipe_out[1], 1);
	close(fdpipe_in[0]);
	close(fdpipe_out[1]);
	
	ret = fork();
	if (ret == 0) { //child process executes the command and writes output to the other pipe
		execvp("/proc/self/exe", NULL);
		perror("execvp");
		exit(1);
	}
	else if (ret < 0) {
		perror("fork");
		exit(1);
	}
	
	dup2(tmpin, 0);
	dup2(tmpout, 1);
	close(tmpin);
	close(tmpout);

	std::string buff;
	char str;
	//read the output of the pipe from the child process
	while (read(fdpipe_out[0], &str, 1)) {
		 if (str != '\n') {
			buff += str;
		}
		else {
			buff += ' ';
		}
	}
	buff += '\0';
	for (int i = buff.length(); i >= 0; i--) {
		unput(buff[i]);
	}
}
		
[^ \t\n>|<&][^ \t\n>|<&]*  {
  /* Assume that file names have only alpha chars */
  yylval.string_val = strdup(yytext);
  return WORD;
}
