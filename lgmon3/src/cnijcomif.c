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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/time.h>
#include <libusb.h>

#include "cnijlgmon3.h"
#include "cnijcomif.h"
#include "cnijifnet.h"
#include "cnijifusb.h"
#include "cnijifnet2.h"
#include "./common/libcnnet.h"

// #define _DEBUG_MODE_


int CNIF_Open(const char *_deviceID, CNIF_INFO *_if_info)
{
	switch (_if_info->ifType)
	{
		case CNIF_TYPE_USB:
			return CNIF_USB_Open(_deviceID, _if_info);
		case CNIF_TYPE_NET:
			return CNIF_Network_Open(_deviceID, _if_info);
		case CNIF_TYPE_NET2:
			return CNIF_Network2_Open(_deviceID);

		default:
			break;
	}
	return CN_LGMON_OK;
}

int CNIF_StartSession(CNIF_INFO *_if_info)
{
	switch (_if_info->ifType)
	{
		case CNIF_TYPE_USB:
			return CN_LGMON_OK;
		case CNIF_TYPE_NET:
			return CNIF_Network_StartSession();
		case CNIF_TYPE_NET2:
			return CNIF_Network2_StartSession();
		default:
			break;
	}
	return CN_LGMON_OK;
}

int CNIF_Close(CNIF_INFO *_if_info)
{
	switch (_if_info->ifType)
	{
		case CNIF_TYPE_USB:
			return CNIF_USB_Close();
		case CNIF_TYPE_NET:
			return CNIF_Network_Close();
		case CNIF_TYPE_NET2:
			return CNIF_Network2_EndSession();
		default:
			break;
	}
	return CN_LGMON_OK;
}

int CNIF_Reset(CNIF_INFO *_if_info)
{
	switch (_if_info->ifType)
	{
		case CNIF_TYPE_USB:
			return CNIF_USB_Reset();
		case CNIF_TYPE_NET:
			return CNIF_Network_Reset();
		default:
			break;
	}
	return CN_LGMON_OK;
}

int CNIF_Read(CNIF_INFO *_if_info, uint8_t *buffer, size_t bufferSize, size_t *readSize, int isPrinting)
{
	switch (_if_info->ifType)
	{
		case CNIF_TYPE_USB:
			return CNIF_USB_Read(buffer, bufferSize, readSize);
		case CNIF_TYPE_NET:
			return CNIF_Network_Read(buffer, bufferSize, readSize);
		case CNIF_TYPE_NET2:
			return CNIF_Network2_ReadStatusPrint(buffer, bufferSize, readSize, isPrinting);
		default:
			break;
	}
	return CN_LGMON_OK;
}

int CNIF_Write(CNIF_INFO *_if_info, uint8_t *buffer, size_t bufferSize, size_t *writtenSize)
{
	int err = 0;
	switch (_if_info->ifType)
	{
		case CNIF_TYPE_USB:
			err = CNIF_USB_Write(buffer, bufferSize, writtenSize);
			if (err == LIBUSB_ERROR_TIMEOUT) {
				err = 0;
				
#ifdef _DEBUG_MODE_
				fprintf(stderr, "DEBUG: Data send time out(%d). transfered_data = %d\n", err, (int) *writtenSize);//ERROR_MSG
#endif
				
				if(bufferSize == *writtenSize) {
					*writtenSize = 0;
				}
			}
			break;
		case CNIF_TYPE_NET:
			return CNIF_Network_Write(buffer, bufferSize, writtenSize);
		case CNIF_TYPE_NET2:
			return CNIF_Network2_SendData(buffer, bufferSize, writtenSize);
		default:
			break;
	}
	return err;
}

int CNIF_Send(CNIF_INFO *_if_info, uint8_t *buffer, size_t bufferSize, size_t *writtenSize)
{
	int err = 0;

	switch (_if_info->ifType)
	{
		case CNIF_TYPE_NET2:
			return CNIF_Network2_SendData(buffer, bufferSize, writtenSize);
		default:
			break;
	}

	return err;
}

