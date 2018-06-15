// hs_test.cpp : Defines the entry point for the console application.
//

#include <stdarg.h>
#include "osisolate.h"
#include "common.h"
#include "errordef.h"
#include "typedef.h"
#include "httpserver.h"
#include "rtspstreamingserver.h"
#include "sockdef.h"
#include "bitstreambufdef.h"
//#include "encrypt_md5.h"
//#include "accountmgr.h"

#ifdef _SIP
#include "sipua.h"	
#endif

#ifdef _DEBUG
#include <vld.h>
#endif // _DEBUG

#define WEBPAGE_PATH	"K:/tmp/Web"
#define SSMAXCONNECTION	10

FILE* pVideoFile[MULTIPLE_STREAM_NUM];
FILE* pAudioFile;
FILE* pOutFile;
HANDLE ghRTSPStreamingServer;
BYTE gVideoData[MULTIPLE_STREAM_NUM][200000];

DWORD gdwVideoSize;
BYTE gAudioData[10000];
DWORD gdwAudioSize;
//DWORD  gSendClientId;
TBitstreamBuffer gBitStream;
BYTE gUpStreamBuffer[1460];
DWORD g_dwVideoLastTick;
DWORD g_dwAudioLastTick;
TBitstreamBuffer g_VideoBitStream;
TBitstreamBuffer g_AudioBitStream;

	char g_acMPEG4Header[28] = {
 0x00,0x00,0x01,0xB0,0x03,0x00,0x00,0x01,0xB5,0x09,
 0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x20,0x00,0x84,
 0x5D,0x4C,0x28,0x58,0x20,0xF0,0xA3,0x1F};

	char g_acD1MPEG4Header[29] ={
 0x00,0x00,0x01,0xB0,0x08,0x00,0x00,0x01,0xB5,0x09,
 0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x20,0x00,0xC4,
 0x88,0x81,0xF4,0x51,0x40,0x43,0xC1,0x46,0x3F};

	char g_ac176x144MPEG4Header[29] ={
 0x00,0x00,0x01,0xB0,0x08,0x00,0x00,0x01,0xB5,0x09,
 0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x20,0x00,0xC4,
 0x88,0x81,0xF4,0x50,0x58,0x41,0x21,0x44,0x3F};

#define UserName			""
#define	Password			""
#define HTTP_HOST_NAME		"HTTP_Server"
#define WEBPAGE_PATH		"E:/WWW"

char	g_acVideoFileName[3][50] = {	"176x144.hgd",
										"d1.hgd",
										"motion.hgd" };

char	g_acAccessName[3][20] = {"live.sdp","livel.sdp","livem.sdp"};
char	g_acVideoTrackName[3][20] = {"trackID=1","trackID=2","trackID=3"};

#define	g_acAudioTrackName  "trackID=4"

int		g_iStreamMediaType[3] = { RTSPSTREAMING_MEDIATYPE_AUDIOVIDEO,
								RTSPSTREAMING_MEDIATYPE_AUDIOVIDEO,
								RTSPSTREAMING_MEDIATYPE_AUDIOVIDEO,};

FILE* pUpFile;
HANDLE gServerObj;
DWORD  gSendClientId;

static char TestMemoryHtml[] =
    "<html>\n"
    "<head>\n"
    "<title>Restore Factory Settings</title>\n"
    "<script language=\"JavaScript\">\n"
    "<!--\n"
    "function close_window() {\n"
    "    window.close();\n"
    "}\n"
    "function restore()\n"
    "{\n"
    "    document.Factory.submit();\n"
    "    window.close();\n"
    "}\n"
    "//-->\n"
    "</script>\n"
    "</head>\n"
    "<body bgcolor=\"#000000\" text=\"#FFFFFF\">\n"
    "<b><font size=\"4\" face=\"Arial, Helvetica, sans-serif\">Restore Factory Settings</font></b>\n"
    "<table width=\"340\" border=\"0\" height=\"149\">\n"
    "<tr>\n"
    "  <td height=\"164\"> \n"
    "  <form name=\"Factory\" method=\"post\" action=\"/setup/factory.cgi\" onSubmit=\"return restore();\">\n"
    "  <table width=\"332\" border=\"0\">\n"
    "    <tr>\n"
    "      <td width=\"332\">Restore factory settings and lose any changes?\n"
    "       System will restart and need installer program to setup network.</td>\n"
    "    </tr>\n"
    "    <tr>\n"
    "      <td>&nbsp;</td>\n"
    "    </tr>\n"
    "    <tr> \n"
    "      <td height=\"22\"> \n"
	"        <input type=\"hidden\" name=\"restart\" value=\"go\">\n"
    "        <input type=\"submit\" name=\"OK\" value=\"OK\">\n"
    "        <input type=\"button\" name=\"Cancel\" value=\"Cancel\" onClick=\"close_window()\">\n"
    "      </td>\n"
    "    </tr>\n"
    "  </table>\n"
    "  </form>\n"
    "  </td>\n"
    "</tr>\n"
    "</table>\n"
    "</body>\n"
    "</html>\n";

