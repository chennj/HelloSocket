#ifndef _INIT_H_
#define _INIT_H_

#include<assert.h>

#ifdef _DEBUG
#include<stdio.h>
#define xPrintf(...) printf(__VA_ARGS__)
#else
#define xPrintf(...)
#endif

#endif

