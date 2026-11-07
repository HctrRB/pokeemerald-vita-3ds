#ifndef GUARD_PORT_LOG_H
#define GUARD_PORT_LOG_H

#include <stdio.h>
#include "../vita/vita_platform.h"

#ifndef PORT_LOG
#define PORT_LOG(...) VITA_LOG(__VA_ARGS__)
#endif

#endif // GUARD_PORT_LOG_H
