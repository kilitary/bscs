#include "stdafx.h"

void hexdump(char *buffer, int size)
{
	int i,d;
	char szOut[128];
	char szTemp[128];
	
	szOut[0] = 0x0;
	deb("== dumping buffer at %p size: %d\n", buffer, size);
	for( d=0 ; d < size ; d++ ) {
		wsprintf(szOut,"0x%x: ", buffer + d);
		for( i=0 ; i < 12 ; i++ ) {
			wsprintf(szTemp, "%02x ", (u_char) buffer[d++]);
			lstrcat(szOut, szTemp);
			if(d >= size)
				goto finish;
		}
finish:
		deb("%s",szOut);
	}
	deb("== end of dump.\n");
}

char* fmterr(void)
{
	LPVOID lpMsgBuf = NULL;
	static char szInternal[255] = {0};
	
	FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,NULL,GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf,0,NULL);
	
	lstrcpy(szInternal,(char*) lpMsgBuf);
	LocalFree(lpMsgBuf);
	return szInternal;
}

void fdeb(char *msg,...) 
{
	va_list ap;
	char string[32768];
	char stringout[32768];
	
	va_start(ap,msg);
	vsprintf(string, msg, ap);
	va_end(ap);

	wsprintf(stringout, "SERVER:<%X> %s", GetCurrentThreadId(), string);
	OutputDebugString(stringout);
	printf("%s\n", stringout);
}