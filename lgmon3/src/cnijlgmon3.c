/*
 *  Canon Inkjet Printer Driver for Linux
 *  Copyright CANON INC. 2001-2021
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA.
 *
 * NOTE:
 *  - As a special exception, this program is permissible to link with the
 *    libraries released as the binary modules.
 *  - If you write modifications of your own for these programs, it is your
 *    choice whether to permit this exception to apply to your modifications.
 *    If you do not wish that, delete this exception.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef ENABLE_NLS
#include <libintl.h>
#endif

#include "cnijcomif.h"
#include "cnijlgmon3.h"
#include "cnijifusb.h"
#include "keytext.h"

#include "cnclcmdutils.h"
#include "./common/libcnnet3_type.h"
#include "cnijifnet2.h"

int (*CNCL_GetString)(const char*, const char*, int, uint8_t**);

static char	uuid[UUID_LEN + 1];
static int	isPrinting = CNNET3_FALSE;
static int	isCanceled = CNNET3_FALSE;
static char	_jobID[10] = "";
static int	prot = 0;

// #define _DEBUG_MODE_

int interrupt_sign = 0;
int (*GET_RESPONSE)(char *, int, int *, char *, int **);
int (*GET_STATUS)(char *, int, int *, int * , char *);
int (*GET_STATUS2)(char *, int, char *, int *, int * , char *, char *);
int (*GET_STATUS2_MAINTENANCE)(char *, int, char *, int *, int * , char *, char *);
int (*GET_PROTOCOL)(char *, size_t);


 int main(int argc, char *argv[])
{
#ifdef _DEBUG_MODE_
	fprintf(stderr, "DEBUG: lgmon3 started.\n");
#endif

	/*--------------param setting-----------------*/
	CNIF_INFO if_info;
	FILE *fp = stdin;
	//ppd_file_t *ppd;
	//ppd_attr_t *attr;
	int err = 0;
	int sign = 1;
	int copies = 0;
	int installer = 0;
	int response_detail = 0;
	size_t transfered_data = 0;
	size_t readed_data = 0;
	// char *cmd_buffer = NULL;
	char *write_buffer = NULL;
	char *job_id = NULL;
	char *temp_job = NULL;
	char *dev_uri = NULL;
	char *deviceID = NULL;
	char *library_path = NULL;
	char model_number[] = "com2";
	int isFirst;

	char *serviceType = getenv("CONTENT_TYPE");


	/*--------------localize init-----------------*/
	setlocale( LC_ALL, "" );
	bindtextdomain( PACKAGE, PACKAGE_LOCALE_DIR );
	bind_textdomain_codeset( PACKAGE, "UTF-8" );
	textdomain( PACKAGE );
	SetKeyTextDir( PACKAGE_DATA_DIR );
	if ( LoadKeyTextList() != 0 ){

#ifdef _DEBUG_MODE_
		fprintf(stderr, "DEBUG: lgmon3 LoadKeyTextList failed.\n");
#endif
		goto onErr;
	}

	/*--------------param init-----------------*/
	// cmd_buffer = (char *)malloc(CN_READ_SIZE);
	// if(cmd_buffer == NULL) {
	// 	err = CN_LGMON_OTHER_ERROR;
	// 	goto onErr;
	// }

	write_buffer = (char *)malloc(CN_WRITE_SIZE);
	if(write_buffer == NULL) {
		err = CN_LGMON_OTHER_ERROR;
		goto onErr;
	}

	job_id = (char *)malloc(CN_IVEC_JOBID_LEN);
	if(job_id == NULL) {
		err = CN_LGMON_OTHER_ERROR;
		goto onErr;
	}

	temp_job = (char *)malloc(CN_IVEC_JOBID_LEN);
	if(temp_job == NULL) {
		err = CN_LGMON_OTHER_ERROR;
		goto onErr;
	}

	dev_uri = (char *)malloc(CN_DEVICE_URI_LEN);
	if(dev_uri == NULL) {
		err = CN_LGMON_OTHER_ERROR;
		goto onErr;
	}

	deviceID = (char *)malloc(CN_DEVICE_ID_LEN);
	if(deviceID == NULL) {
		err = CN_LGMON_OTHER_ERROR;
		goto onErr;
	}

	library_path = (char *)malloc(CN_LIB_PATH_LEN);
	if(library_path == NULL) {
		err = CN_LGMON_OTHER_ERROR;
		goto onErr;
	}

	// memset(cmd_buffer, 		'\0', CN_READ_SIZE);
	memset(write_buffer, 	'\0', CN_WRITE_SIZE);
	memset(job_id, 			'\0', CN_IVEC_JOBID_LEN);
	memset(temp_job, 		'\0', CN_IVEC_JOBID_LEN);
	memset(dev_uri, 		'\0', CN_DEVICE_URI_LEN);
	memset(deviceID, 		'\0', CN_DEVICE_ID_LEN);
	memset(library_path, 	'\0', CN_LIB_PATH_LEN);

	CNIF_SetSignal(CN_SIGTERM);
	/*--------------switch option(argv[])-----------------*/
#ifdef _DEBUG_MODE_
	fprintf(stderr, "DEBUG: argc = %d\n", argc);
#endif

	switch(argc) {
		case 1:
			/* discover */
			err = CNIF_Discover(&if_info, installer);
			if (err < 0) {
				err = CN_LGMON_OTHER_ERROR;
			}
			return err;
			break;
		case 2:
			/* --installer_usb --search */
			if(!strncmp(CN_CMD_INSTALL_USB, argv[1], sizeof(CN_CMD_INSTALL_USB))){
				if_info.ifType = CNIF_TYPE_USB;
				installer = 1;
				err = CNIF_Discover(&if_info, installer);
				if (err < 0) {
					err = CN_LGMON_OTHER_ERROR;
				}
				return err;
			/* --installer_net --search */
			} else if(!strncmp(CN_CMD_INSTALL_NET, argv[1], sizeof(CN_CMD_INSTALL_NET))){
				if_info.ifType = CNIF_TYPE_NET;
				installer = 1;
				err = CNIF_Discover(&if_info, installer);
				if (err < 0) {
					err = CN_LGMON_OTHER_ERROR;
				}

				if_info.ifType = CNIF_TYPE_NET2;
				installer = 1;
				err = CNIF_Discover(&if_info, installer);

				if (err < 0) {
					err = CN_LGMON_OTHER_ERROR;
				}

				return err;
			}
#ifdef _DEBUG_MODE_
			fprintf(stderr, "ERROR: command error\n");
#endif
			err = CN_LGMON_CMD_ERROR;
			return err;;
			break;
		// case 3:
		case 5:
			copies = atoi(argv[2]);
			break;
		default:
#ifdef _DEBUG_MODE_
			fprintf(stderr, "ERROR: command error\n");
#endif
			err = CN_LGMON_CMD_ERROR;
			return err;
			break;
	}

	/* read the file data before tansferdata */
	readed_data = fread(write_buffer, sizeof(char), CN_WRITE_SIZE, fp);


	/*---------------dynamic link of library---------------------*/
	void *libclss = NULL;

	snprintf(library_path, CN_LIB_PATH_LEN - 1, CN_CNCL_LIB_PATH, model_number);

