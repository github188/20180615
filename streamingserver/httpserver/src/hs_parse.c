/*  Copyright (c) 2003 Vivotek Inc. All rights reserved.
 *  +-----------------------------------------------------------------+
 *  | THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY ONLY BE USED | 
 *  | AND COPIED IN ACCORDANCE WITH THE TERMS AND CONDITIONS OF SUCH  | 
 *  | A LICENSE AND WITH THE INCLUSION OF THE THIS COPY RIGHT NOTICE. | 
 *  | THIS SOFTWARE OR ANY OTHER COPIES OF THIS SOFTWARE MAY NOT BE   | 
 *  | PROVIDED OR OTHERWISE MADE AVAILABLE TO ANY OTHER PERSON. THE   | 
 *  | OWNERSHIP AND TITLE OF THIS SOFTWARE IS NOT TRANSFERRED.        | 
 *  |                                                                 | 
 *  | THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT   | 
 *  | ANY PRIOR NOTICE AND SHOULD NOT BE CONSTRUED AS A COMMITMENT BY | 
 *  | VIVOTEK INC.                                                    | 
 *  +-----------------------------------------------------------------+ 
 *  Module name        :   HTTPServer
 *  File name          :   parse.c
 *  File description   :   HTTP Server Module API
 *  Author             :   Jason Yang
 *  Created at         :   2003/05/26
 *  Note               :   
 */


/*!
 **********************************************************************
 * Copyright (C) 2003 Vivotek, Inc. All rights reserved.
 *
 * \file
 * parse.c
 *
 * \brief
 * HTTP Server Module API
 *
 * \date
 * 2003/05/26
 *
 * \author
 * Jason Yang
 *
 *
 **********************************************************************
 */

#include "httpserver_local.h"
//#include "osisolate.h"
//#include "hs_parse.h"

static UINT MonthToNumber(char *pszMonth);
static void ParseTime(char *pszTime, struct tm *pts);
static void ParseTime2(char *pszTime, TOSDateTimeInfo *ptTimeInfo);


