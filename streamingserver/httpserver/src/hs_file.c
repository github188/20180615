/*
 *  File:       Sfil.c
 *
 *  Contains:   Simple file system interface routines for standard C library
 *
 *  $History: hs_file.c $
 * 
 * *****************  Version 3  *****************
 * User: Shengfu      Date: 06/01/23   Time: 4:13p
 * Updated in $/RD_1/Project/PNX1300_PSOS/Farseer/network/netap/httpserver/src
 * 
 * *****************  Version 3  *****************
 * User: Joe          Date: 03/08/12   Time: 11:52a
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * 1. Porting to Trimedia MDS card.
 * 
 * *****************  Version 2  *****************
 * User: Joe          Date: 03/07/30   Time: 11:28a
 * Updated in $/rd_1/Project/TM1300_PSOS/FarSeer/network/netap/httpserver/src
 * Fix resource leak problem.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "osisolate.h"
#include "hs_file.h"


static ULONG        gFileCount;
static PTFileData   gpFileData;



/*
    The SfsOpenFileSystem routine is called once when RomPager starts up.  The
    maximum number of simultaneous open files is passed in to the file system
    so that the file system interface can dynamically initialize it's internal
    variables and processes.  The number passed in will usually be the same as
    the number of simultaneous HTTP requests that the RomPager engine supports.

    Inputs:
        theOpenFilesCount:          - number of allowed open files

    Returns:
        theResult:
            eRpNoError              - no error
            eRpFileSystemNotOpen    - can't initialize the file system
*/

SCODE HsFile_Initial(ULONG ulMaxFilesCount)
{
    SCODE sResult;

    sResult = S_OK;
    
    gpFileData = (PTFileData) malloc(ulMaxFilesCount * sizeof(TFileData));
    if (gpFileData == (PTFileData) 0)
    {
        sResult = S_FAIL;
    }
    else
    {
        memset(gpFileData, 0, ulMaxFilesCount * sizeof(TFileData));
        gFileCount = ulMaxFilesCount;
    }
    return sResult;
}


/*
    The SfsCloseFileSystem routine is called once at when RomPager finishes so
    that the file system can deinitialize any internal variables and processes.

    Returns:
        theResult:
            eRpNoError              - no error
            eRpFileSystemNotClosed  - can't deinitialize file system
*/

SCODE HsFile_Release(void)
{
    PTFileData ptFileData;
    UINT       uiFileIndex;
    SCODE      sResult;

    sResult = S_OK;
    if (gpFileData == (PTFileData) 0)
    {
        sResult = S_FAIL;
    }
    else
    {
        for (uiFileIndex = 0, ptFileData = gpFileData;
             uiFileIndex < gFileCount; ptFileData += 1, uiFileIndex += 1)
        {
            if (ptFileData->pFile != (FILE *) 0)
            {
                HsFile_Close(uiFileIndex);
            }
        }
        free(gpFileData);
    }
    return sResult;
}


/*
    The SfsOpenFile routine is called to open an individual file.  The file byte
    position is set to 1.  The open file call is responsible for all directory
    positioning, since the full file name from the URL will be passed in.  An
    example file name is: /MyFirstDirectory/TheSecondDirectory/MyFile.

    Inputs:
        theFileNumber:          - file id
        theFullNamePtr:         - pointer to full URL object name

    Returns:
        theResult:
            eRpNoError          - no error
            eRpFileNotFound     - can't find this file to open
            eRpFileOpenError    - can't open file
*/

SCODE HsFile_Open(UINT uiFileIndex, char *pszFileName)
{
    PTFileData  ptFileData;
    SCODE       sResult;

    sResult = S_OK;
    if (gpFileData == (PTFileData) 0)
    {
        sResult = S_FAIL;
    }
    else
    {
        ptFileData = gpFileData + uiFileIndex;
        ptFileData->pFile = fopen (pszFileName, "rb");
        if (ptFileData->pFile == (FILE *) 0)
        {
            sResult = S_FAIL;
        }
    }
    return sResult;
}