#ifdef _DEBUG_MODE_
	fprintf(stderr, "DEBUG: progpath = %s\n", library_path);
#endif

	libclss = dlopen(library_path, RTLD_LAZY);
	if(!libclss){

#ifdef _DEBUG_MODE_
		fprintf(stderr, "ERROR: dynamic link error.(%s)", dlerror());
#endif
		return CN_LGMON_DYNAMID_LINK_ERROR;
	}

	GET_RESPONSE = dlsym(libclss, "CNCL_GetInfoResponse");
	if(dlerror() != NULL){

#ifdef _DEBUG_MODE_
		fprintf(stderr, "DEBUG: cannnot find function2.(%s)", dlerror());
#endif
		return CN_LGMON_DYNAMID_LINK_ERROR;
	}
	GET_STATUS = dlsym(libclss, "CNCL_GetStatus");
	if(dlerror() != NULL){

#ifdef _DEBUG_MODE_
		fprintf(stderr, "DEBUG: cannnot find function3.(%s)", dlerror());
#endif
		return CN_LGMON_DYNAMID_LINK_ERROR;
	}

	GET_PROTOCOL = dlsym(libclss, "CNCL_GetProtocol");
	if(dlerror() != NULL){

#ifdef _DEBUG_MODE_
		fprintf(stderr, "DEBUG: cannnot find function4.(%s)", dlerror());
#endif

		return CN_LGMON_DYNAMID_LINK_ERROR;
	}

	GET_STATUS2 = dlsym(libclss, "CNCL_GetStatus2");
	if(dlerror() != NULL){

#ifdef _DEBUG_MODE_
		fprintf(stderr, "DEBUG: cannnot find function5.(%s)", dlerror());
#endif

		return CN_LGMON_DYNAMID_LINK_ERROR;
	}

	GET_STATUS2_MAINTENANCE = dlsym(libclss, "CNCL_GetStatus_Maintenance");
	if(dlerror() != NULL){

#ifdef _DEBUG_MODE_
		fprintf(stderr, "DEBUG: cannnot find function6.(%s)", dlerror());
#endif

		return CN_LGMON_DYNAMID_LINK_ERROR;
	}

	/*--------------duplicate device uri-----------------*/
#ifdef _DEBUG_MODE_
	fprintf(stderr, "DEBUG: dynamic link ok!!\n");
#endif

	strncpy(dev_uri, argv[1], CN_DEVICE_URI_LEN);

#ifdef _DEBUG_MODE_
	fprintf(stderr, "DEBUG: dev_uri = %s\n", dev_uri);
#endif

	/*--------------get deviceID-----------------*/
	err = CNIF_GetSerialNum(&if_info, (uint8_t *)dev_uri, CN_DEVICE_URI_LEN, deviceID);
	if(err < 0) {
		err = CN_LGMON_OTHER_ERROR;
		goto onErr;
	}

#ifdef _DEBUG_MODE_
	fprintf(stderr, "DEBUG: serial = %s\n", deviceID);
#endif

	memset( uuid, '\0', UUID_LEN + 1 );

	if( GetUUID( argv[3], uuid ) != 0 ){
		strncpy( uuid, argv[4], UUID_LEN );
	}

#ifdef _DEBUG_MODE_
	fprintf(stderr, "DEBUG: [lgmon3] uuid = %s\n", uuid);
#endif

	const char *p_ppd_name = getenv("PPD");

	CAPABILITY_DATA capability;
	memset(&capability, 0, sizeof(CAPABILITY_DATA));

#ifdef _DEBUG_MODE_
	fprintf(stderr, "DEBUG: [GetCapabilityFromPPDFile] Call\n");
#endif

	if( ! GetCapabilityFromPPDFile(p_ppd_name, &capability) ){

#ifdef _DEBUG_MODE_
		fprintf(stderr, "DEBUG: [GetCapabilityFromPPDFile] failed : %s\n", dlerror());
#endif

		goto onErr;
	}

#ifdef _DEBUG_MODE_
	fprintf(stderr, "DEBUG: [CNCL_GetProtocol] call\n");
#endif

	prot = GET_PROTOCOL((char *)capability.deviceID, capability.deviceIDLength);

	// if( prot == 2 ){
	if( if_info.ifType != CNIF_TYPE_USB && prot == 2 ){
		if_info.ifType = CNIF_TYPE_NET2;
	}

	/*--------------open device-----------------*/
#ifdef _DEBUG_MODE_
		fprintf(stderr, "DEBUG: [CNIF_Open] Call\n");
#endif

	do {
		err = CNIF_Open(deviceID, &if_info);

#ifdef _DEBUG_MODE_
		fprintf(stderr, "DEBUG: CNIF_Open ret = %d\n", err);
#endif

		if(err < 0) {
			fprintf(stderr, "INFO: %s\n", LookupText( "LBM_PREPARING" ));
			sleep(30);
		}
	} while (err < 0);

	/*---------------clear printer buffer----------------------*/
#ifdef _DEBUG_MODE_
		fprintf(stderr, "DEBUG: [CNIF_GetResponse] Call\n");
