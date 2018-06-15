#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <sched.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "osisolate.h"
#include "rtsps_local.h"
#include "rtsps_fdipc.h"

BOOL bEnd;
HANDLE hRTSPS;
char acConfigFile[50];
int g_iSaveBandwidth = 1 ;
volatile char rtsps_app_rcsid[] = "$Id: " RTSPS_VERSION_STRING " rtsps "RTSPS_MODIFY_DATETIME" $";

//20110725 Add by danny For Multicast RTCP receive report keep alive
#ifdef RTSPRTP_MULTICAST
int g_iMulticastTimeout;
#endif

void sig_term(int signo)
{
	printf("recv signal no %d\n", signo);
	bEnd = TRUE;
	return;
}

void sig_savebandwidth(int signo)
{
	g_iSaveBandwidth = (g_iSaveBandwidth) ? 0 : 1 ;
	printf("recv signal no %d SaveBandwidth %d\n", signo, g_iSaveBandwidth);
	return;
}

void sig_updateAccount(int signo)
{
	StreamingServer_AccountManagerParse(hRTSPS);
	
	return;
}


void sig_updateDynamicParameter(int signo)
{
	printf("dynamic update param !!\n");
	StreamingServer_UpdateDynamicPamater(hRTSPS,acConfigFile);
	
	return;
}

void usage(void)
{
	printf("Streaming Server App Version: %d.%d.%d.%d\n"
	         "RTSPSTREAMING Server Module Version: %d.%d.%d.%d\n"
		   "Usage:\n"
		   "    rtsps.out -a <address> -c <config file> -l <access list> -d(optional)\n"
		   "    address:     IP address of streaming server\n"
		   "    config file: configuration file of streaming server\n"
		   "    access list: access list file of streaming server to perform IP filtering\n"
		   "    daemon mode: -d (optional)\n",
		   ( (unsigned int)RTSPS_VERSION      ) % 256,
		   ( (unsigned int)RTSPS_VERSION >> 8 ) % 256,
		   ( (unsigned int)RTSPS_VERSION >> 16) % 256,
		   ( (unsigned int)RTSPS_VERSION >> 24),
		   ( (unsigned int)RTSPSTREAMINGSERVER_VERSION      ) % 256,
		   ( (unsigned int)RTSPSTREAMINGSERVER_VERSION >> 8 ) % 256,
		   ( (unsigned int)RTSPSTREAMINGSERVER_VERSION >> 16) % 256,
		   ( (unsigned int)RTSPSTREAMINGSERVER_VERSION >> 24));
	exit(1);
}

