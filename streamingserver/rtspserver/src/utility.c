#include "utility.h"

int rtspstrncasecmp(char* pcSource, char* pcTarget, int len)
{

	int		i;
	char	acTargetTemp[200];
	char	acSourceTemp[200];

	memset(acTargetTemp,0,200);
	memset(acSourceTemp,0,200);

	if( len < 200 )
	{
		strncpy(acSourceTemp,pcSource,len);
		strncpy(acTargetTemp,pcTarget,len);
	}
	else
		return -1;

   for( i=0; i<len; i++)
   {
     if( acSourceTemp[i] <= 90 && acSourceTemp[i] >=65 )
       acSourceTemp[i] = acSourceTemp[i] + 32;
     if( acTargetTemp[i] <= 90 && acTargetTemp[i] >=65 )
       acTargetTemp[i] = acTargetTemp[i] + 32;   	
   }
   
   for(i=0 ; acTargetTemp[i] == acSourceTemp[i] ; i++) 
   {
     if( i ==( len -1 ))
      return 0;
   }
    
   return acTargetTemp[i] - acSourceTemp[i];
   
}


int rtspstrcasecmp(char* s, char* tt)
{
   int i;
   char t[200];
  
   memset(t,0,200);
   strncpy(t,tt,199);
   
   for( i=0; s[i]!='\0'; i++)
   {
     if( s[i] <= 90 && s[i] >=65 )
       s[i] = s[i] + 32;
     if( t[i] <= 90 && t[i] >=65 )
       t[i] = t[i] + 32;   	
   }
   
   for(i=0 ; s[i] == t[i] ; i++)
     if( s[i] =='\0' )
     return 0;
     
   return s[i] - t[i];
}

DWORD rtspCheckTimeDifference(DWORD dwStartTime, DWORD dwEndTime)
{
	DWORD	dwElasped = 0;
	//printf("Start %u end %u\n", dwStartTime, dwEndTime);
	if(dwEndTime >= dwStartTime)
	{
		dwElasped = dwEndTime - dwStartTime;
	}
	else
	{
		dwElasped = 0xffffffff - dwStartTime + dwEndTime + 1;
	}

	return dwElasped;
}

/*char *rtspstrcpy(char *acDst, const char *pcSrc, unsigned int uiSize)
{
	memset(acDst, 0, uiSize);
	return strncpy(acDst, pcSrc, uiSize - 1);
}

char *rtspstrcat(char *acDst, const char *pcSrc, unsigned int uiSize)
{
	strncat(acDst, pcSrc, uiSize - strlen(acDst) - 1);
	acDst[uiSize - 1] = 0;	
	return acDst;
}*/

SCODE GetStringValueFromText(char *pcSrc, char *pcKeyWord, DWORD dwKeyWordLen, char *pcNameValue, DWORD dwNameValueLen)
{
	char	*pcFront = NULL, *pcBehind = NULL;
	int		iValueLen = 0;
	if(pcSrc[0] == '0' || pcKeyWord[0] == '0')
	{
		return S_FAIL;
	}
	if((pcFront = strstr(pcSrc, pcKeyWord)) != NULL)
	{
		pcFront += dwKeyWordLen;
		if((pcBehind = strstr(pcFront, "\r\n")) != NULL)
		{
			iValueLen = pcBehind - pcFront;
			if(dwNameValueLen >= iValueLen)
			{
				memcpy(pcNameValue, pcFront, iValueLen);
				pcNameValue[iValueLen] = '\0';
				return S_OK;
			}
			else
			{
				return S_FAIL;
			}
		}
		else
		{
			return S_FAIL;
		}
	}
	else
	{
		return S_FAIL;
	}
}

SCODE GetIntValueFromText(char *pcSrc, char *pcKeyWord, DWORD dwKeyWordLen, int *piData)
{
	// declarations
	char	acData[128];
	char	*pcFront = NULL, *pcBehind = NULL;
	int		iValueLen = 0;

	if(pcSrc[0] == '0' || pcKeyWord[0] == '0')
	{
		return S_FAIL;
	}
	if((pcFront = strstr(pcSrc, pcKeyWord)) != NULL)
	{
		pcFront += dwKeyWordLen;
		if((pcBehind = strstr(pcFront, "\r\n")) != NULL)
		{
			iValueLen = pcBehind - pcFront;
			if(sizeof(acData)-1 >= iValueLen)
			{
				memcpy(acData, pcFront, iValueLen);
				acData[iValueLen]='\0';
				*piData = (ULONG)atoi(acData);
				return S_OK;
			}
			else
			{
				return S_FAIL;
			}
		}
		else
		{
			return S_FAIL;
		}
	}
	else
	{
		return S_FAIL;
	}
}