#endif

	err = CNIF_GetResponse(&if_info, temp_job, &response_detail, CNNET3_FALSE);
	if(err < 0) {
		err = CN_LGMON_RESPONSE_ERROR;
		goto onErr;
	}

	/*---------------check printer status----------------------*/
	while(1) {
		/* check intterrupt signal */
		if(interrupt_sign == 1){
			fprintf(stderr, "INFO: \n");
			goto onErr;
		}

		err = CNIF_GetResponse(&if_info, temp_job, &response_detail, CNNET3_FALSE);

		if (err >= 0) {
			// if( err != CN_IVEC_STATUS_IDLE ) {
			if( err != CN_IVEC_STATUS_IDLE && err != CLSS_OK_AND_IDLE && err != CLSS_NOT_SUPP_AND_IDLE ) {
				sleep(1);
				continue;
			}
			break;
		} else {
			err = CN_LGMON_RESPONSE_ERROR;
			goto onErr;
		}
	}

	fprintf(stderr, "DEBUG: session start\n");
	/*--------------start session-----------------*/
	while (1){
		/* check intterrupt signal */
		if(interrupt_sign == 1){
			fprintf(stderr, "INFO: \n");
			goto onErr;
		}

#ifdef _DEBUG_MODE_
		fprintf(stderr, "DEBUG: [CNIF_StartSession] Call\n");
#endif

		/* start session */
		err = CNIF_StartSession(&if_info);

#ifdef _DEBUG_MODE_
		fprintf(stderr, "DEBUG: [CNIF_StartSession] return : %d\n", err);
#endif

		if (err < 0 && err != CN_LGMON_BUSY){
			err = CN_LGMON_OPEN_ERROR;
			goto onErr;
		}
		else if (err == CN_LGMON_OK){
			break;
		}
		else
		{
			// display status
			err = CNIF_GetResponse(&if_info, temp_job, &response_detail, CNNET3_FALSE);

			// device is busy
			// fprintf(stderr, "INFO: %s\n", LookupText( "LBM_BUSY" ));
			sleep(4);
			continue;
		}
	}

	fprintf(stderr, "INFO: %s\n", LookupText( "LBM_PRINTING" ));

    /*---------------TransferData-----------------------------*/
	time_t start_time;
	time_t end_time;
	double past_sec = 0;
	time(&start_time);

	int times = 0;
	size_t max_read_size = 0;

	int flgSendStart = CNNET3_FALSE;

	while(copies > 0) {
		copies--;

		while(sign) {
			max_read_size = readed_data;

			if (readed_data < CN_WRITE_SIZE) {
				if (feof(fp)) {
					sign = 0;	//EOF
				}
				if (ferror(fp)) {
#ifdef _DEBUG_MODE_
					fprintf(stderr, "ERROR: fread error\n");//ERROR_MSG
#endif
					err = CN_LGMON_OTHER_ERROR;
					goto onErr;
				}
			}

			/* transfer data to printer */
			for( ; readed_data != 0; readed_data -= transfered_data) {
				time(&end_time);
				past_sec = difftime(end_time, start_time);

				if(past_sec >= 4){
					time(&start_time);

#ifdef _DEBUG_MODE_
					fprintf(stderr, "DEBUG: [CNIF_GetResponse] 4sec\n");
#endif

					if ( strncmp( serviceType, SVC_MAINTENANCE, sizeof(SVC_MAINTENANCE) )  == 0 ){
						err = CNIF_GetResponse(&if_info, temp_job, &response_detail, CNNET3_TRUE);
					}
					else{
						err = CNIF_GetResponse(&if_info, temp_job, &response_detail, CNNET3_FALSE);
					}

					if(err == CN_IVEC_STATUS){
#ifdef _DEBUG_MODE_
						fprintf(stderr, "DEBUG: -------status----%d----\n", err);
#endif
					} else if((err == CN_IVEC_STARTJOB) || (err == CN_IVEC_ENDJOB) || (err == CN_IVEC_VENDERSHIFT)){
#ifdef _DEBUG_MODE_
						fprintf(stderr, "DEBUG: returned is %d, not status_res\n", err);//ERROR_MSG
#endif
					// } else if(err == CN_IVEC_STATUS_CANCELING){
					} else if( err == CN_IVEC_STATUS_CANCELING || err == CLSS_OK_AND_CANCEL || err == CLSS_NOT_SUPP_AND_CANCEL ){

						interrupt_sign = 1;
						isCanceled = CNMPU2_TRUE;
#ifdef _DEBUG_MODE_
						fprintf(stderr, "ERROR: Response CN_IVEC_STATUS_CANCELING(%d)\n", err);
#endif
					}
				}

				transfered_data = 0;

				err = CNIF_Write(&if_info, (uint8_t *)write_buffer + (max_read_size - readed_data), readed_data, &transfered_data);

#ifdef _DEBUG_MODE_
				// fprintf(stderr, "DEBUG: [%d]:readed data = %d, write data = %d\n", times, readed_data, transfered_data);
#endif

				if(err < 0) {

#ifdef _DEBUG_MODE_
					// fprintf(stderr, "ERROR: Data send error(%d). transfered_data = %d\n", err, transfered_data);//ERROR_MSG
#endif

					if( interrupt_sign == 1 && if_info.ifType == CNIF_TYPE_NET2 ){
						continue;
					}

					if( err == CN_NET3_SEND_TIMEOUT && if_info.ifType == CNIF_TYPE_NET2 ){
						// fprintf(stderr, "INFO: %s\n", LookupText( "LBM_CANT_COMM_PRINT" ));
						continue;
					}

					//fprintf(stderr, "ERROR: %s\n", LookupText( "LBM_CANT_COMM_PRINT" ));
					fprintf(stderr, "INFO: %s\n", LookupText( "LBM_CANT_COMM_PRINT" ));
					err = CN_LGMON_WRITEDATA_ERROR;
					goto onErr;
		    	}
				else {
					flgSendStart = CNNET3_TRUE;
				}

				if(interrupt_sign == 1){
#ifdef _DEBUG_MODE_
					fprintf(stderr, "DEBUG: [lgmon3] Cancel print\n");
#endif
					//reset and close printer

					if( if_info.ifType == CNIF_TYPE_USB || if_info.ifType == CNIF_TYPE_NET2 ){
						if( isPrinting == CNMPU2_TRUE && isCanceled == CNMPU2_TRUE ){
							CNIF_Cancel(&if_info);
							err = CN_LGMON_OK;
							isPrinting = CNNET3_FALSE;
							isCanceled = CNNET3_FALSE;

							fprintf(stderr, "INFO: \n");
							goto onErr;
						}
						else if( isPrinting == CNNET3_FALSE && isCanceled == CNMPU2_TRUE ){
							goto onErr;
						}
					}
					// else if( if_info.ifType == CNIF_TYPE_USB || if_info.ifType == CNIF_TYPE_NET ){
					else if( if_info.ifType == CNIF_TYPE_NET ){
						//reset and close printer
						err = CNIF_Cancel(&if_info);
						err = CN_LGMON_OK;
						fprintf(stderr, "INFO: \n");
						goto onErr;
					}
				}
			}
			times++;

			/* read the file data */
			memset(write_buffer, '\0', CN_WRITE_SIZE);
			readed_data = fread(write_buffer, sizeof(char), CN_WRITE_SIZE, fp);
		};
	}