int main(int argc, char *argv[])
{
	TRTSPSInitOptions	tRtspInitOpt;
	FILE				*fp;
	char				acAccessFile[50];
	char				acQosFile[64];		//20090223 QOS
	int					ch;
	TSTREAMSERVERINFO	*pThis = NULL;

	bEnd = FALSE;
#ifdef _SCHED_PRIORITY
	struct sched_param param;
	int low_priority, high_priority;
#endif
	memset(&tRtspInitOpt,0,sizeof(TRTSPSInitOptions));
	acConfigFile[0] = 0;
	acAccessFile[0] = 0;
	acQosFile[0] = 0;
	
	OS_Init();
	
	while ((ch = getopt(argc, argv, "da:c:l:q:")) != -1) 
	{
		switch (ch) 
		{
		case 'a':					
			strncpy((char*)&(tRtspInitOpt.szIPAddr),optarg, sizeof(tRtspInitOpt.szIPAddr) - 1);		//CID:1083, CHECKER:STRING_OVERFLOW
			break;
			
		case 'c':
			strncpy(acConfigFile,optarg, sizeof(acConfigFile) - 1);	//CID:1083, CHECKER:STRING_OVERFLOW
			acConfigFile[sizeof(acConfigFile) - 1] = 0;
			break;			
		
		case 'l':
			strncpy(acAccessFile,optarg, sizeof(acAccessFile) - 1);	//CID:1083, CHECKER:STRING_OVERFLOW
			acAccessFile[sizeof(acAccessFile) - 1] = 0;
			break;	
			
		case 'd':
			printf("run as daemon mode\n");
			daemon(0,0);
			break;

		case 'q':									
			strncpy(acQosFile,optarg, sizeof(acQosFile) - 1);		//CID:1083, CHECKER:STRING_OVERFLOW
			acQosFile[sizeof(acQosFile) - 1] = 0;
			break;
				
		default:
			usage();
		}
	}
	//20081231 If access-file is not provided, the program can still continue!
	if( tRtspInitOpt.szIPAddr[0] == 0 || acConfigFile[0] == 0)
	{
		usage();
		exit(1);
	}

#ifdef _SCHED_PRIORITY
	memset (&param, 0, sizeof(param));
	/* Get parameters to use later.  Do this now  */
	/* Avoid overhead during time-critical phases.*/
	high_priority = sched_get_priority_max(SCHED_RR);
	low_priority = sched_get_priority_min(SCHED_RR);
	param.sched_priority = low_priority;
	
	if( sched_setscheduler(0, SCHED_RR, &param) != 0)
	{
		printf("Set scheduler fail, %s\n",strerror(errno) );
		exit(1);
	}
#endif

#ifdef _NICEVALUE
	if (setpriority(PRIO_PROCESS, 0, _NICEVALUE) == 0)
	{
		printf("priority changed, new=%d\n", _NICEVALUE);
	}
	else
	{
		printf("Set priority fail, %s\n", strerror(errno));
	}
#endif

#ifdef _ANTI_COUNTERFEIT
	if(ACLib_CheckKey() != S_OK)
	{
		exit(1);
	}
#endif

	openlog(RTSP_SYSLOG_ID_STRING,0, LOG_USER);

	tRtspInitOpt.dwStreamNumber = MULTIPLE_STREAM_NUM;	
	tRtspInitOpt.dwVersion = RTSPS_VERSION;
	rtspstrcpy((char*)&(tRtspInitOpt.szSubnetMask),"255.255.255.0", sizeof(tRtspInitOpt.szSubnetMask));

    signal(SIGPIPE,SIG_IGN); // intercept the broken pipe signal
	signal(SIGTERM, sig_term);
	signal(SIGINT, sig_term);
	signal(SIGUSR1,sig_updateAccount);
	signal(SIGUSR2,sig_updateDynamicParameter);
	signal(SIGHUP, sig_savebandwidth) ;

	if(StreamingServer_Initial(&hRTSPS, &tRtspInitOpt,acConfigFile,acAccessFile, acQosFile) != S_OK)
	{	
		printf("initial rtsp server fail\n");
		exit(1);
	}
	//20080829 merge fdipc thread
	pThis = (TSTREAMSERVERINFO *)hRTSPS;

	if (StreamingServer_Start(hRTSPS) != S_OK)
	{
		printf("start rtsp server fail\n");
		exit(1);
	}
	//20090324 multiple stream
    if( StreamingServer_SetMediaTrackParam(hRTSPS, NULL) != S_OK)
    {
        printf("Set media track parameter fail\n");        
        exit(1);
    }

	if( (fp = fopen(RTSPS_PID_FILE,"w"))!= NULL )
	{
		int pid;
		pid = getpid();
		fprintf(fp,"%d",pid);
		fclose(fp);
	}
	else
	{
        printf("pid file create fail\n");        
        exit(1);
    }	
	
	//20080829 allocate buffer for fdipc
	if((pThis->ptMsgInfo = (TMessageInfo *)calloc(RTSPS_MESSAGE_NUMBER, sizeof(TMessageInfo))) == NULL)
	{
		printf("Error initialize fdipc server!\n");
		exit(1);
	}
	memset(&pThis->tSSConnSettings, 0, sizeof(TStreamServer_ConnectionSettings));
	printf("start fdipc server\n");
	
	//20090626 SWatchDog
	InitializeSWatchDog(hRTSPS);

	while(1)
	{
		//20080829 fdipc thread eliminated
		RTPOverHttpSocketExchanger(hRTSPS);
		
		//20090626 SWatchDog
		//20140605 Modified by Charles to let swatchdog restart the server when RTSPServer thread is lock
		if(!pThis->bRTSPServerRestarting)
		{
			KickSWatchDog(hRTSPS);
		}

		//20110401 Added by danny For support RTSPServer thread watchdog
		CheckRTSPServerThreadAlive(hRTSPS);
		
		if (bEnd)
		{
			break;
		}
	}

	//20090626 SWatchDog
	ReleaseSWatchDog(hRTSPS);

	//20080829 release buffer for fdipc 
	if(pThis->ptMsgInfo != NULL)
	{
		free(pThis->ptMsgInfo);
	}

	if (StreamingServer_Stop(hRTSPS) != S_OK)
	{
		printf("stop rtsp server fail\n");
		exit(1);
	}

	if (StreamingServer_Release(&hRTSPS) != S_OK)
	{
		printf("release rtsp server fail\n");
		exit(1);
	}

	remove(RTSPS_PID_FILE);
	return 0;
}
