/*
 * CS354: Operating Systems. 
 * Purdue University
 * Example that shows how to read one line with simple editing
 * using raw terminal.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#define MAX_BUFFER_LINE 2048

extern void tty_raw_mode(void);

// Buffer where line is stored
int line_length;
char line_buffer[MAX_BUFFER_LINE];
int line_pos = 0;
// Simple history array
// This history does not change. 
// Yours have to be updated.
int history_index = 0;
char ** history;
int history_length = 0;
int history_max = 2;
void BACKSPACE(int start, int end) {
  char ch = 8;
  for (int i = start; i < end; i++) {
	write(1, &ch, 1);
  }
}
void CLEAR(int n) {
  BACKSPACE(0, n);
  char ch = ' ';
  for(int i = 0; i < n; i++) {
  	write(1, &ch, 1);
  }
  BACKSPACE(0, n);
}
void initHistory() {
  history = (char**)malloc(history_max * sizeof(char*));
}
void addHistory(char* line) {
 char* entry = (char*)malloc(strlen(line) * sizeof(char*));
 if(strlen(entry) != line_length) {
	int i;
	for(i = 0; i < line_length; i++) {
		entry[i] = line_buffer[i];
	}
	entry[i] = '\0';
	strncpy(line_buffer, entry, line_length);
 }
 else {
	entry = strdup(line);
 }		
 if (entry[strlen(entry)-1] == '\n') {
 	entry[strlen(entry)-1] = 0;
 }
 if (history_length == history_max) {
  	history_max *= 2;
	history = (char**)realloc(history, history_max * (sizeof(char*)));
  }
  history[history_index++] = entry;
  history_length++;
}
void writeHistory() {
  if (history[history_index] == NULL) {
  	strcpy(line_buffer, "");
  }
  else {
  	strcpy(line_buffer, history[history_index]);
  }
  line_length = strlen(line_buffer);
  write(1, line_buffer, line_length);
}
char* lastCommand() {
   if(history_index > 1) {
	char* hist = strdup(history[history_index-2]);
        return hist;
   }
   return NULL;
}	
void read_line_print_usage()
{
  char * usage = "\n"
    " ctrl-?       Print usage\n"
    " Backspace    Deletes last character\n"
    " ctrl-D       Deletes character at cursor\n"
    " ctrl-H       Deletes last character\n"
    " ctrl-A       Move cursor to the beginning of the line\n"
    " ctrl-E       Move cursor to the end of the line\n"
    " up arrow     See last command in the history\n"
    " down arrow   See next command in the history\n"
    " left arrow   Move cursor to the left\n"
    " right arrow  Move cursory to the right\n";
  write(1, &usage, strlen(usage));
}

/* 
 * Input a line with some basic editing.
 */
char * read_line() {
  // Set terminal in raw mode
  struct termios orig_attr;
  tcgetattr(0, &orig_attr);
  tty_raw_mode();

  line_length = 0;
  line_pos = 0;
  
  if(history == NULL) {
  	initHistory();
  }
  // Read one line until enter is typed
  while (1) {

    // Read one character in raw mode.
    char ch;
    read(0, &ch, 1);

    if (ch>=32 && ch < 127) {
      // It is a printable character. 
      // If max number of character reached return.
      if (line_length==MAX_BUFFER_LINE-2) break; 
      if (line_pos == line_length) {
		write(1, &ch, 1);
		line_buffer[line_length] = ch;
		line_length++;
		line_pos++;
      }
      else {
  	    //shift characters
	      int i;
    	  for(i = line_length; i > line_pos; i--) {
			line_buffer[i] = line_buffer[i-1];
    	  }
    	  line_buffer[line_pos] = ch;
  	  // Do echo
    	  // add char to buffer.
    	  line_length++;
  	  for(i = line_pos; i < line_length; i++) {
     	 		write(1, &line_buffer[i], 1);
    	  }
    	  line_pos++;
    	  BACKSPACE(line_pos, line_length);
      }
    }
    else if (ch == 1) {
	//ctrl a
	BACKSPACE(0, line_pos);
	line_pos = 0;
    }
    else if (ch == 5) {
	//ctrl e
 	 while (line_pos < line_length) {
  		write(1, &line_buffer[line_pos], 1);
       		line_pos++;
  	 }
    }
    else if (ch == 4) {
	//ctrl d
	if (line_pos == line_length) {
		continue;
	}
	for(int i = line_pos; i < line_length; i++) {
		line_buffer[i] = line_buffer[i+1];
	}
	line_buffer[line_length--] = '\0';
	for(int i = line_pos; i < line_length; i++) {
		write(1, &line_buffer[i], 1);
	}
	ch = 32;
	write(1, &ch, 1);
	BACKSPACE(0, 1);	
	BACKSPACE(line_pos, line_length);	
    }
    else if (ch==10) {
      // <Enter> was typed. Return line
      
      //Add to history
      if (strlen(line_buffer) > 0) {
      		addHistory(line_buffer);
      }
      // Print newline
      write(1, &ch, 1);
      break;
    }
    else if (ch == 31) {
      // ctrl-?
      read_line_print_usage();
      line_buffer[0]=0;
      break;
    }
    else if (ch == 8 || ch == 127) {
      // <backspace> was typed. Remove previous character read.
      if(line_pos <= 0 || line_pos <= 0) {
      		line_pos = 0;
		continue;
      }
      for(int i = line_pos; i < line_length; i++) {
		line_buffer[i-1] = line_buffer[i];
      }
      line_pos--;
      line_length--;
      BACKSPACE(0, 1);
      for(int i = line_pos; i < line_length; i++) {
		write(1, &line_buffer[i], 1);
      }
      ch = 32;
      write(1, &ch, 1);
      BACKSPACE(0, 1);
      BACKSPACE(line_pos, line_length);
    }
    else if (ch==27) {
      // Escape sequence. Read two chars more
      //
      // HINT: Use the program "keyboard-example" to
      // see the ascii code for the different chars typed.
      //
      char ch1; 
      char ch2;
      read(0, &ch1, 1);
      read(0, &ch2, 1);
      if (ch1==91 && ch2==65) {
	// Up arrow. Print next line in history.
      		if (history_index != 0) {	
			CLEAR(line_length);
			history_index--;
			writeHistory();
		}
      }
      else if (ch1 == 91 && ch2 == 66) {
	//Down arrow.
		if (history_index != history_length) {
			CLEAR(line_length);
			history_index++;
			writeHistory();
		}
      }
      else if (ch1 == 91 && ch2 == 68) {
	//Left arrow.
	if (line_pos > 0) {
		line_pos--;
		BACKSPACE(0,1);
	}
      }
      else if (ch1 == 91 && ch2 == 67) {
	//Right arrow
	if (line_length > line_pos) {
		write(1, &line_buffer[line_pos], 1);
		line_pos++;
	}
      }
    }

  }

  // Add eol and null char at the end of string
  line_buffer[line_length]=10;
  line_length++;
  line_buffer[line_length]=0;

  tcsetattr(0, TCSANOW, &orig_attr);
  return line_buffer;
}

