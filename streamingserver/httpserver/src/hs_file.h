/*
 *  File:       Sfil.h
 *
 *  Contains:   Prototypes for simple interface routines to file system
 */


#ifndef _HS_FILE_
#define _HS_FILE_


#define S_END_OF_FILE       0x00010001


typedef struct {
    FILE *      pFile;
} TFileData, *PTFileData;


extern SCODE HsFile_Initial(ULONG ulMaxFilesCount);
extern SCODE HsFile_Release(void);
extern SCODE HsFile_Open(UINT uiFileIndex, char *pszFileName);
extern SCODE HsFile_Close(UINT uiFileIndex);
extern SCODE HsFile_Read(UINT uiFileIndex, char *pszReadBuf, DWORD dwByteCount, 
                         DWORD* pdwReaded);
extern SCODE HsFile_SetPosition(UINT uiFileIndex, DWORD dwBytePosition);



#endif