#ifdef _DEBUG_MODE_
	fprintf(stderr, "DEBUG: all data sended\n");
#endif


	// for keep session
	time_t start_time2;
	time_t end_time2;
	double past_sec2 = 0;
	time(&start_time2);


	if ( strncmp( serviceType, SVC_MAINTENANCE, sizeof(SVC_MAINTENANCE) )  == 0 ){
		sleep(2);
	}

	/*------------check printer status-----------*/
	isFirst = 1;
	while(1) {
		if( if_info.ifType == CNIF_TYPE_NET2 ){
			if( flgSendStart == CNNET3_TRUE ){
				time(&end_time2);
				past_sec2 = difftime(end_time2, start_time2);

				if(past_sec2 >= 15){

#ifdef _DEBUG_MODE_
					fprintf(stderr, "DEBUG: [CNIF_KeepSession] Call\n");
#endif

					time(&start_time2);
					CNIF_KeepSession( &if_info );
				}

				if( isPrinting == CNMPU2_TRUE && isCanceled == CNMPU2_TRUE ){

#ifdef _DEBUG_MODE_
					fprintf(stderr, "DEBUG: [CNIF_Cancel] Call\n");
#endif

					err = CNIF_Cancel(&if_info);
					isPrinting = CNNET3_FALSE;
					isCanceled = CNNET3_FALSE;

					fprintf(stderr, "INFO: \n");
					goto onErr;
				}

				if ( strncmp( serviceType, SVC_MAINTENANCE, sizeof(SVC_MAINTENANCE) )  == 0 ){
					err = CNIF_GetResponse(&if_info, temp_job, &response_detail, CNNET3_TRUE);
				}
				else{
					err = CNIF_GetResponse(&if_info, temp_job, &response_detail, CNNET3_FALSE);
				}

#ifdef _DEBUG_MODE_
				fprintf(stderr, "DEBUG: [CNIF_GetResponse] waiting for complete : %d\n", err);
				fprintf(stderr, "DEBUG: waiting for complete : isPrinting : %d\n", isPrinting);
#endif

	            if( err == CLSS_OK ) {
					// error
	                if( response_detail != CN_LGMON_OK ){
						err = CN_LGMON_RESPONSE_ERROR;
						goto onErr;
	                }

					sleep(4);
					continue;
	            }
				else if ( strncmp( serviceType, SVC_MAINTENANCE, sizeof(SVC_MAINTENANCE) )  == 0 &&
                          err == CLSS_NOT_SUPPORT ){
					isPrinting = CNNET3_FALSE;
					break;
				}
	            else if ( ( isPrinting == CNMPU2_TRUE && err == CLSS_NOT_SUPPORT_CUSTOM ) || ( isPrinting == CNMPU2_TRUE && err == CLSS_NOT_SUPP_AND_IDLE )
							 || ( isPrinting == CNMPU2_TRUE && err == CLSS_NOT_SUPP_AND_CANCEL ) ) {
	                // CNIJLOG_MSG(@"print job finished");
					isPrinting = CNNET3_FALSE;
					break;
				}
				else{
					sleep(4);
					continue;
				}
			}
			else{
				break;
			}
		}
		else if( if_info.ifType == CNIF_TYPE_USB || if_info.ifType == CNIF_TYPE_NET ){
			if ( isFirst ) {
				sleep(2);
				isFirst = 0;
			}
			/* check intterrupt signal */
			if(interrupt_sign == 1){
				fprintf(stderr, "INFO: \n");
				goto onErr;
			}

			err = CNIF_GetResponse(&if_info, temp_job, &response_detail, CNNET3_FALSE);
			if (err >= 0) {
				if(err != CN_IVEC_STATUS_IDLE) {
					sleep(4);
					continue;
				}
				break;
			} else {
				err = CN_LGMON_RESPONSE_ERROR;
				goto onErr;
			}
		}
	}
	/*-----------close dynamic link--------------*/
	if(libclss != NULL){
		dlclose(libclss);
	}
	
	fprintf(stderr, "INFO: \n");
	goto OK;
	
onErr:
	if(err < 0) {
		err = 1;
	}
	
	CNIF_Reset(&if_info);
	
OK:
	/*--------------close device-----------------*/
#ifdef _DEBUG_MODE_
	fprintf(stderr, "DEBUG: device close\n");
#endif
	CNIF_Close(&if_info);
	
	/*--------------free ptr-----------------*/
	// free(cmd_buffer);
	free(write_buffer);
	free(job_id);
	free(temp_job);
	free(dev_uri);
	free(deviceID);
	free(library_path);
	//free(model_number);
	
	FreeKeyTextList();
	FreeKeyTextDir();
	
	return err;
}

/* set signal */
void CNIF_SetSignal(int sign)
{
	/*struct  sigaction action;
	//sigset
	sigset(sign, CNIF_SigCatch);
	
	//sigaction
	memset(&action, 0, sizeof(action));
	action.sa_handler = CNIF_SigCatch;
	sigaction(sign, &action, NULL);
	*/
	//signal
	if (SIG_ERR == signal(sign, CNIF_SigCatch)) {
#ifdef _DEBUG_MODE_
		fprintf(stderr, "DEBUG: failed to set signal handler\n"); 
#endif
		exit(1); 
	}
}

//signal catch
void CNIF_SigCatch(int sign)
{
#ifdef _DEBUG_MODE_
	fprintf(stderr, "DEBUG: write_process catch the interrupt signal %d\n", sign);//DEBUG_MSG
#endif

	interrupt_sign = 1;
	isCanceled = CNMPU2_TRUE;
}

