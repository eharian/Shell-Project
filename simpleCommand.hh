#ifndef simplcommand_h
#define simplecommand_h
struct SimpleCommand {
  // Available space for arguments currently preallocated
  int _numOfAvailableArguments;
  
  // Number of arguments
  int _numOfArguments;
  char ** _arguments;
  
 // int flag;
  static char* shellPath;
  static int returnCode;
  static int lastPID;
  SimpleCommand();
  void insertArgument( char * argument );
  char * expandEnvironment( char * argument );
  char * expandTilde( char * argument );
};

#endif