#ifdef _WIN32_
int HttpServerCallback(DWORD dwInstance, DWORD dwCallbackType, void* pvCallbackData)
{
	THTTPServer_AcceptData*				ptCallbackAccept;
	THTTPServer_RequestData*			ptCallbackRequest;
	THTTPServer_Send*					ptCallbackSend;
	HANDLE								hThread;
	DWORD								threadid,dwPrivilege,scResult;
	SOCKET								sckControl;
    TStreamServer_ConnectionSettings	tSSConnSettings;
	THTTPServer_Author_Info				*ptAuthorInfo;

	switch(dwCallbackType)
	{
	case HTTPServer_Callback_Accept:

		ptCallbackAccept = (THTTPServer_AcceptData*) pvCallbackData;
		printf("APP Callback on Accept : [%02d] IP = 0x%08X, Port = %d\n", ptCallbackAccept->dwClientID,
			ptCallbackAccept->dwClientIPAddress, ptCallbackAccept->usClientPort);
		break;

	case HTTPServer_Callback_Authorize:

		ptCallbackRequest = (THTTPServer_RequestData*) pvCallbackData;
		printf("APP Callback on Authorize [%02d] : URL = %s\n", ptCallbackRequest->dwClientID, ptCallbackRequest->pszURL);

		ptCallbackRequest->tResponseStatus = HTTP_URL_OK;

		// Test url not found
		if (strcmp(ptCallbackRequest->pszURL, "/nourl.htm") == 0)
		{
			ptCallbackRequest->tResponseStatus = HTTP_URL_NOT_FOUND;
		}
		// Test authorize fail
		if (strcmp(ptCallbackRequest->pszURL, "/authfail.htm") == 0)
		{
			ptCallbackRequest->tResponseStatus = HTTP_URL_UNAUTHORIZED;
		}

		//ShengFu 2006/01/16 authorization
		if (strcmp(ptCallbackRequest->pszURL, "/protect.htm") == 0)
		{			
			ptCallbackRequest->tResponseStatus = HTTP_URL_UNAUTHORIZED;

			if( ptCallbackRequest->iAuthenticateType == HTTPServer_AuthorizationType_Digest )
			{
				if( (strcmp(ptCallbackRequest->pszUserName,UserName)== 0 ) &&
					(strcmp(ptCallbackRequest->pszPassword,"DIGESTOK") == 0 ))
				{
					ptCallbackRequest->tResponseStatus = HTTP_URL_OK;
				}
			}
			else if(ptCallbackRequest->iAuthenticateType == HTTPServer_AuthorizationType_Basic )
			{
				if( (strcmp(ptCallbackRequest->pszUserName,UserName)== 0 ) &&
					(strcmp(ptCallbackRequest->pszPassword,Password) == 0 ) )
				{
					ptCallbackRequest->tResponseStatus = HTTP_URL_OK;	
				}
			}
		}

		if(strncmp(ptCallbackRequest->pszURL, "/live.sdp", 10) == 0)
		{
/*****************************************************************************************/
			ptCallbackRequest->tResponseStatus = HTTP_URL_UNAUTHORIZED;

			if( UserName[0] == 0 && Password[0] == 0 )
				ptCallbackRequest->tResponseStatus = HTTP_URL_OK;

			if( ptCallbackRequest->iAuthenticateType == HTTPServer_AuthorizationType_Digest )
			{
				if( (strcmp(ptCallbackRequest->pszUserName,UserName)== 0 ) &&
					(strcmp(ptCallbackRequest->pszPassword,"DIGESTOK") == 0 ))
				{
					ptCallbackRequest->tResponseStatus = HTTP_URL_OK;
				}
			}
			else if(ptCallbackRequest->iAuthenticateType == HTTPServer_AuthorizationType_Basic )
			{
				if( (strcmp(ptCallbackRequest->pszUserName,UserName)== 0 ) &&
					(strcmp(ptCallbackRequest->pszPassword,Password) == 0 ) )
				{
					ptCallbackRequest->tResponseStatus = HTTP_URL_OK;	
				}
			}

/****************************************************************************************/

			HTTPServer_TakeClientOut(gServerObj, ptCallbackRequest->dwClientID, &sckControl);
			dwPrivilege = 143;
			tSSConnSettings.sckControl = sckControl;
			tSSConnSettings.dwPrivilege = dwPrivilege;
			tSSConnSettings.dwConnectionID = 0;
			tSSConnSettings.pszSessionCookie = ptCallbackRequest->pszSessionCookie;
			tSSConnSettings.dwRecvLength =0;

			if(ptCallbackRequest->tHttpMethod == HTTP_GET_COMMAND)
			{
				tSSConnSettings.iHTTPMethod = SS_HTTPMOTHOD_GET;
				printf("RTSP over HTTP SEND socket %d\r\n",sckControl);
			}
			else if(ptCallbackRequest->tHttpMethod == HTTP_POST_COMMAND)
			{
				tSSConnSettings.iHTTPMethod = SS_HTTPMOTHOD_POST;
				//printf("RTSP over HTTP RECV socket %d\r\n",sckControl);
				ptCallbackRequest->tResponseStatus = HTTP_URL_NOT_RESPONSE;
			}

			scResult = RTSPStreaming_AddRTPOverHTTPSock(ghRTSPStreamingServer,&tSSConnSettings);

//			scResult = StreamServer_AddConnection(pThis->hStreamServer, &tSSConnSettings);
			if(scResult != CONTROLCHANNEL_S_WAIT2NDSOCKET)
			{
				TelnetShell_DbgPrint("StreamServer_AddConnection, result = %08X, ID = %u, P = %d\r\n", scResult, tSSConnSettings.dwConnectionID, dwPrivilege);
			}
			if ((scResult != S_OK) && (scResult != CONTROLCHANNEL_S_WAIT2NDSOCKET))
			{
				ptCallbackRequest->tResponseStatus = HTTP_URL_UNAVAILABLE;
				break;
			}

			ptCallbackRequest->dwStreamID = tSSConnSettings.dwConnectionID;
			ptCallbackRequest->dwPrivilege = dwPrivilege;
			ptCallbackRequest->tDataType = DATA_TYPE_TUNNELLED;

		}

		break;

	case HTTPServer_Callback_Request:

		ptCallbackRequest = (THTTPServer_RequestData*) pvCallbackData;
		printf("APP Callback on Request [%02d] : Data Length = %d\n", ptCallbackRequest->dwClientID, ptCallbackRequest->dwObjectLength);

		if (strcmp(ptCallbackRequest->pszURL, "/memdata.htm") == 0) // Test memory data source
		{
			ptCallbackRequest->tDataSource = DATA_SOURCE_MEMORY;
			ptCallbackRequest->pSendBuffer = TestMemoryHtml;
			ptCallbackRequest->dwSendBufferLength = (DWORD) strlen(TestMemoryHtml);
		}
		/*
		//else if (strcmp(ptCallbackRequest->pszURL, "/doc/p4b.pdf") == 0) // Test send data by application
		{
			gSendClientId = ptCallbackRequest->dwClientID;
			ptCallbackRequest->tDataSource = DATA_SOURCE_PENDING;
			hThread = CreateThread(NULL,
			 					   0,
								   (LPTHREAD_START_ROUTINE)SendThreadProc,
								   (LPVOID)0,
								   0,
								   &threadid);
		}*/
		else if (strcmp(ptCallbackRequest->pszURL, "/security.cgi") == 0) // Test post
		{
			printf("APP Callback on Request [%02d] : Data = %s\n", ptCallbackRequest->dwClientID, ptCallbackRequest->pszObjectBuffer);
			ptCallbackRequest->tDataSource = DATA_SOURCE_FILE;
			strcpy(ptCallbackRequest->szFileName, WEBPAGE_PATH"/security.html");
		}
        else if(strcmp(ptCallbackRequest->pszURL, "/") == 0)    // it is root directory
        {
			ptCallbackRequest->tDataSource = DATA_SOURCE_FILE;
	        ptCallbackRequest->dwLastModified = 0;
			ptCallbackRequest->tResponseStatus = HTTP_URL_OK;
            strcpy(ptCallbackRequest->szFileName, WEBPAGE_PATH"/index.htm");
        }
		else // Test file data source
		{
			ptCallbackRequest->tDataSource = DATA_SOURCE_FILE;
	        ptCallbackRequest->dwLastModified = 0;
			ptCallbackRequest->tResponseStatus = HTTP_URL_OK;
            sprintf(ptCallbackRequest->szFileName, WEBPAGE_PATH"%s", ptCallbackRequest->pszURL);
			//strcpy(ptCallbackRequest->szFileName, ptCallbackRequest->pszURL);
		}
		break;

	case HTTPServer_Callback_Multipart_Head:

		ptCallbackRequest = (THTTPServer_RequestData*) pvCallbackData;
		printf("APP Callback on Multipart Head [%02d] : Name = %s, File name = %s\n", ptCallbackRequest->dwClientID, ptCallbackRequest->pszContentName, ptCallbackRequest->pszUpFileName);

		// Test file upload
		if (strcmp(ptCallbackRequest->pszURL, "/upload.cgi") == 0 && strcmp(ptCallbackRequest->pszContentName, "file") == 0)
		{
			if (strcmp(ptCallbackRequest->pszUpFileName, "") != 0)
				pUpFile = fopen(ptCallbackRequest->pszUpFileName, "wb");
		}

		break;

	case HTTPServer_Callback_Multipart_Data:

		ptCallbackRequest = (THTTPServer_RequestData*) pvCallbackData;
		printf("APP Callback on Multipart Data [%02d] : Len = %d\n", ptCallbackRequest->dwClientID, ptCallbackRequest->dwObjectLength);

		if (strcmp(ptCallbackRequest->pszURL, "/upload.cgi") == 0 && strcmp(ptCallbackRequest->pszContentName, "file") == 0) // Test file upload
		{
			if (ptCallbackRequest->dwObjectLength != 0)
				fwrite(ptCallbackRequest->pszObjectBuffer, 1, ptCallbackRequest->dwObjectLength, pUpFile);
			if (ptCallbackRequest->bObjectComplete)
			{
				fclose(pUpFile);
				ptCallbackRequest->tDataSource = DATA_SOURCE_FILE;
				strcpy(ptCallbackRequest->szFileName, WEBPAGE_PATH"/upgrade.html");
			}
		}
		break;

	case HTTPServer_Callback_Multipart_Request:

		ptCallbackRequest = (THTTPServer_RequestData*) pvCallbackData;
		if (strcmp(ptCallbackRequest->pszURL, "/upload.cgi") == 0) // Test file upload
		{
			ptCallbackRequest->tDataSource = DATA_SOURCE_FILE;
			strcpy(ptCallbackRequest->szFileName, WEBPAGE_PATH"/upgrade.html");
		}
		break;

	case HTTPServer_Callback_Disconnect:
		break;

	case HTTPServer_Callback_Send:

		ptCallbackSend = (THTTPServer_Send*) pvCallbackData;
		printf("APP Callback on Send [%02d]\n", ptCallbackSend->dwClientID);
		break;

	case HTTPServer_Callback_Digest_Auth_Request:

		ptAuthorInfo =(THTTPServer_Author_Info*)pvCallbackData;
		
		strcpy(ptAuthorInfo->pszUserName,UserName);
		strcpy(ptAuthorInfo->pszPassword,Password);

		break;

	default:
		break;

	}

	return 1;
}
#endif
int  TelnetShell_DbgPrint(const char *pszFormat, ...)
{
	va_list arg_pt;

	va_start(arg_pt, pszFormat);
	vprintf(pszFormat, arg_pt); 
	va_end(arg_pt);

	return 0;
}

