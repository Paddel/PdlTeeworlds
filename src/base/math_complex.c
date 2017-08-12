
#include <math.h>

#include "math_complex.h"

#if defined(__cplusplus)
extern "C" {
#endif


float cosinusf(float x)
{
	return cosf(x);
}

float sinusf(float x)
{
	return sinf(x);
}

float acosinusf(float x)
{
	return acosf(x);
}

float asinusf(float x)
{
	return asinf(x);
}


float atangensf(float x)
{
	return atanf(x);
}

float powerf(float x, float n)
{
	return powf(x, n);
}

float squarerootf(float x)
{
	return sqrtf(x);
}

float fmodulu(float x, float n)
{
	return fmod(x, n);
}

float fabsolute(float x)
{
	return fabs(x);
}

#ifdef __cplusplus
}
#endif