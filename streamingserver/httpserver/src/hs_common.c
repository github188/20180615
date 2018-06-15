/*
 *  File:       AsCommon.c
 *
 */
#include "httpserver_local.h"

#if 0
static Unsigned32 gPowersOf10[] = {
    1000000000,
    100000000,
    10000000,
    1000000,
    100000,
    10000,
    1000,
    100,
    10,
    1,
    0
};


static Unsigned64 g64BitPowersOf10[] = {
    10000000000000000000,
    1000000000000000000,
    100000000000000000,
    10000000000000000,
    1000000000000000,
    100000000000000,
    10000000000000,
    1000000000000,
    100000000000,
    10000000000,
    1000000000,
    100000000,
    10000000,
    1000000,
    100000,
    10000,
    1000,
    100,
    10,
    1,
    0
};

static Unsigned32 gPowersOf16[] = {
    268435456,
    16777216,
    1048576,
    65536,
    4096,
    256,
    16,
    1,
    0
};
#endif

//#if RomPagerHttpOneDotOne
#if 0

void RpSetupChunkedBuffer(rpHttpRequestPtr theRequestPtr)
{
    if (theRequestPtr->fResponseIsChunked) {
        /*
            If the output is chunked, the buffer needs to have room
            for the leading chunk size "HHH<CR><LF>", the trailing "<CR><LF>"
            and a possible last chunk marker of "0<CR><LF><CR><LF>".
        */
        theRequestPtr->fHtmlFillPtr += 3;
        *(theRequestPtr->fHtmlFillPtr++) = '\x0d';
        *(theRequestPtr->fHtmlFillPtr++) = '\x0a';
        theRequestPtr->fFillBufferAvailable -= kChunkedOverhead;
    }
    return;
}

void RpCloseChunkedBuffer(rpHttpRequestPtr theRequestPtr)
{
    Unsigned32      theChunkSize;
    char            *theBufferPtr;

    if (theRequestPtr->fResponseIsChunked) {
        theChunkSize = theRequestPtr->fHtmlResponseLength - kChunkedOverhead;

        /*
            Mark the end of the buffer.
        */
        theBufferPtr = theRequestPtr->fHtmlResponsePtr +
                theChunkSize + kChunkedOffset;
        if (theRequestPtr->fHttpTransactionState == eRpHttpResponseComplete) {
            /*
                We have the last buffer, so put out a 0 length chunk at
                the end of the buffer.
            */
            RP_STRCPY(theBufferPtr, kLastChunk);
        }
        else {
            /*
                This is an intermediate buffer, so put out the "<CR><LF>"
                to terminate this chunk and adjust the response length
                by the size of a last chunk.
            */
            RP_STRCPY(theBufferPtr, kCRLF);
            theRequestPtr->fHtmlResponseLength -= kChunkedOffset;
        }
        /*
            Turn the chunk size into a hex string.
        */
        (void) RpConvertUnsigned32ToHex(theChunkSize,
                theRequestPtr->fHtmlResponsePtr);
    }
    return;
}

#endif  /* RomPagerHttpOneDotOne */


#if 0

Unsigned16 RpConvertUnsigned32ToHex(Unsigned32 theData,
                                            char *theBufferPtr) {
    char *          theCurrentPtr;
    Unsigned8       theDigitCount;
    Unsigned32 *    thePowersOf16Ptr;

    theCurrentPtr = theBufferPtr;
    if (theData == 0) {
        *theCurrentPtr++ = kAscii_0;
    }
    else {
        /*
            The latest specification for HTTP/1.1 says leading zeroes
            are ok for chunked encoding and will eliminate buffer offset
            games, so just start at 256 and accept a leading zero
            if it happens.
        */
        thePowersOf16Ptr = gPowersOf16 + 5;
        /* pump out the digits */
        while (theData > 0) {
            theDigitCount = 0;
            while (theData >= *thePowersOf16Ptr) {
                theData -= *thePowersOf16Ptr;
                theDigitCount += 1;
            }
            *theCurrentPtr++ = NIBBLE_TO_HEX(theDigitCount & 0x0f);
            thePowersOf16Ptr += 1;
        }
        /* pump out any trailing zeros */
        while (*thePowersOf16Ptr > 0) {
            *theCurrentPtr++ = kAscii_0;
            thePowersOf16Ptr += 1;
        }
    }
    return theCurrentPtr - theBufferPtr;
}