SCODE RTSPStreamServerVideoCallback(DWORD dwInstance, DWORD dwCallbackType, void* pvCallbackData)
{
	//TBitstreamBuffer tBitStream;
	DWORD dwRead;
	DWORD dwCurrentTick;
	DWORD dwFrameType;
	int i;
	static int k=0, iIndex=0;
	
	//ptBitStream = (TBitstreamBuffer *) pvCallbackData;


	switch(dwCallbackType)
	{
	case MEDIA_CALLBACK_CHECK_CODEC_INDEX:

		/*for( i=0 ; i< MULTIPLE_STREAM_NUM; i++ )
		{
			if( strcmp(g_acVideoTrackName[i],pvCallbackData) == 0 )
				return i+1;
		}*/

		if( pvCallbackData <= MULTIPLE_STREAM_NUM )
			return (DWORD)pvCallbackData;

		return -1;
		break;

	case MEDIA_CALLBACK_REQUEST_BUFFER:
		
		if( MULTIPLE_STREAM_NUM > 1 )
		{
			iIndex++;
			iIndex = iIndex%MULTIPLE_STREAM_NUM ;
		}

        OSSleep_MSec(25);

/**********************************************************************
		dwRead = fread(gVideoData, 1, 4, pVideoFile);
		if (dwRead < 4)
		{
			fseek(pVideoFile, 0 ,SEEK_SET);
			dwRead = fread(gVideoData, 1, 4, pVideoFile);
		}
		gdwVideoSize = gVideoData[3]<<24 | gVideoData[2]<<16 | gVideoData[1]<<8 | gVideoData[0];
		dwRead = fread(gVideoData+4, 1, gdwVideoSize, pVideoFile);

		dwFrameType = gVideoData[8] >> 4;

		if (dwFrameType == 0)
			g_VideoBitStream.tFrameType = MEDIADB_FRAME_INTRA;
		else
			g_VideoBitStream.tFrameType = MEDIADB_FRAME_PRED;

		g_VideoBitStream.pbyBuffer = (BYTE*)gVideoData+4;
		g_VideoBitStream.dwBytesUsed = gdwVideoSize;
		OSTick_GetMSec(&dwCurrentTick);
***********************************************************************/
		
		dwRead = fread(gVideoData[iIndex], 1, 4, pVideoFile[iIndex]);
		if (dwRead < 4)
		{
			fseek(pVideoFile[iIndex], 0 ,SEEK_SET);
			dwRead = fread(gVideoData[iIndex], 1, 4, pVideoFile[iIndex]);
			k = 1;
		}
		gdwVideoSize = gVideoData[iIndex][0]<<24 | gVideoData[iIndex][1]<<16 | gVideoData[iIndex][2]<<8 | gVideoData[iIndex][3];

		dwRead = fread(gVideoData[iIndex]+4, 1, gdwVideoSize, pVideoFile[iIndex]);

		dwFrameType = gVideoData[iIndex][8] >> 4;

		if (dwFrameType == 0)
			g_VideoBitStream.tFrameType = MEDIADB_FRAME_INTRA;
		else
			g_VideoBitStream.tFrameType = MEDIADB_FRAME_PRED;

		g_VideoBitStream.dwOffset = 20 + gVideoData[iIndex][19]*4;
		g_VideoBitStream.pbUserData = (BYTE*)gVideoData[iIndex];
		g_VideoBitStream.pbyBuffer = (BYTE*)gVideoData[iIndex] + g_VideoBitStream.dwOffset;
		g_VideoBitStream.dwBytesUsed = gdwVideoSize - g_VideoBitStream.dwOffset + 4;

		g_VideoBitStream.dwStreamIndex = iIndex+1;
/*		if( k != 1 )
			fwrite(g_VideoBitStream.pbyBuffer,sizeof(char),g_VideoBitStream.dwBytesUsed,pOutFile);

		for(i=0;i<8;i++)
			printf("%02x",g_VideoBitStream.pbyBuffer[i]);

		printf("\n");*/
		OSTick_GetMSec(&dwCurrentTick);
/***************************************************************************/
		while ((dwCurrentTick - g_dwVideoLastTick) < 30)
		{
			//Sleep(1);
			OSSleep_MSec(1);
			//dwCurrentTick = GetTickCount();
			OSTick_GetMSec(&dwCurrentTick);
			//g_dwVideoLastTick = dwCurrentTick;
		}
		g_dwVideoLastTick = dwCurrentTick;
		*((TBitstreamBuffer**)pvCallbackData) = &g_VideoBitStream;

		break;

	case MEDIA_CALLBACK_RELEASE_BUFFER:

		//printf("APP Callback Video Release Buffer\n");

		break;

	default:
		break;

	}

	return 1;
}
	