/*
    The SfsCloseFile routine is used to signal the end of usage for a particular
    file.  The file system should close the file and initialize any internal
    variables pointed to by the file number.  This call is asynchronous and
    completes when the close has been started.  The SfsCloseStatus call is
    used to detect the completion of the close.  theCompleteFlag is used to
    signal whether all data was received from the application.

    Inputs:
        theFileNumber:          - file id
        theCompleteFlag
            True (1)            - file is complete
            False (0)           - file is incomplete

    Returns:
        theResult:
            eRpNoError          - no error
            eRpFileNotOpen      - this file number not open
            eRpFileCloseError   - can't close file
*/

SCODE HsFile_Close(UINT uiFileIndex)
{
    PTFileData  ptFileData;
    SCODE       sResult;

    sResult = S_OK;
    if (gpFileData == (PTFileData) 0)
    {
        sResult = S_FAIL;
    }
    else
    {
        ptFileData = gpFileData + uiFileIndex;
        if (ptFileData->pFile == (FILE *) 0)
        {
            sResult = S_FAIL;
        }
        else
        {
            if (fclose(ptFileData->pFile) == EOF)
            {
                sResult = S_FAIL;
            }
            // Add by Joe, 2003/07/30
            ptFileData->pFile = NULL;
        }
    }
    return sResult;
}


/*
    The SfsReadFile routine is called to start a read into the buffer provided
    for the number of bytes in the count.  The read takes place at the current
    file byte position with the file byte position being updated after the read
    completes.

    Inputs:
        theFileNumber:          - file id
        theReadPtr:             - pointer to the read buffer
        theByteCount:           - number of bytes to read

    Returns:
        theResult:
            eRpNoError              - no error
            eRpFileNotOpen          - this file number not open
            eRpFileInvalidPosition  - can't read at this position
                                            (possibly after EOF)
            eRpFileReadError            - can't read file
*/

SCODE HsFile_Read(UINT uiFileIndex, char *pszReadBuf, DWORD dwByteCount, 
                  DWORD* pdwReaded)
{
    PTFileData  ptFileData;
    SCODE       sResult;

    sResult = S_OK;
    if (gpFileData == (PTFileData) 0)
    {
        sResult = S_FAIL;
    }
    else
    {
        ptFileData = gpFileData + uiFileIndex;
        if (ptFileData->pFile == (FILE *) 0)
        {
            sResult = S_FAIL;
        }
        else
        {
            *pdwReaded = (DWORD) fread(pszReadBuf, 1, dwByteCount, ptFileData->pFile);
                
            if (*pdwReaded == 0)
            {
                if (feof(ptFileData->pFile))
                {
                    sResult = S_END_OF_FILE;
                }
                else
                {
                    sResult = S_FAIL;
                }
            }
        }
    }
    return sResult;
}

/*
    The SfsSetFilePosition routine is called to set a new current file byte
    position for subsequent reads or writes.

    Inputs:
        theFileNumber:          - file id
        theBytePosition:        - the new file byte position

    Returns:
        theResult:
            eRpNoError              - no error
            eRpFileNotOpen          - this file number not open
            eRpFileInvalidPosition  - can't set to this position
*/

SCODE HsFile_SetPosition(UINT uiFileIndex, DWORD dwBytePosition)
{
    PTFileData  ptFileData = NULL;
    SCODE       sResult;

    sResult = S_OK;
    if (gpFileData == (PTFileData) 0)
    {
        sResult = S_FAIL;
    }
    else
    {
        ptFileData = gpFileData + uiFileIndex;
        if (ptFileData->pFile == (FILE *) 0)
        {
            sResult = S_FAIL;
        }
    }
    if (sResult == S_OK)
    {
        if (fseek(ptFileData->pFile, dwBytePosition, SEEK_SET) != 0)
        {
            sResult = S_FAIL;
        }
    }
    return sResult;
}

