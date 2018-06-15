/*
 *  File:       AsParse.h
 *
 */

#ifndef _HS_PARSE_
#define _HS_PARSE_

#define MAX_PARSE_LENGTH     300

// Parsing control structure
typedef struct
{
    char*    pszBeginBuffer;
    char*    pszCurrentBuffer;        
    char*    pszCurrentBeginLine;
    char*    pszCurrentEndOfLine;
    char*    pszCurrentEndOfBuffer;
    char     pszTempBuffer[MAX_PARSE_LENGTH];
    DWORD    dwDataBufferLength;         // Total data length in buffer
    DWORD    dwHttpObjectLengthToRead;  
    UINT     uiCompleteLineLength;      
    UINT     uiCurrentLineLength;       
    UINT     uiPartialLineLength;       
} TParsingControl, *PTParsingControl;

// Line parsing states
typedef enum
{
    LINE_COMPLETE,
    LINE_PARTIAL,
    LINE_ERROR,
    LINE_EMPTY
} TLineState;

#define HTTP_AUTH_USER_LENGTH			30
#define	HTTP_AUTH_NONCE_LENGTH			50
#define HTTP_AUTH_RESPONCE_LENGTH		33

typedef struct
{
	char acUserName[HTTP_AUTH_USER_LENGTH];
	char acNonce[HTTP_AUTH_NONCE_LENGTH];
	char acResponse[HTTP_AUTH_RESPONCE_LENGTH];
	char acNonceCount[9];
	char acCNonce[HTTP_AUTH_NONCE_LENGTH];
}THTTPRAWAUTHORINFO;

void ConvertTokenToLowerCase(char *pszToken, UINT uiTokenLength);
void ConvertHeaderToLowerCase(PTParsingControl ptParsing);
UINT FindLineEnd(char *pszStartOfToken);
UINT FindTokenEnd(char *pszStartOfToken);
char * FindTokenStart(char *pszBeginLine);
DWORD ParseDate(char *pszDateString);
TLineState GetLineFromBuffer(PTParsingControl ptParsing);
UINT FindTokenDelimited(char *pszStartOfToken, char cDelimiter);
char* FindTokenDelimitedPtr(char *pszStartOfToken, char cDelimiter);
char* FindValueStart(char *pszValue);
UINT FindValueLength(char *pszBeginLine);

int ParseAuthorDigestInfo(char *pcMsgBuffer,THTTPRAWAUTHORINFO *ptHttpRawAuthInfo);								  


#endif

