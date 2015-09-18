#include "stdafx.h"

void *halloc(u_int size)
{
	void *p = NULL;

	p = HeapAlloc(GetProcessHeap(), 0, size);
	if(p == NULL) {
		deb("halloc:HeapAlloc: %s", FORMATERROR);
		return NULL;
	}
	//deb("halloc: memory at 0x%x ", p);
	return p;
}

void hfree(void *p)
{
	//deb("hfree: memory at 0x%x ", p);
	if(!HeapFree(GetProcessHeap(), 0, p)) {
		deb("hfree:heapfree: %s", FORMATERROR);
	}
}