int CNIF_GetSerialNum(CNIF_INFO *_if_info, uint8_t *buffer, size_t bufferSize, char *serial_num) {
	char keyword[] = "serial";
	char *tp = NULL;
	char *head = NULL;
	char *name = NULL;
	char *value = NULL;
	char *dev_uri = NULL;
	
	head = (char *)malloc(64);
	if(head == NULL){
		goto onErr;
	}
	
	name = (char *)malloc(20);
	if(name == NULL){
		goto onErr;
	}
	
	value = (char *)malloc(32);
	if(value == NULL){
		goto onErr;
	}
	
	dev_uri = (char *)malloc(CN_DEVICE_URI_LEN);
	if(dev_uri == NULL){
		goto onErr;
	}
	
	memset(head, 0, 64);
	memset(name, 0, 20);
	memset(value, 0, 32);
	strncpy(dev_uri, (const char *)buffer, CN_DEVICE_URI_LEN);
	
	sscanf(dev_uri, "cnijbe2://Canon/?%63s", head);
	
	tp = strtok(head, "&");
	if(tp != NULL) {
		sscanf(tp, "%19[a-z]=%31[0-9a-zA-Z%-]", name, value);
		if(!strncmp(name, "port", 20)){
			if(!strcmp(value, "net")){
				_if_info->ifType = CNIF_TYPE_NET;
			}
			if(!strcmp(value, "usb")){
				_if_info->ifType = CNIF_TYPE_USB;
			}
		} else {
			goto onErr;
		}
	}
	while(tp != NULL){
		tp = strtok(NULL, "&");
		if(tp != NULL) {
			memset(name, 0, 20);
			memset(value, 0, 32);
			sscanf(tp, "%19[a-z]=%31[0-9a-zA-Z%-]", name, serial_num);
			if(!strncmp(name, keyword, 20)){
				return CN_LGMON_OK;
			}
		}
	}
	
onErr:
	return CN_LGMON_ERROR;
}

int CNIF_Discover(CNIF_INFO *_if_info, int installer)
{
	int err_usb = 0, err_net = 0, err_net2 = 0;
	
	if(installer == 0) {
		
#ifdef _DEBUG_MODE_
		fprintf(stderr, "DEBUG: CNIF_Discover normal mode\n");
#endif

		err_usb = CNIF_USB_Discover();
		err_net = CNIF_Network_Discover(installer);
		err_net2 = CNIF_Network2_Discover(installer);

		if (err_usb < 0) {
			return err_usb;
		}
		if (err_net < 0) {
			return err_net;
		}
		if (err_net2 < 0) {
			return err_net2;
		}
	} else {

#ifdef _DEBUG_MODE_
		fprintf(stderr, "DEBUG: CNIF_Discover installer mode\n");
#endif

		switch (_if_info->ifType)
		{
			case CNIF_TYPE_USB:
				err_usb = CNIF_USB_Discover();
				if (err_usb < 0) {
					return err_usb;
				}
				break;
			case CNIF_TYPE_NET:
				err_net = CNIF_Network_Discover(installer);
				if (err_net < 0) {
					return err_net;
				}
				break;
			case CNIF_TYPE_NET2:
				err_net2 = CNIF_Network2_Discover(installer);
				if (err_net2 < 0) {
					return err_net2;
				}
				break;
			default:
				break;
		}
	}
	return CN_LGMON_OK;
}

int CNIF_Cancel(CNIF_INFO *_if_info)
{
	switch (_if_info->ifType)
	{
		case CNIF_TYPE_USB:
			return CNIF_USB_Cancel();
		case CNIF_TYPE_NET:
			return CNIF_Network_Cancel();
		case CNIF_TYPE_NET2:
			return CNIF_Network2_CancelPrint("00000002");
		default:
			break;
	}
	return CN_LGMON_OK;
}

int CNIF_KeepSession(CNIF_INFO *_if_info)
{
	switch (_if_info->ifType)
	{
		case CNIF_TYPE_USB:
			break;
		case CNIF_TYPE_NET:
			break;
		case CNIF_TYPE_NET2:
			return CNIF_Network2_SendDummyData();
		default:
			break;
	}

	return CN_LGMON_OK;
}