SCODE RTSPStreamServerAudioCallback(DWORD dwInstance, DWORD dwCallbackType, void* pvCallbackData)
{
	//TBitstreamBuffer* ptBitStream;
	DWORD dwRead, tt;
	DWORD dwCurrentTick;
	
	//ptBitStream = (TBitstreamBuffer *) pvCallbackData;

	switch(dwCallbackType)
	{
	case MEDIA_CALLBACK_CHECK_CODEC_INDEX:
		return 1;

	case MEDIA_CALLBACK_REQUEST_BUFFER:

		dwRead = fread(gAudioData, 1,53, pAudioFile);
		if (dwRead < 53)
		{
			fseek(pAudioFile, 0 ,SEEK_SET);
			dwRead = fread(gAudioData, 1,53, pAudioFile);
		}
		g_AudioBitStream.pbyBuffer = (BYTE*)gAudioData;
		g_AudioBitStream.dwBytesUsed = dwRead;
		g_VideoBitStream.dwOffset = 0;

		g_AudioBitStream.dwStreamIndex = 1;

		OSTick_GetMSec(&dwCurrentTick);
		OSSleep_MSec(80);
		/*while ((dwCurrentTick - g_dwAudioLastTick) < 110)
		{
			OSSleep_MSec(1);
			OSTick_GetMSec(&dwCurrentTick);
		}*/
		g_dwAudioLastTick = dwCurrentTick;
		*((TBitstreamBuffer**)pvCallbackData) = &g_AudioBitStream;

/*		dwRead = fread(gAudioData, 1, 4, pAudioFile);
		if (dwRead < 4)
		{
			fseek(pAudioFile, 0 ,SEEK_SET);
			dwRead = fread(gAudioData, 1, 4, pAudioFile);
		}
		tt = (DWORD) gAudioData[3];
		gdwAudioSize = (((DWORD) gAudioData[0] & 0x000000FF)<<24) | 
			           (((DWORD) gAudioData[1] & 0x000000FF)<<16) |
					   (((DWORD) gAudioData[2] & 0x000000FF)<<8) |
					   ((DWORD)gAudioData[3] & 0x000000FF);
		dwRead = fread(gAudioData+4, 1, gdwAudioSize, pAudioFile);
		g_AudioBitStream.pbyBuffer = (BYTE*)gAudioData+23;
		g_AudioBitStream.dwBytesUsed = gdwAudioSize +4-23;

		g_AudioBitStream.dwStreamIndex = 1;

		OSTick_GetMSec(&dwCurrentTick);
		while ((dwCurrentTick - g_dwAudioLastTick) < 110)
		{
			OSSleep_MSec(1);
			OSTick_GetMSec(&dwCurrentTick);
		}
		g_dwAudioLastTick = dwCurrentTick;
		*((TBitstreamBuffer**)pvCallbackData) = &g_AudioBitStream;*/
		break;

	case MEDIA_CALLBACK_RELEASE_BUFFER:

		//printf("APP Callback Audio Release Buffer\n");
		break;

	default:
		break;

	}

	return 1;
}

