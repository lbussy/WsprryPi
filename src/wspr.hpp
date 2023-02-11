#ifndef _WSPR_H
#define _WSPR_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <dirent.h>
#include <math.h>
#include <cmath>
#include <cstdint>
#include <fcntl.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <malloc.h>
#include <time.h>
#include <sys/time.h>
#include <getopt.h>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <pthread.h>
#include <sys/timex.h>

#include "version.hpp"
#include "singleton.hpp"
#include "monitorfile.hpp"
#include "config.hpp"

#ifdef __cplusplus
extern "C"
{
#include "mailbox.h"
}
#endif /* __cplusplus */

#endif // _WSPR_H
