#ifndef	_UTILITY_H
#define	_UTILITY_H

#include "string.h"
#include "typedef.h"
#include "stdlib.h"

int rtspstrncasecmp(char* s, char* tt, int len);
int rtspstrcasecmp(char* s, char* tt);
SCODE GetIntValueFromText(char *pcSrc, char *pcKeyWord, DWORD dwKeyWordLen, int *piData);
SCODE GetStringValueFromText(char *pcSrc, char *pcKeyWord, DWORD dwKeyWordLen, char *pcNameValue, DWORD dwNameValueLen);

#endif	/* _utility_h */