SCODE RTSPStreamServerCtrlChCallback(DWORD dwInstance, DWORD dwConnectionID, DWORD dwCallbackType, DWORD dwCallbackData)
{
	CHAR	*pcLoc;
	TAuthorInfo *ptAuthInfo;

	if(dwCallbackType == ccctGetLocation)
	{
		pcLoc = (CHAR*) dwCallbackData;
		strcpy(pcLoc, "SkyLand");
		printf("[App] CID %u get location sting %s\r\n", dwConnectionID, pcLoc);
	}
	else if(dwCallbackType == ccctForceIntra)
	{
		printf("[App] CID %u force intra\r\n", dwConnectionID);
	}
	else if(dwCallbackType == ccctPacketLoss)
	{
		printf("[App] CID %u report packet loss\r\n", dwConnectionID);
	}
	else if(dwCallbackType == ccctWaitHTTPSocket)
	{
//		printf("[App] CID %u receive waiting HTTP socket callback\r\n", dwConnectionID);
//		bWaitHTTPSocket = TRUE;
	}
	else if(dwCallbackType == ccctSocketError)
	{
		printf("[App] CID %u receive Socket Error\r\n", dwConnectionID);
	}
	else if(dwCallbackType == ccctClose)
	{
		printf("[App] CID %u closed \r\n", dwConnectionID);
	}
	else if(dwCallbackType == ccctTimeout)
	{
		printf("[App] CID %u timeout\r\n", dwConnectionID);
	}
	else if( dwCallbackType == ccctAuthorization )
	{
		ptAuthInfo = (TAuthorInfo*)dwCallbackData;

		if( Password[0] == 0 )
			return 0;
		
		if( ptAuthInfo->iAuthType == RTSPSTREAMING_AUTHENTICATION_BASIC )
		{
			if( strcmp(ptAuthInfo->acUserName,UserName) == 0 )
				return 0;
			else
				return -1;
		}

		if(ptAuthInfo->iAuthType == RTSPSTREAMING_AUTHENTICATION_DIGEST )
		{
			if( ptAuthInfo->acUserName[0] == 0 )
				return -1;

			memset((void*)(ptAuthInfo->acPasswd),0,SS_AUTH_PASS_LENGTH);
			//sprintf(acTemp,"%s:RTSP server:%s",UserName,Password);
			EncryptionUtl_MD5(ptAuthInfo->acPasswd,UserName,":","streaming_server",":",Password,NULL);
			return 0;
		}
		
	}
	else
	{
		printf("[App] CID %u Callback %u\r\n", dwConnectionID, dwCallbackType);
	}
	return S_OK;
}


#ifdef _SIP
int SIPUAClient_ReadConfig(TSIP_PARAM *ptInitOptions)
{
	//[YenChun] SIP user agent server
	{
		FILE *input ;
		int filenum = 0;
		char * token;
		char * strtoken[50];
		char Doc[512] ="" ;
#ifdef _WIN32_
		input = fopen("sipconfig.ini", "rb");
		if(!input)
		{
			printf("[SIPUA] Read config file fail.\n" );
			return 0;
		}
#elif defined (_LINUX)
		input = fopen( "/mnt/flash/etc/conf.d/sipua.conf", "rb" );
#else
		input = NULL;
#endif
		if(!input)
		{
			printf("[SIPUA] Read config file fail. Use default setting.\n" );
			strtoken[0] = "5060"; //SIP Port
			strtoken[1] = "202.5.224.91"; //OUTBOUND_PROXY_IP
			strtoken[2] = "9090"; //OUTBOUND_PROXY_PORT
			strtoken[3] = "test3@vivotek.com"; //AuthorizationName
			strtoken[4] = "123456"; //AuthorizationPassword
			strtoken[5] = "test3_AT_vivotek.com"; //URIName
			strtoken[6] = "teltel.com"; //URIDomain
			strtoken[7] = "yenchun_AT_vivotek.com@teltel.com"; //RequestURI
		}
		else
		{
			*Doc = getc( input );
			//fread( Doc, sizeof(char), sizeof(Doc) -1, input);			
			while( !feof(input) && filenum < 510 )
			{
				Doc[filenum+1] = getc( input );
				filenum++;
			}
			Doc[filenum] = '\0';			
			//fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __func__);
			fclose(input);

			//printf("SIP network information:\n%s\n", Doc);
			filenum = 0 ;
#ifdef _LINUX
			token = (char*) strtok( Doc, "\n" );
#else
			token = (char*) strtok( Doc, "\r\n" );
#endif
			while( token != NULL )
			{
				char* equal = NULL; 
				/* While there are tokens in "string" */
				equal =  (char*) strstr( token, "=");
				if( !equal ) return 0;
				strtoken[filenum] = equal + 1;
				filenum++;
				/* Get next token: */
#ifdef _LINUX
				token = (char*) strtok( NULL, "\n" );
#else
				token = (char*) strtok( NULL, "\r\n" );
#endif
			}
		}

		printf("[SIPUA] Read config file success.\n" );
		printf("%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n", strtoken[0], strtoken[1], strtoken[2], strtoken[3], strtoken[4], strtoken[5], strtoken[6], strtoken[7]);

		strcpy(ptInitOptions->szUserName, strtoken[3]);
		strcpy(ptInitOptions->szPassword, strtoken[4]);
		strcpy(ptInitOptions->szSIPDomain, strtoken[6]);
		strcpy(ptInitOptions->szDisplayName, strtoken[5]);
		ptInitOptions->usPort = atoi( strtoken[0]);
		ptInitOptions->ulOBProxyIP = inet_addr(strtoken[1]);
		ptInitOptions->usOBProxyPort = atoi( strtoken[2]);
//		strncpy(g_acRequestURI, strtoken[7], sizeof(g_acRequestURI)-1);
	}
	
	ptInitOptions->iSIPMaxConnectionNum = 2;

	return S_OK;

}
#endif

