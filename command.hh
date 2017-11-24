#ifndef command_h
#define command_h

#include "simpleCommand.hh"

// Command Data Structure

struct Command {
  int _numOfAvailableSimpleCommands;
  int _numOfSimpleCommands;
  SimpleCommand ** _simpleCommands;
  char * _outFile;
  char * _inFile;
  char * _errFile;
  int _background;
  int _append;
  int _errOnly;

  void prompt();
  void print();
  void execute();
  void clear();
  void runShellrc();
  int checkBuiltIn(int i);

  Command();
  void insertSimpleCommand( SimpleCommand * simpleCommand );
	
  static int _src;
  static int _tmpstdin;
  static Command _currentCommand;
  static SimpleCommand *_currentSimpleCommand;
};

#endif
