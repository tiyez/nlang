

#define Assert(expr, exprstring, file, line) ((expr) ? 1 : (fprintf (stderr, "%s(%d): Assertion failed: %s\n", file, line, exprstring), fflush (stderr), abort (), 0))
#define Unreachable(file, line) (fprintf (stderr, "%s(%d): unreachable codepath was reached\n", file, line), abort (), 0)