void RpCatUnsigned32ToString(Unsigned32 theNumber, char *theStringPtr,
        Unsigned8 theMinimumDigits) {
    Unsigned16      theCharsWritten;
    int             theDigitCount;
    char            *theNumberStringPtr;
    Unsigned32Ptr   thePowerOf10Ptr;

    theNumberStringPtr = theStringPtr + RP_STRLEN(theStringPtr);
    theDigitCount = 1;
    thePowerOf10Ptr =
        gPowersOf10 + (sizeof(gPowersOf10) / sizeof(Unsigned32) - 2);
    while (thePowerOf10Ptr >= gPowersOf10  &&
            theNumber >= *--thePowerOf10Ptr) {
        theDigitCount += 1;
    }
    while (theMinimumDigits > theDigitCount) {
        *theNumberStringPtr++ = kAscii_0;
        theDigitCount += 1;
    }
    *theNumberStringPtr = '\0';
    theCharsWritten = RpConvertUnsigned32ToAscii(theNumber, theNumberStringPtr);
    *(theNumberStringPtr + theCharsWritten) = '\0';
    return;
}


Unsigned16 RpConvertUnsigned32ToAscii(Unsigned32 theData,
                                        char *theBufferPtr) {
    char *          theCurrentPtr;
    Unsigned8       theDigitCount;
    Unsigned32 *    thePowersOf10Ptr;

    theCurrentPtr = theBufferPtr;
    if (theData == 0) {
        *theCurrentPtr++ = kAscii_0;
    }
    else {
        thePowersOf10Ptr = gPowersOf10;
        /* cycle to a useful power of 10 */
        while (*thePowersOf10Ptr > theData) {
            thePowersOf10Ptr += 1;
        }
        /* pump out the digits */
        while (theData > 0) {
            theDigitCount = 0;
            while (theData >= *thePowersOf10Ptr) {
                theData -= *thePowersOf10Ptr;
                theDigitCount += 1;
            }
            *theCurrentPtr++ = kAscii_0 + theDigitCount;
            thePowersOf10Ptr += 1;
        }
        /* pump out any trailing zeros */
        while (*thePowersOf10Ptr > 0) {
            *theCurrentPtr++ = kAscii_0;
            thePowersOf10Ptr += 1;
        }
    }
    return theCurrentPtr - theBufferPtr;
}


void RpCatSigned32ToString(Signed32 theNumber, char *theStringPtr) {
    Unsigned16      theCharsWritten;
    char *          theNumberStringPtr;

    theNumberStringPtr = theStringPtr + RP_STRLEN(theStringPtr);
    theCharsWritten = RpConvertSigned32ToAscii(theNumber, theNumberStringPtr);
    *(theNumberStringPtr + theCharsWritten) = '\0';
    return;
}


Unsigned16 RpConvertSigned32ToAscii(Signed32 theData, char *theBufferPtr) {
    Unsigned16      theCharacterCount;
    Unsigned16      theSignCount;

    if (theData < 0) {
        *theBufferPtr++ = '-';
        theData = -theData;
        theSignCount = 1;
    }
    else {
        theSignCount = 0;
    }
    theCharacterCount = RpConvertUnsigned32ToAscii((Unsigned32) theData,
            theBufferPtr);
    return theCharacterCount + theSignCount;
}

#endif  /* RomPagerServer || RomWebClient || RomPagerQueryIndex || RomXml */



