#ifndef _GLOBAL_H
#define _GLOBAL_H

#include <algorithm>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <time.h>
#include <math.h>

using namespace std;

//template <class T> T min(T a,T b){return (a < b)?a:b;}
//template <class T> T max(T a,T b){return (a > b)?a:b;}
template <class T> T abs(T a){return ((a) < 0)?-(a):(a);}
template <class T> T clamp(T val,T min,T max) {return val<min?min:val>max?max:val;};
template <class T> T div_signed(T val,T s){return val<0?-(((-val)+(1<<(s-1)))>>s):(val+(1<<(s-1)))>>s;};

typedef signed short int sint16_t;
typedef signed long long s64;
typedef unsigned long long u64;

#define LOG2I (1.442695040f)
#define MAXFNAME (1024)

#endif