int CNIF_GetResponse(CNIF_INFO *if_info, char *job_id, int *response_detail, int isMaintenance)
{
	/*---------------define parameter-------------------------*/
    int err = 0;
	int result = 0;
	int opration_id = 0;
	size_t read_size = 0;
	int sumsize = 0;
	int status = 0;
	int statusDetail = 0;
	char dum_id[] = "11111111";
	char *supportID = NULL;
	char *stat_msg = NULL;
	char *stat_tmp_msg = NULL;
	char *cmdBuffer = NULL;
	char *cmdBuffer_big = NULL;
	char *tmpBuffer = NULL;
	char *tmpBuffer_big = NULL;
	char *jobId = NULL;

	/*---------------init parameter-------------------------*/
	supportID = (char *)malloc(CN_IVEC_SUPPORT_CODE_LENGTH);
	if(supportID == NULL){
		err = -1;
		goto onErr;
	}
	
	stat_msg = (char *)malloc(CN_STAT_MSG_LEN);
	if(stat_msg == NULL){
		err = -1;
		goto onErr;
	}
	
	stat_tmp_msg = (char *)malloc(CN_STAT_MSG_LEN);
	if(stat_tmp_msg == NULL){
		err = -1;
		goto onErr;
	}

	cmdBuffer = (char *)malloc(CN_READ_SIZE);
	if(cmdBuffer == NULL){
		err = -1;
		goto onErr;
	}

	cmdBuffer_big = (char *)malloc(CN_READ_SIZE_BIG);
	if(cmdBuffer_big == NULL){
		err = -1;
		goto onErr;
	}

	tmpBuffer = (char *)malloc(CN_READ_SIZE);
	if(tmpBuffer == NULL){
		err = -1;
		goto onErr;
	}

	tmpBuffer_big = (char *)malloc(CN_READ_SIZE_BIG);
	if(tmpBuffer_big == NULL){
		err = -1;
		goto onErr;
	}

	jobId = (char *)malloc(CN_IVEC_JOBID_LEN);
	if(jobId == NULL){
		err = -1;
		goto onErr;
	}


	memset(jobId, 		'\0', CN_IVEC_JOBID_LEN);
	memset(cmdBuffer, 	'\0', CN_READ_SIZE);
	memset(cmdBuffer_big, 	'\0', CN_READ_SIZE_BIG);
	memset(tmpBuffer, 	'\0', CN_READ_SIZE);
	memset(tmpBuffer_big, 	'\0', CN_READ_SIZE_BIG);
	memset(supportID, 	'\0', CN_IVEC_SUPPORT_CODE_LENGTH);
	memset(stat_msg, 	'\0', CN_STAT_MSG_LEN);
	memset(stat_tmp_msg, 	'\0', CN_STAT_MSG_LEN);


	/*---------------get printer state info-------------------------*/
    int i = 1;
    while(1) {
		if ( if_info->ifType == CNIF_TYPE_USB ){
			read_size = 0;

			err = CNIF_Read(if_info, (uint8_t *)cmdBuffer, CN_READ_SIZE * i, &read_size, CNNET3_FALSE);

#ifdef _DEBUG_MODE_
			fprintf(stderr, "DEBUG: [CNIF_Read] return : %d\n", err);
			fprintf(stderr, "DEBUG: [CNIF_Read] read_size : %ld\n", read_size);
#endif

			if (err == 0) {
				sumsize = read_size;

#ifdef _DEBUG_MODE_
				fprintf(stderr, "-----------------------------cmdBuffer(old)-------------------------\n");
				fprintf(stderr, "%s\n", cmdBuffer);
				fprintf(stderr, "sumsize = %d\n", (int)sumsize);
#endif

				break;
			} else if (err == -8) {

#ifdef _DEBUG_MODE_
				fprintf(stderr, "DEBUG: [CNIF_Read] overflow error\n");
#endif

				i = i + 1;

				cmdBuffer = (char *)realloc(cmdBuffer, i * CN_READ_SIZE);

				if (cmdBuffer == NULL) {
					err = -1;
					goto onErr;
				} else {
					memset( cmdBuffer, '\0', CN_READ_SIZE * i );
				}

				continue;
			} else {
				if (read_size == 0) {
					fprintf(stderr, "INFO: %s\n", LookupText( "LBM_CANT_COMM_PRINT" ));
				}
				goto onErr;
			}
		}
		else if ( if_info->ifType == CNIF_TYPE_NET ){
			read_size = 0;

			err = CNIF_Read(if_info, (uint8_t *)cmdBuffer_big, CN_READ_SIZE_BIG, &read_size, CNNET3_FALSE);

#ifdef _DEBUG_MODE_
			fprintf(stderr, "DEBUG: [CNIF_Read] return : %d\n", err);
			fprintf(stderr, "DEBUG: [CNIF_Read] read_size : %ld\n", read_size);
#endif

			if (err == 0) {
				sumsize = read_size;

#ifdef _DEBUG_MODE_
				fprintf(stderr, "-----------------------------cmdBuffer_big(old)-------------------------\n");
				fprintf(stderr, "%s\n", cmdBuffer_big);
				fprintf(stderr, "sumsize = %d\n", (int)sumsize);
#endif

				break;
			}
			else {
				if (read_size == 0) {
					fprintf(stderr, "INFO: %s\n", LookupText( "LBM_CANT_COMM_PRINT" ));
				}
				goto onErr;
			}
		}
		else if( if_info->ifType == CNIF_TYPE_NET2 ){
		// else if( ( if_info->ifType == CNIF_TYPE_USB && prot == 2 ) || if_info->ifType == CNIF_TYPE_NET2 ){
			err = CNIF_Read(if_info, (uint8_t *)tmpBuffer_big, CN_READ_SIZE_BIG, &read_size, isMaintenance);

#ifdef _DEBUG_MODE_
			fprintf(stderr, "-----------------------------tmpBuffer(new)-------------------------\n");
			fprintf(stderr, "%s\n", tmpBuffer_big);
			fprintf(stderr, "read_size = %d, err = %d\n", (int)read_size, err);
#endif
			cmdBuffer_big = strncat(cmdBuffer_big, tmpBuffer_big, CN_READ_SIZE_BIG);
			sumsize += read_size;

			if (err == 0) {
				break;
			}
			else {
				if (read_size == 0) {
					fprintf(stderr, "INFO: %s\n", LookupText( "LBM_CANT_COMM_PRINT" ));
				}
				goto onErr;
			}
		}
	}

	if ( if_info->ifType == CNIF_TYPE_NET2 ){
	// if ( ( if_info->ifType == CNIF_TYPE_USB && prot == 2 ) || if_info->ifType == CNIF_TYPE_NET2 ){
#ifdef _DEBUG_MODE_
		fprintf(stderr, "DEBUG: Get status\n");
#endif

		char *serviceType = getenv("CONTENT_TYPE");
		char typePtnMaintenance[100];
		memset( typePtnMaintenance, '\0', sizeof(typePtnMaintenance) );

		strncpy( typePtnMaintenance, SVC_MAINTENANCE, strlen(SVC_MAINTENANCE) );

		if( strncmp( serviceType, typePtnMaintenance, sizeof(typePtnMaintenance) )  == 0 &&
            isMaintenance == CNNET3_TRUE ){
			err = GET_STATUS2_MAINTENANCE( cmdBuffer_big, sumsize, uuid, &status, &statusDetail, supportID, _jobID );

#ifdef _DEBUG_MODE_
			fprintf(stderr, "DEBUG: GET_STATUS2_MAINTENANCE : %d\n", err);
#endif
		}
		else{
			err = GET_STATUS2( cmdBuffer_big, sumsize, uuid, &status, &statusDetail, supportID, _jobID );

#ifdef _DEBUG_MODE_
			fprintf(stderr, "DEBUG: GET_STATUS2 : %d\n", err);
#endif
		}


		if( err == CLSS_OK && _jobID[0] != 0x00 ){
			isPrinting = CNMPU2_TRUE;
			job_id = _jobID;

#ifdef _DEBUG_MODE_
			fprintf(stderr, "DEBUG: isPrinting : %d\n", isPrinting);
#endif
		}
		// else if( strncmp( serviceType, SVC_MAINTENANCE, sizeof(SVC_MAINTENANCE) )  == 0 &&
  //                err == CLSS_NOT_SUPPORT &&
  //                pri == CNNET3_TRUE ){
  //           goto onErr;
		// }

		if( supportID != '\0' ) {
			//snprintf(stat_msg, CN_STAT_MSG_LEN - 1, "[support id: %s.] ", supportID);
			snprintf(stat_tmp_msg, CN_STAT_MSG_LEN - 1, "[%s] ", LookupText("LBM_ERROR_CODE"));
			snprintf(stat_msg, CN_STAT_MSG_LEN - 1, stat_tmp_msg, supportID);
		} else {
			/* supportID equel NULL */
		}

		/* check status */
		switch(status) {
			case CN_IVEC_STATUS_IDLE:
#ifdef _DEBUG_MODE_
				fprintf(stderr, "DEBUG: [CNIF_GetResponse] CN_IVEC_STATUS_IDLE\n");
#endif
				fprintf(stderr, "INFO: %s\n", LookupText( "LBM_PRINTING" ));

				// return CN_IVEC_STATUS_IDLE;

				if( err == CLSS_OK ){
					err = CLSS_OK_AND_IDLE;
				}
				else if( err == CLSS_NOT_SUPPORT_CUSTOM ){
					err = CLSS_NOT_SUPP_AND_IDLE;
				}
				else{
					err = CN_IVEC_STATUS_IDLE;
				}

				break;
			case CN_IVEC_STATUS_PROCESSING:
#ifdef _DEBUG_MODE_
				fprintf(stderr, "DEBUG: [CNIF_GetResponse] CN_IVEC_STATUS_PROCESSING\n");
#endif
				fprintf(stderr, "INFO: %s\n", LookupText( "LBM_PRINTING" ));
				break;
			case CN_IVEC_STATUS_STOPPED:
			case CN_IVEC_STATUS_NOTREADY:

#ifdef _DEBUG_MODE_
				fprintf(stderr, "DEBUG: [CNIF_GetResponse] CN_IVEC_STATUS_STOPPED or CN_IVEC_STATUS_NOTREADY\n");
#endif
				/* check status detail */
				switch(statusDetail) {
					case CN_IVEC_STATUS_DETAIL_MEDIA_JAM:
#ifdef _DEBUG_MODE_
						fprintf(stderr, "DEBUG: [CNIF_GetResponse] CN_IVEC_STATUS_DETAIL_MEDIA_JAM\n");
#endif
						fprintf(stderr, "INFO: %s%s\n", stat_msg, LookupText( "LBM_JAM" ));
						break;
					case CN_IVEC_STATUS_DETAIL_DOOR_OPEN:
#ifdef _DEBUG_MODE_
						fprintf(stderr, "DEBUG: [CNIF_GetResponse] CN_IVEC_STATUS_DETAIL_DOOR_OPEN\n");
#endif
						fprintf(stderr, "INFO: %s%s\n", stat_msg, LookupText( "LBM_COVER_OPEN" ));
						break;
					case CN_IVEC_STATUS_DETAIL_MEDIA_EMPTY:
#ifdef _DEBUG_MODE_
						fprintf(stderr, "DEBUG: [CNIF_GetResponse] CN_IVEC_STATUS_DETAIL_MEDIA_EMPTY\n");
#endif
						fprintf(stderr, "INFO: %s%s\n", stat_msg, LookupText( "LBM_PAPER_NOT_SET" ));
						break;
					case CN_IVEC_STATUS_DETAIL_BUSYING:
#ifdef _DEBUG_MODE_
						fprintf(stderr, "DEBUG: [CNIF_GetResponse] CN_IVEC_STATUS_DETAIL_BUSYING\n");
#endif
						//fprintf(stderr, "INFO: %s%s\n", stat_msg, LookupText( "LBM_BUSY" ));
						fprintf(stderr, "INFO: %s\n", LookupText( "LBM_BUSY" ));
						break;
					default:
#ifdef _DEBUG_MODE_
						fprintf(stderr, "DEBUG: [CNIF_GetResponse] LBM_CHK_PRINTER\n");
#endif
						if( supportID != '\0' ){
							fprintf(stderr, "INFO: %s%s\n", stat_msg, LookupText( "LBM_CHK_PRINTER" ));
						}
						break;
				}
				break;
			case CN_IVEC_STATUS_CANCELING:
#ifdef _DEBUG_MODE_
				fprintf(stderr, "DEBUG: [CNIF_GetResponse] CN_IVEC_STATUS_CANCELING\n");
#endif
				// return CN_IVEC_STATUS_CANCELING;

				if( err == CLSS_OK ){
					err = CLSS_OK_AND_CANCEL;
				}
				else if( err == CLSS_NOT_SUPPORT_CUSTOM ){
					err = CLSS_NOT_SUPP_AND_CANCEL;
				}
				else{
					err = CN_IVEC_STATUS_CANCELING;
				}

			default:
				break;
		}
	}
	else if( if_info->ifType == CNIF_TYPE_NET ){
#ifdef _DEBUG_MODE_
			fprintf(stderr, "DEBUG: Check Status\n");
#endif
		err = GET_RESPONSE(cmdBuffer_big, sumsize, &opration_id, jobId, &response_detail);

#ifdef _DEBUG_MODE_
		fprintf(stderr, "CLSS_GetResponse err = %d\n", err);
#endif

		if(err < 0 && err != -5) {
			err = 0;
			goto onErr;
		}

#ifdef _DEBUG_MODE_
		fprintf(stderr, "DEBUG: opration id = %d, jobId = %s, response detail = %d\n", opration_id, jobId, *response_detail);
#endif

		switch(opration_id) {
			case CN_IVEC_START_RESPONSE:

#ifdef _DEBUG_MODE_
				fprintf(stderr, "DEBUG: sjob response = %s\n", jobId);
#endif
				result = CN_IVEC_STARTJOB;
				strncpy(job_id, jobId, CN_IVEC_JOBID_LEN);
				break;
			case CN_IVEC_VENDER_RESPONSE:
		
#ifdef _DEBUG_MODE_
				fprintf(stderr, "DEBUG: vjob response = %s\n", jobId);
#endif
				result = CN_IVEC_VENDERSHIFT;
				strncpy(job_id, jobId, CN_IVEC_JOBID_LEN);
				break;
			case CN_IVEC_END_RESPONSE:

#ifdef _DEBUG_MODE_
				fprintf(stderr, "DEBUG: ejob response = %s\n", jobId);
#endif
				result = CN_IVEC_ENDJOB;
				strncpy(job_id, jobId, CN_IVEC_JOBID_LEN);
				break;
			case CN_IVEC_STATUS_RESPONSE:

#ifdef _DEBUG_MODE_
				fprintf(stderr, "DEBUG: status response\n");
#endif
				//get printer status
				result = GET_STATUS(cmdBuffer_big, sumsize, &status, &statusDetail, supportID);

				if(result < 0) {
					err = 0;
					goto onErr;
				}

				if(*supportID != '\0') {
					//snprintf(stat_msg, CN_STAT_MSG_LEN - 1, "[support id: %s.] ", supportID);
					snprintf(stat_tmp_msg, CN_STAT_MSG_LEN - 1, "[%s] ", LookupText("LBM_ERROR_CODE"));
					snprintf(stat_msg, CN_STAT_MSG_LEN - 1, stat_tmp_msg, supportID);
				} else {
					/* supportID equel NULL */
				}
				/* check status */
				switch(status) {
					case CN_IVEC_STATUS_IDLE:
#ifdef _DEBUG_MODE_
						fprintf(stderr, "DEBUG: [CNIF_GetResponse] CN_IVEC_STATUS_IDLE\n");
#endif
						fprintf(stderr, "INFO: %s\n", LookupText( "LBM_PRINTING" ));
						return CN_IVEC_STATUS_IDLE;
						break;
					case CN_IVEC_STATUS_PROCESSING:
#ifdef _DEBUG_MODE_
						fprintf(stderr, "DEBUG: [CNIF_GetResponse] CN_IVEC_STATUS_PROCESSING\n");
#endif
						fprintf(stderr, "INFO: %s\n", LookupText( "LBM_PRINTING" ));
						break;
					case CN_IVEC_STATUS_STOPPED:
					case CN_IVEC_STATUS_NOTREADY:
						/* check status detail */
						switch(statusDetail) {
							case CN_IVEC_STATUS_DETAIL_MEDIA_JAM:
#ifdef _DEBUG_MODE_
								fprintf(stderr, "DEBUG: [CNIF_GetResponse] CN_IVEC_STATUS_DETAIL_MEDIA_JAM\n");
#endif
								fprintf(stderr, "INFO: %s%s\n", stat_msg, LookupText( "LBM_JAM" ));
								break;
							case CN_IVEC_STATUS_DETAIL_DOOR_OPEN:
#ifdef _DEBUG_MODE_
								fprintf(stderr, "DEBUG: [CNIF_GetResponse] CN_IVEC_STATUS_DETAIL_DOOR_OPEN\n");
#endif
								fprintf(stderr, "INFO: %s%s\n", stat_msg, LookupText( "LBM_COVER_OPEN" ));
								break;
							case CN_IVEC_STATUS_DETAIL_MEDIA_EMPTY:
#ifdef _DEBUG_MODE_
								fprintf(stderr, "DEBUG: [CNIF_GetResponse] CN_IVEC_STATUS_DETAIL_MEDIA_EMPTY\n");
#endif
								fprintf(stderr, "INFO: %s%s\n", stat_msg, LookupText( "LBM_PAPER_NOT_SET" ));
								break;
							case CN_IVEC_STATUS_DETAIL_BUSYING:
#ifdef _DEBUG_MODE_
								fprintf(stderr, "DEBUG: [CNIF_GetResponse] CN_IVEC_STATUS_DETAIL_BUSYING\n");
#endif
								//fprintf(stderr, "INFO: %s%s\n", stat_msg, LookupText( "LBM_BUSY" ));
								fprintf(stderr, "INFO: %s\n", LookupText( "LBM_BUSY" ));
								break;
							default:
								if(*supportID != '\0'){
									fprintf(stderr, "INFO: %s%s\n", stat_msg, LookupText( "LBM_CHK_PRINTER" ));
								}
								break;
						}
						break;
					case CN_IVEC_STATUS_CANCELING:
#ifdef _DEBUG_MODE_
						fprintf(stderr, "DEBUG: [CNIF_GetResponse] CN_IVEC_STATUS_CANCELING\n");
#endif
						return CN_IVEC_STATUS_CANCELING;
					default:
						break;
				}
				result = CN_IVEC_STATUS;
				strncpy(job_id, dum_id, CN_IVEC_JOBID_LEN);
				break;
			default:

#ifdef _DEBUG_MODE_
				fprintf(stderr, "DEBUG: ~~~default~~~\n");
#endif
				break;
		}

		err = result;
	}
	else if( if_info->ifType == CNIF_TYPE_USB ){
#ifdef _DEBUG_MODE_
			fprintf(stderr, "DEBUG: Check Status\n");
#endif
		err = GET_RESPONSE(cmdBuffer, sumsize, &opration_id, jobId, &response_detail);

#ifdef _DEBUG_MODE_
		fprintf(stderr, "CLSS_GetResponse err = %d\n", err);
#endif

		if(err < 0 && err != -5) {
			err = 0;
			goto onErr;
		}

#ifdef _DEBUG_MODE_
		fprintf(stderr, "DEBUG: opration id = %d, jobId = %s, response detail = %d\n", opration_id, jobId, *response_detail);
#endif

		switch(opration_id) {
			case CN_IVEC_START_RESPONSE:

#ifdef _DEBUG_MODE_
				fprintf(stderr, "DEBUG: sjob response = %s\n", jobId);
#endif
				result = CN_IVEC_STARTJOB;
				strncpy(job_id, jobId, CN_IVEC_JOBID_LEN);
				break;
			case CN_IVEC_VENDER_RESPONSE:
		
#ifdef _DEBUG_MODE_
				fprintf(stderr, "DEBUG: vjob response = %s\n", jobId);
#endif
				result = CN_IVEC_VENDERSHIFT;
				strncpy(job_id, jobId, CN_IVEC_JOBID_LEN);
				break;
			case CN_IVEC_END_RESPONSE:

#ifdef _DEBUG_MODE_
				fprintf(stderr, "DEBUG: ejob response = %s\n", jobId);
#endif
				result = CN_IVEC_ENDJOB;
				strncpy(job_id, jobId, CN_IVEC_JOBID_LEN);
				break;
			case CN_IVEC_STATUS_RESPONSE:

#ifdef _DEBUG_MODE_
				fprintf(stderr, "DEBUG: status response\n");
#endif
				//get printer status
				result = GET_STATUS(cmdBuffer, sumsize, &status, &statusDetail, supportID);

				if(result < 0) {
					err = 0;
					goto onErr;
				}

				if(*supportID != '\0') {
					//snprintf(stat_msg, CN_STAT_MSG_LEN - 1, "[support id: %s.] ", supportID);
					snprintf(stat_tmp_msg, CN_STAT_MSG_LEN - 1, "[%s] ", LookupText("LBM_ERROR_CODE"));
					snprintf(stat_msg, CN_STAT_MSG_LEN - 1, stat_tmp_msg, supportID);
				} else {
					/* supportID equel NULL */
				}
				/* check status */
				switch(status) {
					case CN_IVEC_STATUS_IDLE:
#ifdef _DEBUG_MODE_
						fprintf(stderr, "DEBUG: [CNIF_GetResponse] CN_IVEC_STATUS_IDLE\n");
#endif
						fprintf(stderr, "INFO: %s\n", LookupText( "LBM_PRINTING" ));
						return CN_IVEC_STATUS_IDLE;
						break;
					case CN_IVEC_STATUS_PROCESSING:
#ifdef _DEBUG_MODE_
						fprintf(stderr, "DEBUG: [CNIF_GetResponse] CN_IVEC_STATUS_PROCESSING\n");
#endif
						fprintf(stderr, "INFO: %s\n", LookupText( "LBM_PRINTING" ));
						break;
					case CN_IVEC_STATUS_STOPPED:
					case CN_IVEC_STATUS_NOTREADY:
						/* check status detail */
						switch(statusDetail) {
							case CN_IVEC_STATUS_DETAIL_MEDIA_JAM:
#ifdef _DEBUG_MODE_
								fprintf(stderr, "DEBUG: [CNIF_GetResponse] CN_IVEC_STATUS_DETAIL_MEDIA_JAM\n");
#endif
								fprintf(stderr, "INFO: %s%s\n", stat_msg, LookupText( "LBM_JAM" ));
								break;
							case CN_IVEC_STATUS_DETAIL_DOOR_OPEN:
#ifdef _DEBUG_MODE_
								fprintf(stderr, "DEBUG: [CNIF_GetResponse] CN_IVEC_STATUS_DETAIL_DOOR_OPEN\n");
#endif
								fprintf(stderr, "INFO: %s%s\n", stat_msg, LookupText( "LBM_COVER_OPEN" ));
								break;
							case CN_IVEC_STATUS_DETAIL_MEDIA_EMPTY:
#ifdef _DEBUG_MODE_
								fprintf(stderr, "DEBUG: [CNIF_GetResponse] CN_IVEC_STATUS_DETAIL_MEDIA_EMPTY\n");
#endif
								fprintf(stderr, "INFO: %s%s\n", stat_msg, LookupText( "LBM_PAPER_NOT_SET" ));
								break;
							case CN_IVEC_STATUS_DETAIL_BUSYING:
#ifdef _DEBUG_MODE_
								fprintf(stderr, "DEBUG: [CNIF_GetResponse] CN_IVEC_STATUS_DETAIL_BUSYING\n");
#endif
								//fprintf(stderr, "INFO: %s%s\n", stat_msg, LookupText( "LBM_BUSY" ));
								fprintf(stderr, "INFO: %s\n", LookupText( "LBM_BUSY" ));
								break;
							default:
								if(*supportID != '\0'){
									fprintf(stderr, "INFO: %s%s\n", stat_msg, LookupText( "LBM_CHK_PRINTER" ));
								}
								break;
						}
						break;
					case CN_IVEC_STATUS_CANCELING:
#ifdef _DEBUG_MODE_
						fprintf(stderr, "DEBUG: [CNIF_GetResponse] CN_IVEC_STATUS_CANCELING\n");
#endif
						return CN_IVEC_STATUS_CANCELING;
					default:
						break;
				}
				result = CN_IVEC_STATUS;
				strncpy(job_id, dum_id, CN_IVEC_JOBID_LEN);
				break;
			default:

#ifdef _DEBUG_MODE_
				fprintf(stderr, "DEBUG: ~~~default~~~\n");
#endif
				break;
		}

		err = result;
	}


onErr:
	free(jobId);
	free(cmdBuffer);
	free(tmpBuffer);
	free(cmdBuffer_big);
	free(tmpBuffer_big);
	free(supportID);
	free(stat_msg);
	free(stat_tmp_msg);

	return err;
}

