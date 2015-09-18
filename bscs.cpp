#include "stdafx.h"
#include "globals.h"

int main()
{
 	OpenControlLink();

	while(ControlLinkEnabled)
	{
		Sleep(5000);
		
		deb("main thread: waiting for control link shutdown...");
	}

	return 0;
}
