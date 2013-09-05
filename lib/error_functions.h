#ifndef ERROR_FUNCTIONS
#define ERROR_FUNCTIONS

// Prints a message on standard error.
void errMsg(const char *format, ...);

#ifdef __GNUC__

#define NORETURN __attribute__ ((__noreturn__))
#else
#define NORETURN
#endif

// Prints a msg to stderr and also terminates the program.
void errExit(const char *format, ...) NORETURN;

// Similar to errExit but doesn't flush stdout uses
// _exit instead of exit.
void err_exit(const char *format, ...) NORETURN;

// Same as errExit, but prints the error text given in the
// errnum parameter.
void errExitEN(int errnum, const char *format, ...) NORETURN;

/*****************************************************
 *                  General Errors
 ****************************************************/
// Diagnoses general errors.
void fatal(const char *format, ...) NORETURN;

// Diagnoses error in command-line argument usage.
void usageErr(const char *format, ...) NORETURN;

// Diagnoses errors in the command-line arguments.
void cmdLineErr(const char *format, ...) NORETURN;
#endif
