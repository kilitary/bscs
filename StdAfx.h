#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#ifndef DEBUG
//#pragma comment(linker,"/filealign:512 /IGNORE:4078")
//#pragma comment(linker,"/merge:.data=x /merge:.data1=x /merge:.rdata=x /merge:.text=x")
//#pragma comment(linker,"/entry:_main")
//#pragma comment(linker,"/section:x,WRX")
#pragma optimize("gsy",on)
#endif

#include <windows.h>
#include <Winsock2.h>
#include <stdio.h>
#include <stdlib.h>


#include "configuration.h"
#include "memory.h"
#include "mbscs.h"
#include "debug.h"
#include "low_sockets.h"