BYTE HexToNibble(BOOL *bError, char cHex)
{
    BYTE byNibble = 0;

    *bError = FALSE;
    if (cHex >= ASCII_0 && cHex <= ASCII_9)
    {
        byNibble = cHex - ASCII_0;
    }
    else if (cHex >= ASCII_A && cHex <= ASCII_F)
    {
        byNibble = cHex - ASCII_A + 10;
    }
    else if (cHex >= ASCII_a && cHex <= ASCII_f)
    {
        byNibble = cHex - ASCII_a + 10;
    }
    else
    {
        *bError = TRUE;
    }

    return byNibble;
}


#if 0

void RpHexToString(unsigned char * theHexDataPtr, char * theStringPtr,
                Unsigned16 theLength) {

    while (theLength) {
        *theStringPtr++ = NIBBLE_TO_HEX(*theHexDataPtr >> 4);
        *theStringPtr++ = NIBBLE_TO_HEX(*theHexDataPtr & 0x0f);
        theHexDataPtr++;
        theLength--;
    }

    *theStringPtr = '\0';

    return;
}

#endif



void StrLenCpy(char *pszTo, char *pszFrom, DWORD dwLength)
{
    *pszTo = '\0';
    
    if (strlen(pszFrom) < dwLength)
    {
        strcpy(pszTo, pszFrom);
    }
    
    return;
}

void StrLenCpyTruncate(char *pszTo, char *pszFrom, DWORD dwLength)
{
    DWORD  dwStringLength;

    // Leave room for the Null terminator.
    dwLength -= 1;

    // Determine how much to copy.  Either the whole string or
    // as much of it as will fit into the destination buffer.
    dwStringLength = strlen(pszFrom);

    if (dwStringLength < dwLength)
    {
        dwLength = dwStringLength;
    }

    // Copy the string, then terminate it.
    memcpy(pszTo, pszFrom, dwLength);
    *(pszTo + dwLength) = '\0';
    
    return;
}

void EscapeDecodeString(char* pszEncodedString, DWORD dwEncodeLen,
            char* pszDecodedString, DWORD* pdwDecodeLen, BOOL bFormData)
{
    char            cCurrentCharacter;
    BOOL            bErrorFlag;
    THexEscapeState tHexEscapeState;
    DWORD           dwLength;

    tHexEscapeState = ESCAPE_LOOK_FOR_PERCENT;
    cCurrentCharacter = *pszEncodedString++;
    dwLength = 0;
    
    while (cCurrentCharacter != '\0' && dwEncodeLen > 0)
    {
        switch(tHexEscapeState)
        {
            case ESCAPE_LOOK_FOR_PERCENT:
                if (cCurrentCharacter == ASCII_Percent)
                {
                    tHexEscapeState = ESCAPE_GET_FIRST_HEX_BYTE;
                }
                else if (cCurrentCharacter == ASCII_Plus && bFormData)
                {
                    *pszDecodedString++ = ' ';
                    dwLength++;
                }
                else
                {
                    *pszDecodedString++ = cCurrentCharacter;
                    dwLength++;
                }
                break;

            case ESCAPE_GET_FIRST_HEX_BYTE:
                *pszDecodedString = HexToNibble(&bErrorFlag, cCurrentCharacter) << 4;
                tHexEscapeState = ESCAPE_GET_SECOND_HEX_BYTE;
                break;

            case ESCAPE_GET_SECOND_HEX_BYTE:
                *pszDecodedString++ |= HexToNibble(&bErrorFlag, cCurrentCharacter);
                tHexEscapeState = ESCAPE_LOOK_FOR_PERCENT;
                dwLength++;
                break;
        }
        cCurrentCharacter = *pszEncodedString++;
        dwEncodeLen--;
    }
    if (cCurrentCharacter == '\0')
        *pszDecodedString = '\0';
    *pdwDecodeLen = dwLength;    
    
    return;
}
