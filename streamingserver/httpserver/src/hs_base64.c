/*
 *  File:       AsBase64.c
 *
 */

#include "httpserver_local.h"


static UCHAR ConvertBase64Character(UCHAR ucInputCharacter);


/*
    DecodeBase64Data decodes an array of characters encoded as described
    in section 5.2 of RFC 1521 (MIME Part One)
    "Base64 Content-Transfer-Encoding"

    * theInputPtr points to an array of bytes that is at least as long
      as theInputCount
    * theInputPtr must not contain any white space that should be ignored
    * theOutputPtr is long enough to accept the result including a
      terminating null
*/
DWORD DecodeBase64Data(char *pszInputBuf, UINT uiInputLen, char *pszOutputBuf)
{
    DWORD  dwPackedGroup;
    char*  pszOriginalOutput;

    pszOriginalOutput = pszOutputBuf;

    // Make sure dwInputLen is a multiple of 4.
    uiInputLen -= (uiInputLen % 4);

    while (uiInputLen > 0)
    {
        dwPackedGroup = (DWORD) ConvertBase64Character(*pszInputBuf++) << 18;
        dwPackedGroup |= (DWORD) ConvertBase64Character(*pszInputBuf++) << 12;
        *pszOutputBuf++ = (UCHAR) (dwPackedGroup >> 16);
        if (*pszInputBuf != ASCII_Equal)
        {
            dwPackedGroup |= (DWORD) ConvertBase64Character(*pszInputBuf++) << 6;
            *pszOutputBuf++ = (UCHAR) (dwPackedGroup >> 8);
            if (*pszInputBuf != ASCII_Equal)
            {
                dwPackedGroup |= (DWORD) ConvertBase64Character(*pszInputBuf++);
                *pszOutputBuf++ = (UCHAR) (dwPackedGroup);
            }
        }
        uiInputLen -= 4;
    }
    *pszOutputBuf = '\0';

    return (DWORD) (pszOutputBuf - pszOriginalOutput);
}

static UCHAR ConvertBase64Character(UCHAR ucInputCharacter)
{
    UCHAR ucResult;

    ucResult = 0;
    if (ucInputCharacter >= ASCII_A && ucInputCharacter <= ASCII_Z )
    {
        ucResult = ucInputCharacter - ASCII_A;
    }
    else if (ucInputCharacter >= ASCII_a && ucInputCharacter <= ASCII_z)
    {
        ucResult = ucInputCharacter - ASCII_a + 26;
    }
    else if (ucInputCharacter >= ASCII_0 && ucInputCharacter <= ASCII_9)
    {
        ucResult = ucInputCharacter - ASCII_0 + 52;
    }
    else if (ucInputCharacter == ASCII_Plus)
    {
        ucResult = 62;
    }
    else if (ucInputCharacter == ASCII_Slash)
    {
        ucResult = 63;
    }
    
    return ucResult;
}