const char * gMonthTable[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                               "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };


void ConvertHeaderToLowerCase(PTParsingControl ptParsing)
{
    char * pszLine;
    UINT  uiTokenLength;

    pszLine = ptParsing->pszCurrentBeginLine;

    //A HTTP header line is of the format <headername>:<headervalues>.
    //This routine converts the <headername> to lower case.
    uiTokenLength = FindTokenDelimited(pszLine, ASCII_Colon);
    if (uiTokenLength < ptParsing->uiCompleteLineLength)
    {
        ConvertTokenToLowerCase(pszLine, uiTokenLength);
    }
    return;
}

void ConvertTokenToLowerCase(char *pszToken, UINT uiTokenLength)
{
    char cChar;

    while (uiTokenLength > 0)
    {
        cChar = *pszToken;
        if (cChar >= ASCII_A && cChar <= ASCII_Z)
        {
            *pszToken = cChar ^ ASCII_Space;
        }
        uiTokenLength--;
        pszToken++;
    }
    return;
}

UINT FindLineEnd(char *pszStartOfToken)
{
    UINT  uiTokenLength;

    uiTokenLength = 0;
    while ( *pszStartOfToken != '\x0a' &&
            *pszStartOfToken != '\x0d' )
    {
        pszStartOfToken += 1;
        uiTokenLength += 1;
    }
    return uiTokenLength;
}

UINT FindTokenEnd(char *pszStartOfToken)
{
    UINT  uiTokenLength;

    uiTokenLength = 0;
    while ( *pszStartOfToken != ' '  &&
            *pszStartOfToken != '\t' &&
            *pszStartOfToken != ';'  &&
            *pszStartOfToken != '\0' &&
            *pszStartOfToken != '\x0a' &&
            *pszStartOfToken != '\x0d' )
    {
        pszStartOfToken += 1;
        uiTokenLength += 1;
    }
    return uiTokenLength;
}

char * FindTokenStart(char *pszBeginLine)
{
    char *  pszStartOfToken;

    pszStartOfToken = pszBeginLine;
    while ( *pszStartOfToken == ' '  ||
            *pszStartOfToken == ':'  ||
            *pszStartOfToken == '\t' )
    {
        pszStartOfToken += 1;
    }
    return pszStartOfToken;
}

/*
    This routine parses an Internet format date string and
    turns it into a number of seconds in the internal
    system time format.

    Dates can be passed in three formats:
        Sun, 06 Nov 1994 08:49:37 GMT   ; RFC 822/RFC 1123
        Sunday, 06-Nov-94 08:49:37 GMT  ; RFC 850/RFC 1036
        Sun Nov 6 08:49:37 1994         ; ANSI C's asctime() format

    NOTES:  The first item (the day of the week) is optional.
            The day number (06 in this example) may be 1 or 2 digits.
            The year number (1994 in this example) may be 2 or 4 digits.

    For compatability with HTTP 1.0, dates must be accepted in any format,
    although they will be generated only in RFC 1123 format.
*/

DWORD ParseDate(char *pszDateString)
{
	TOSDateTimeInfo	 	tDateInfo;
    UINT uiTokenLength;
    
    memset(&tDateInfo, 0, sizeof(tDateInfo));
    
    if (*pszDateString < ASCII_0 || *pszDateString > ASCII_9)
    {
        /*
            The first character is not a number.  This
            token must be the day of the week, so skip it.
        */
        uiTokenLength = FindTokenEnd(pszDateString);
        pszDateString = FindTokenStart(pszDateString + uiTokenLength);
    }

    /*
        the second token (or possibly first) is either the month,
        the day of the month, or the RFC 850 date.
    */
    uiTokenLength = FindTokenEnd(pszDateString);
    if (uiTokenLength == 3)
    {
        /*
            well it must be a month, so we have ANSI-C asctime format.
            first make it a C string
        */
        *(pszDateString + uiTokenLength) = '\0';
        tDateInfo.wMonth = MonthToNumber(pszDateString) + 1;
        pszDateString += 4;

        /*
            the third token in asctime will be the day.
            make it a C string after finding its start and length
        */
        pszDateString = FindTokenStart(pszDateString);
        uiTokenLength = FindTokenEnd(pszDateString);
        *(pszDateString + uiTokenLength) = '\0';
        tDateInfo.wMonthDay = atoi(pszDateString);
        pszDateString += uiTokenLength + 1;

        /*
            the fourth token in asctime will be the hours, minutes, seconds
        */
        ParseTime2(pszDateString, &tDateInfo);
        
        pszDateString += 8;

        /*
            the fifth token in asctime format will be the year
            make it a C string after finding its length
        */
        uiTokenLength = FindTokenEnd(pszDateString);
        *(pszDateString + uiTokenLength) = '\0';
        tDateInfo.wYear = atoi(pszDateString);
    }
    else
    {
        /*
            well it must be RFC 822 format or RFC 850 format which may be
            handled the same way, if we're careful.
            first make the day a C string
        */
        uiTokenLength = FindTokenEnd(pszDateString);
        if (uiTokenLength > 2)
        {
            /*
                The character separating the day from the month
                must be a hyphen.
            */
            uiTokenLength = FindTokenDelimited(pszDateString, ASCII_Hyphen);
        }
        *(pszDateString + uiTokenLength) = '\0';
        tDateInfo.wMonthDay = atoi(pszDateString);
        pszDateString += uiTokenLength + 1;

        /*
            the third token in RFC 822 format or RFC 850 format will be
            the month. First make it a C string.
        */
        *(pszDateString + 3) = '\0';
        tDateInfo.wMonth = MonthToNumber(pszDateString) + 1;
        pszDateString += 4;

        /*
            the fourth token in RFC 822 format or RFC 850 format will be
            the year. Make it a C string after finding its length
        */
        uiTokenLength = FindTokenEnd(pszDateString);
        *(pszDateString + uiTokenLength) = '\0';
        tDateInfo.wYear = atoi(pszDateString);
        pszDateString += uiTokenLength + 1;

        /*
            the fifth token in RFC 822 format or RFC 850 format
            will be the hours, minutes, seconds
        */
        ParseTime2(pszDateString, &tDateInfo);
    }
	return OSTime_DateTimeToTimer(&tDateInfo);
}

static UINT MonthToNumber(char *pszMonth)
{
    UINT uiMonthNumber;

    uiMonthNumber = 0;
    while (strcmp(pszMonth, (char *) gMonthTable[uiMonthNumber]) != 0
           && uiMonthNumber < 11)
    {
        uiMonthNumber += 1;
    }

    return uiMonthNumber;
}

static void ParseTime(char *pszTime, struct tm *pts)
{
    *(pszTime + 2) = '\0';
    pts->tm_hour = atoi(pszTime);
    pszTime += 3;
    *(pszTime + 2) = '\0';
    pts->tm_min = atoi(pszTime);
    pszTime += 3;
    *(pszTime + 2) = '\0';
    pts->tm_sec = atoi(pszTime);
    
    return;
}

static void ParseTime2(char *pszTime, TOSDateTimeInfo *ptTimeInfo)
{
    *(pszTime + 2) = '\0';
    ptTimeInfo->wHour = atoi(pszTime);
    pszTime += 3;
    *(pszTime + 2) = '\0';
    ptTimeInfo->wMinute = atoi(pszTime);
    pszTime += 3;
    *(pszTime + 2) = '\0';
    ptTimeInfo->wSecond = atoi(pszTime);
    
    return;
}

TLineState GetLineFromBuffer(PTParsingControl ptParsing)
{
    char*  pszBeginLine;
    char*  pszEndOfBuffer;
    char*  pszEndOfLine;
    UINT   uiFragmentLength;
    
    UINT   uiCompleteLineLength;
    char*  pszTempBuffer;
    UINT   uiCurrentLength;
    UINT   uiPartialLength;
    TLineState  tState;

    ptParsing->pszCurrentBeginLine = ptParsing->pszCurrentBuffer;
    ptParsing->pszCurrentEndOfLine = ptParsing->pszCurrentBeginLine;
    ptParsing->pszCurrentEndOfBuffer = ptParsing->pszCurrentBeginLine +
                                       ptParsing->dwDataBufferLength;
                                       

    if (ptParsing->dwDataBufferLength == 0)
    {
        // The last complete line read happened to line up on the buffer
        // boundary.  All we need to do is return the line state
        // as LINE_PARTIAL, so the caller will go get another buffer.
        return LINE_PARTIAL;
    }
    
    ptParsing->uiCurrentLineLength = 0;
    uiFragmentLength = 0;
    pszEndOfBuffer = ptParsing->pszCurrentEndOfBuffer;
    pszEndOfLine = ptParsing->pszCurrentEndOfLine;
    pszBeginLine = ptParsing->pszCurrentBeginLine;
    tState = LINE_ERROR;
  
    // To find the end of a line   
    while (uiFragmentLength == 0)
    {
        if (pszEndOfLine < pszEndOfBuffer)
        {
            if (*pszEndOfLine == '\x0a')
            {
                // we got a line!
                uiFragmentLength = (UINT) (pszEndOfLine - pszBeginLine + 1);
                if (uiFragmentLength <= MAX_PARSE_LENGTH)
                {
                    tState = LINE_COMPLETE;
                    ptParsing->uiCurrentLineLength = uiFragmentLength;
                }
            }
        }
        else
        {
            // we're at the end of the buffer and we have only a partial
            // line, so save it away, and return for another buffer.
            uiFragmentLength = (UINT) (pszEndOfLine - pszBeginLine);
            if (uiFragmentLength <= MAX_PARSE_LENGTH - ptParsing->uiPartialLineLength)
            {
                tState = LINE_PARTIAL;
                
                // Compose the partial line for continue two partial lines
                pszTempBuffer = ptParsing->pszTempBuffer + ptParsing->uiPartialLineLength;
                memcpy(pszTempBuffer, pszBeginLine, uiFragmentLength);
                ptParsing->uiPartialLineLength += uiFragmentLength;
            }
            // Else is in Error state
            
            ptParsing->dwDataBufferLength = 0;
        }
        pszEndOfLine += 1;
    }

    ptParsing->pszCurrentEndOfLine = pszEndOfLine;

    // If find one line , check the partial line
    if (tState == LINE_COMPLETE)
    {
        uiCurrentLength = ptParsing->uiCurrentLineLength;
        uiPartialLength = ptParsing->uiPartialLineLength;
        if (uiPartialLength > 0)
        {
            // we need to join the two partial lines
            uiCompleteLineLength = uiCurrentLength + uiPartialLength;
            if (uiCompleteLineLength > MAX_PARSE_LENGTH)
            {
                DbgPrint(("RpParseReplyBuffer, line too long, length = %d\n",
                            uiCompleteLineLength));
                tState = LINE_ERROR;
            }
            else
            {
                pszTempBuffer = ptParsing->pszTempBuffer + uiPartialLength;
                memcpy(pszTempBuffer, ptParsing->pszCurrentBeginLine,
                       uiCurrentLength);
                ptParsing->pszCurrentBeginLine = ptParsing->pszTempBuffer;
                ptParsing->uiCompleteLineLength = uiCompleteLineLength;
            }
            ptParsing->uiPartialLineLength = 0;
        }
        else
        {
            ptParsing->uiCompleteLineLength = uiCurrentLength;
        }
        if (ptParsing->uiCompleteLineLength <= 2)
        {
            tState = LINE_EMPTY;
        }
        
        // set up to process the rest of the buffer
        ptParsing->pszCurrentBuffer = ptParsing->pszCurrentEndOfLine;
        ptParsing->dwDataBufferLength -= ptParsing->uiCurrentLineLength;

        // the content length include the multipart header length        
        if (ptParsing->dwHttpObjectLengthToRead > 0)
        {
            ptParsing->dwHttpObjectLengthToRead -= ptParsing->uiCompleteLineLength;
        }
    }

    return tState;
}

char* FindValueStart(char *pszValue)
{
    while (*pszValue != ASCII_Equal)
    {
        if (*pszValue == ASCII_Return || *pszValue == ASCII_Newline)
        {
            *pszValue = ASCII_Null;
            break;
        }
        else
        {
            pszValue++;
        }
    }

    if (*pszValue == ASCII_Equal)
    {
        pszValue++;
        if (*pszValue == ASCII_Quote)
        {
            pszValue++;
        }
    }

    return pszValue;
}

UINT FindValueLength(char *pszBeginLine)
{
    char*  pszValue;

    pszValue = pszBeginLine;

    while (*pszValue != ASCII_Quote &&
           *pszValue != ASCII_Comma &&
           *pszValue != ASCII_Return &&
           *pszValue != ASCII_Newline)
    {
        pszValue++;
    }

    return pszValue - pszBeginLine;
}

UINT FindTokenDelimited(char *pszStartOfToken, char cDelimiter)
{
    UINT  uiTokenLength;

    /*
        Return an offset to the location in the string of the character
        contained in theDelimiter.  If not found the offset points to
        the end of the string.
    */
    uiTokenLength = 0;
    while (*pszStartOfToken != '\0' && *pszStartOfToken != cDelimiter)
    {
        pszStartOfToken += 1;
        uiTokenLength     += 1;
    }
    return uiTokenLength;
}

char* FindTokenDelimitedPtr(char *pszStartOfToken, char cDelimiter)
{
    UINT  uiTokenLength;
    char* pszTokenPtr;

    /*
        Return a character pointer to the location of the character contained
        in theDelimiter. If not found the char pointer is set to null. This
        routine is equivalent to the standard C library routine 'strchr'.
    */
    uiTokenLength = FindTokenDelimited(pszStartOfToken, cDelimiter);
    // Modified by Joe 2003/08/18, to avoid if pszStartOfToken is not a string
    //if (uiTokenLength == strlen(pszStartOfToken))
    if(pszStartOfToken[uiTokenLength] == 0)
    {
        pszTokenPtr = (char *) 0;
    }
    else
    {
        pszTokenPtr = pszStartOfToken + uiTokenLength;
    }
    return pszTokenPtr;
}

//ShengFu 2006/01/06 add digest authentication

int ParseAuthorDigestInfo(char *pcMsgBuffer,THTTPRAWAUTHORINFO *ptHttpRawAuthInfo)								  
{
    char *pcChar = NULL,*pcEnd = NULL,*pcTemp=NULL ;
    
	pcChar = strstr(pcMsgBuffer,"Digest");
    
    if( pcChar == NULL )
        return -1;
    

	if( ( pcTemp = strstr(pcMsgBuffer,"username=\"") )== NULL )
		return -1;
	else
	{
		pcTemp = pcTemp + strlen("username=\""); 
		pcEnd = strchr(pcTemp,'"');

		if( pcEnd == NULL )
			return -1;
		else
		{	
			if( pcEnd-pcTemp < HTTP_AUTH_USER_LENGTH )
			{
				memcpy(ptHttpRawAuthInfo->acUserName,pcTemp,pcEnd-pcTemp);
				ptHttpRawAuthInfo->acUserName[pcEnd-pcTemp] = 0;
			}
			else
				return -1;
		}
	}

	if( ( pcTemp = strstr(pcMsgBuffer,"nonce=\"") )== NULL )
		return -1;
	else
	{
		pcTemp = pcTemp + strlen("nonce=\"") ; //skip '=' and '"'
		pcEnd = strchr(pcTemp,'"');
		if( pcEnd == NULL )
			return -1;
		else
		{
			if( pcEnd-pcTemp < HTTP_AUTH_NONCE_LENGTH )
			{
				memcpy(ptHttpRawAuthInfo->acNonce,pcTemp,pcEnd-pcTemp);
				ptHttpRawAuthInfo->acNonce[pcEnd-pcTemp] = 0;
			}
			else
				return -1;
		}

	}
	
	if( ( pcTemp = strstr(pcMsgBuffer,"nc=") )== NULL )
		return -1;
	else
	{
		pcTemp = pcTemp + strlen("nc="); 
		pcEnd = strchr(pcTemp,',');

		if( pcEnd == NULL )
			return -1;
		else
		{
			if( pcEnd-pcTemp < 9 )
			{
				memcpy(ptHttpRawAuthInfo->acNonceCount,pcTemp,pcEnd-pcTemp);
				ptHttpRawAuthInfo->acNonceCount[pcEnd-pcTemp] = 0;
			}
			else
				return -1;
		}
	}

	if( ( pcTemp = strstr(pcMsgBuffer,"cnonce=\"") )== NULL )
		return -1;
	else
	{
		pcTemp = pcTemp + strlen("cnonce=\"");
		pcEnd = strchr(pcTemp,'"');

		if( pcEnd == NULL )
			return -1;
		else
		{
			if( pcEnd-pcTemp < HTTP_AUTH_NONCE_LENGTH )
			{
				memcpy(ptHttpRawAuthInfo->acCNonce,pcTemp,pcEnd-pcTemp);
				ptHttpRawAuthInfo->acCNonce[pcEnd-pcTemp] = 0;
			}
			else
				return -1;
		}
	}

	if( ( pcTemp = strstr(pcMsgBuffer,"response=\"") )== NULL )
		return -1;
	else
	{
		pcTemp = pcTemp + strlen("response=\"");
		pcEnd = strchr(pcTemp,'"');

		if( pcEnd == NULL )
			return -1;
		else
		{
			if( pcEnd-pcTemp < HTTP_AUTH_RESPONCE_LENGTH )
			{
				memcpy(ptHttpRawAuthInfo->acResponse,pcTemp,pcEnd-pcTemp);
				ptHttpRawAuthInfo->acResponse[pcEnd-pcTemp] = 0;
			}
			else
				return -1;
		}
	}

	return 0;							
		                          
}

