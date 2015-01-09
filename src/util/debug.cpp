#include <signal.h>
#include "debug.h"

/***************************************************************************/

namespace
{

int local_no_debugger = 0;

void sigtrap_handler(int signum)
{
    local_no_debugger = 1;
    signal(SIGTRAP, SIG_DFL);
}

int gdb_check()
{
    signal(SIGTRAP, sigtrap_handler);
    raise(SIGTRAP);
    return local_no_debugger == 0;
}

} // anonymous namespace

/***************************************************************************/

bool debugger_is_present = (gdb_check() != 0);

/***************************************************************************/