int main(int argc, char* argv[])
{

	TRTSPSTREAMING_PARAM		     stRTSPStreamingParam;
	TRTSPSTREAMING_VIDENCODING_PARAM stRTSPVideoParam;
	TRTSPSTREAMING_AUDENCODING_PARAM stRTSPAudioParam;
	
	THTTPServer_InitSettings    tInitSetting;
#ifdef _WIN32_
    WSADATA wsaData;
    struct  hostent  *phost;
#endif
	char    acText[64];
	struct	sockaddr_in addr;
	BYTE	byMajor, byMinor, byBuild, byRevision;
	DWORD	dwVideoFlag=0,dwAudioFlag=0;
    ULONG   ulMyIP;
	SCODE   sResult;
	int		i;

#ifdef _WIN32_
	if (WSAStartup(0x202,&wsaData) == SOCKET_ERROR)
	{
		fprintf(stderr,"WSAStartup error %d\n",WSAGetLastError());
		return -1;
	}
	else
	{
		printf("WSAStartup successed.\n");
	}
	
    gethostname(acText, 64);
	phost = gethostbyname(acText);
	stRTSPStreamingParam.ulLocalIP=addr.sin_addr.S_un.S_addr = *(int*) phost->h_addr_list[0];

	printf("RTSP server IP=%s\n",inet_ntoa(addr.sin_addr));
#endif

    OS_Init();
    printf("[Root] Program Start\r\n");

    Network_Init(1, 1);
    Network_GetIP(&ulMyIP, 0);

#ifdef _WIN32_
	// Initial Http Server
	tInitSetting.dwVersion = HTTPSERVER_VERSION;
	tInitSetting.dwInitSettingsFlag = HTTPServer_ReceiveBufferSizeFlag |
		                              HTTPServer_MaxConnectionsFlag;
	tInitSetting.dwReceiveBufferSize = 1460;
	tInitSetting.ulMaxConnections = 20;

	tInitSetting.iAuthorizationType = HTTPServer_AuthorizationType_Digest;
	tInitSetting.pcHostName=HTTP_HOST_NAME;
	// HTTP server Initial
	sResult = HTTPServer_Initial(&gServerObj, &tInitSetting);
#endif
	if (sResult != S_OK)
	{
		printf("Initial HTTP server fail\n");
		return 0;
	}

	OSTick_GetMSec(&g_dwVideoLastTick);
	OSTick_GetMSec(&g_dwAudioLastTick);		 

	for( i=0 ; i<MULTIPLE_STREAM_NUM ; i++ )
	{
		if( (pVideoFile[i] = fopen(g_acVideoFileName[i], "rb")) == NULL )
		{
			printf("Open video Input file error\r\n");
			exit(1);
		}
	}

	//pVideoFile[0] = fopen("motion.hgd", "rb");
	//pVideoFile[1] = fopen("d1.hgd", "rb");
	//pVideoFile[2] = fopen("176x144.hgd", "rb");
	
	if((pAudioFile = fopen("amrdump.dat", "rb")) == NULL )
	{
		printf("Open audio Input file error\r\n");
		exit(1);
	}

	//pOutFile = fopen("videoout.dat", "wb");

	// set timeout value (response and keep alive)
	//tOptions.dwFlag = CONTROLCHANNEL_SET_RESPONSETIMEOUT | CONTROLCHANNEL_SET_KEEPALIVETIMEOUT;
	//tOptions.dwResponseTimeout = 10000;
	//tOptions.dwKeepAliveTimeout = 120000;
	//scResult = ControlChannel_SetOptions(ghControlCh, &tOptions);
	//printf("ControlChannel_SetOptions result %08X\r\n", scResult);

	// Initial stream Server
	stRTSPStreamingParam.usRTSPPort = 554;
	stRTSPStreamingParam.usRTPVPort = 9888;
	stRTSPStreamingParam.usRTPAPort = 9890;
	stRTSPStreamingParam.usRTCPVPort= 9889;
	stRTSPStreamingParam.usRTCPAPort= 9891;


#ifdef RTSPRTP_MULTICAST
	stRTSPStreamingParam.stMulticastInfo[0].ulMulticastAddress = inet_addr("239.255.1.134");
	stRTSPStreamingParam.stMulticastInfo[0].usMulticastVideoPort = 4568;
	stRTSPStreamingParam.stMulticastInfo[0].usMulticastAudioPort = 4570;
	stRTSPStreamingParam.stMulticastInfo[0].usTTL = 15;
	stRTSPStreamingParam.stMulticastInfo[0].iAlwaysMulticast = 0;
	stRTSPStreamingParam.stMulticastInfo[0].iRTPExtension = 0;
	stRTSPStreamingParam.stMulticastInfo[0].iSDPIndex = 1;      //media index

	stRTSPStreamingParam.stMulticastInfo[1].ulMulticastAddress = inet_addr("239.255.1.135");
	stRTSPStreamingParam.stMulticastInfo[1].usMulticastVideoPort = 4568;
	stRTSPStreamingParam.stMulticastInfo[1].usMulticastAudioPort = 4570;
	stRTSPStreamingParam.stMulticastInfo[1].usTTL = 15;
	stRTSPStreamingParam.stMulticastInfo[1].iAlwaysMulticast = 0;
	stRTSPStreamingParam.stMulticastInfo[1].iRTPExtension = 0;
	stRTSPStreamingParam.stMulticastInfo[1].iSDPIndex = 1;      // media index

#endif
	
	for( i=0 ; i <MULTIPLE_STREAM_NUM; i++ )
	{
		strcpy(stRTSPStreamingParam.acAccessName[i],g_acAccessName[i]);
		stRTSPStreamingParam.iRTSPStreamingMediaType[i] = g_iStreamMediaType[i];
	}

	//stRTSPStreamingParam.iRTSPStreamingMediaType[0]=RTSPSTREAMING_MEDIATYPE_VIDEOONLY;
	//stRTSPStreamingParam.iRTSPStreamingMediaType[1]=RTSPSTREAMING_MEDIATYPE_VIDEOONLY;
	//stRTSPStreamingParam.iRTSPStreamingMediaType[2]=RTSPSTREAMING_MEDIATYPE_AUDIOVIDEO;
	//strcpy(stRTSPStreamingParam.acAccessName[0],"livem.sdp");
	//strcpy(stRTSPStreamingParam.acAccessName[1],"livel.sdp");
	//strcpy(stRTSPStreamingParam.acAccessName[2],"lives.sdp");


	stRTSPStreamingParam.ulLocalSubnetMask=inet_addr("255.255.255.0");
	stRTSPStreamingParam.ulNATIP=0;
	stRTSPStreamingParam.iRTSPAuthentication = RTSPSTREAMING_AUTHENTICATION_DIGEST;
	stRTSPStreamingParam.iRTSPMaxConnectionNum = SSMAXCONNECTION;
	

#ifdef _SIP
	memset((void*)&(stRTSPStreamingParam.tSIPParameter),0,sizeof(TSIP_PARAM));

	SIPUAClient_ReadConfig(&(stRTSPStreamingParam.tSIPParameter));
#endif

	if( (ghRTSPStreamingServer=RTSPStreaming_Create(&stRTSPStreamingParam)) == NULL )
	{
		printf("RTSP streaming server create failed\r\n");
		exit(0);
	}

	RTSPStreaming_GetVersion(&byMajor, &byMinor, &byBuild, &byRevision);
	printf("RTSP streaming verion %d.%d.%d.%d\n",byMajor,byMinor, byBuild, byRevision);

	RTSPStreaming_SetSDPETag(ghRTSPStreamingServer,"1234567890");
   	RTSPStreaming_SetHostName(ghRTSPStreamingServer,"RTSP server");
	RTSPStreaming_AddAccessList(ghRTSPStreamingServer,inet_addr("0.0.0.0"),inet_addr("255.255.255.254"));        	
	
	//Set the 1st video stream type
	stRTSPVideoParam.iProfileLevel = 8;
	stRTSPVideoParam.iBitRate=19200;
	stRTSPVideoParam.iClockRate= 30000;
	stRTSPVideoParam.iMPEG4HeaderLen = 29;
	stRTSPVideoParam.iWidth = 176;
	stRTSPVideoParam.iHeight = 144;
	stRTSPVideoParam.iDecoderBufferSize = 65536*2;

	strcpy(stRTSPVideoParam.acTrackName,g_acVideoTrackName[0]);

	dwVideoFlag = RTSPSTREAMING_VIDEO_PROLEVE|RTSPSTREAMING_VIDEO_BITRATE|
				RTSPSTREAMING_VIDEO_CLOCKRATE|RTSPSTREAMING_VIDEO_MPEG4CI|
				RTSPSTREAMING_VIDEO_WIDTH|RTSPSTREAMING_VIDEO_HEIGHT|
				RTSPSTREAMING_VIDEO_DECODEBUFF;

	memcpy(stRTSPVideoParam.acMPEG4Header,g_ac176x144MPEG4Header,stRTSPVideoParam.iMPEG4HeaderLen);
	memset(stRTSPVideoParam.acMPEG4Header+stRTSPVideoParam.iMPEG4HeaderLen,0,1);
	RTSPStreaming_SetVideoParameters(ghRTSPStreamingServer,1, &stRTSPVideoParam,dwVideoFlag);

	if(MULTIPLE_STREAM_NUM > 1)
	{
		//Set the 2nd video stream type
		stRTSPVideoParam.iProfileLevel = 8;
		stRTSPVideoParam.iBitRate=19200;
		stRTSPVideoParam.iClockRate= 30000;
		stRTSPVideoParam.iMPEG4HeaderLen = 29;
		stRTSPVideoParam.iWidth = 640;
		stRTSPVideoParam.iHeight = 480;
		stRTSPVideoParam.iDecoderBufferSize = 65536*2;

		strcpy(stRTSPVideoParam.acTrackName,g_acVideoTrackName[1]);

		dwVideoFlag = RTSPSTREAMING_VIDEO_PROLEVE|RTSPSTREAMING_VIDEO_BITRATE|
			RTSPSTREAMING_VIDEO_CLOCKRATE|RTSPSTREAMING_VIDEO_MPEG4CI|
			RTSPSTREAMING_VIDEO_WIDTH|RTSPSTREAMING_VIDEO_HEIGHT|
			RTSPSTREAMING_VIDEO_DECODEBUFF;

		memcpy(stRTSPVideoParam.acMPEG4Header,g_acD1MPEG4Header,stRTSPVideoParam.iMPEG4HeaderLen);
		memset(stRTSPVideoParam.acMPEG4Header+stRTSPVideoParam.iMPEG4HeaderLen,0,1);
		RTSPStreaming_SetVideoParameters(ghRTSPStreamingServer,2, &stRTSPVideoParam,dwVideoFlag);

		//Set the 3rd video stream type
		stRTSPVideoParam.iProfileLevel = 3;
		stRTSPVideoParam.iBitRate=192000;
		stRTSPVideoParam.iClockRate= 30000;
		stRTSPVideoParam.iMPEG4HeaderLen = 28;
		stRTSPVideoParam.iWidth = 352;
		stRTSPVideoParam.iHeight = 240;
		stRTSPVideoParam.iDecoderBufferSize = 65536*2;

		strcpy(stRTSPVideoParam.acTrackName,g_acVideoTrackName[2]);

		dwVideoFlag = RTSPSTREAMING_VIDEO_PROLEVE|RTSPSTREAMING_VIDEO_BITRATE|
			RTSPSTREAMING_VIDEO_CLOCKRATE|RTSPSTREAMING_VIDEO_MPEG4CI|
			RTSPSTREAMING_VIDEO_WIDTH|RTSPSTREAMING_VIDEO_HEIGHT|
			RTSPSTREAMING_VIDEO_DECODEBUFF;

		memcpy(stRTSPVideoParam.acMPEG4Header,g_acMPEG4Header,stRTSPVideoParam.iMPEG4HeaderLen);
		memset(stRTSPVideoParam.acMPEG4Header+stRTSPVideoParam.iMPEG4HeaderLen,0,1);

		RTSPStreaming_SetVideoParameters(ghRTSPStreamingServer,3, &stRTSPVideoParam,dwVideoFlag);
	}

	//we got only one audio bitstream file here. only one stream is allowed to have audio
	stRTSPAudioParam.iBitRate=4750;
	stRTSPAudioParam.iClockRate=8000;
	stRTSPAudioParam.iPacketTime=200;
	stRTSPAudioParam.iOctetAlign=1;
	stRTSPAudioParam.iAMRcrc=0;
	stRTSPAudioParam.iRobustSorting=0;
	stRTSPAudioParam.bIsBigEndian = FALSE;
	
	strcpy(stRTSPAudioParam.acTrackName,g_acAudioTrackName);

	dwAudioFlag = RTSPSTREAMING_AUDIO_BITRATE|RTSPSTREAMING_AUDIO_CLOCKRATE|
				RTSPSTREAMING_AUDIO_PACKETTIME|RTSPSTREAMING_AUDIO_OCTECTALIGN|
				RTSPSTREAMING_AUDIO_AMRCRC|RTSPSTREAMING_AUDIO_ROBUSTSORT|
				RTSPSTREAMING_AUDIO_PACKINGMODE;

	//RTSPStreaming_SetAudioParameters(ghRTSPStreamingServer,1,&stRTSPAudioParam,dwAudioFlag);
	//RTSPStreaming_SetAudioParameters(ghRTSPStreamingServer,2,&stRTSPAudioParam,dwAudioFlag);
	RTSPStreaming_SetAudioParameters(ghRTSPStreamingServer,1,&stRTSPAudioParam,dwAudioFlag);
	RTSPStreaming_SetAudioParameters(ghRTSPStreamingServer,2,&stRTSPAudioParam,dwAudioFlag);
	RTSPStreaming_SetAudioParameters(ghRTSPStreamingServer,3,&stRTSPAudioParam,dwAudioFlag);


    RTSPStreaming_SetControlCallback(ghRTSPStreamingServer,(FControlChannel_Callback) RTSPStreamServerCtrlChCallback, 0);
    RTSPStreaming_SetVideoCallback(ghRTSPStreamingServer, (MEDIA_CALLBACK)RTSPStreamServerVideoCallback, 0);
    RTSPStreaming_SetAudioCallback(ghRTSPStreamingServer, (MEDIA_CALLBACK)RTSPStreamServerAudioCallback, 0);

#ifdef _WIN32_
	// Set Callback function
	HTTPServer_SetCallback(gServerObj, HttpServerCallback, 1);
    
		// Start the Http Server task
	if(HTTPServer_Start(gServerObj) != S_OK)
	{
		printf("Start server fail!\r\n");
		return 0;
	}
#endif

	RTSPStreaming_Start(ghRTSPStreamingServer);
		
	// Just wait
	//Sleep(INFINITE);
	printf("all task is ready\r\n");
    {
        char szLine[80];

        while(1)
        {
            gets(szLine);
            if(strcmp(szLine, "exit") == 0)
            {
                break;
            }
        }
    }

    // Stop the RTSP streaimng server task
    RTSPStreaming_Stop(ghRTSPStreamingServer);

#ifdef _WIN32_
    // Stop the HTTP server task
    HTTPServer_Stop(gServerObj);

    // Release HTTP Server
    HTTPServer_Release(&gServerObj);
#endif    
    // Release RTSP streaming Server
    RTSPStreaming_Close(&ghRTSPStreamingServer);

	for( i=0 ; i<MULTIPLE_STREAM_NUM ; i++ )
	{
		if (pVideoFile[i])
		{
			fclose(pVideoFile[i]);
		}
	}
	fclose(pAudioFile);

    OS_Exit(0);
//	return 0;

}



