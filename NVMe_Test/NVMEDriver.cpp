////////////////////////////////////////////////////////////////////////////
//
//		Ahsan Uddin
//		© Sunnahwalker Productions 2017
//		Windows NVMe App to check Drive information
//		App is based on Microsft WIndows Windows NVMe Drivers
//		Version 1.01
//		3/12/2017
//		For questions nor concerns please conatct me at ahsan89@gmail.com
//
/////////////////////////////////////////////////////////////////////////////


#include <iostream>
#include <stdio.h>
#include <Windows.h>
#include <assert.h>
#include <string>

// Includes NVMe driver specific libraries
#include <WinIoCtl.h>
#include <Nvme.h>
#include <Ntddstor.h>

//Local Pre Processors
//#define DEBUG_PRINTS_2
#define MAX_DEVICE 32

#define IDFY_PAYLOAD	(sizeof(NVME_IDENTIFY_CONTROLLER_DATA))		// 4KB payload
#define SMART_PAYLOAD	(sizeof(NVME_HEALTH_INFO_LOG))				// 512B payload

HANDLE NVMe_Drive;

using namespace std;

//	This API probes in from 0 to MAX_DEVICE physical slots and checks for drives
//
//	In:	Nothing
//	Out: If a valid drive is found, it will return TRUE, or else FALSE
//
BOOL find_port() {

	TCHAR port[64];
	LPTSTR line = "\\\\.\\PhysicalDrive";
	LPCTSTR format = "%s%d";

	for (int i = 0; i < MAX_DEVICE; i++) {
		wsprintf(port, format, line, i);

		NVMe_Drive = CreateFile(port,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);

		if (NVMe_Drive == INVALID_HANDLE_VALUE) {
			if (GetLastError() == ERROR_ACCESS_DENIED) {
				printf("Access is denied. Please run in Admin mode!!!...\n\n");;
			}
#if defined DEBUG_PRINTS_2
			else {
				printf("Failed to get handle! Port: %d Error: %d\n", i, GetLastError());
			}
#endif
		}
		else {
			printf("NVMe drive found in port: %d\n", i);
			return TRUE;
			//break;
		}
	}

	//If we reach here that means we couldn't find any valid drives!
	return FALSE;
}

BOOL Close_Handle() {
	assert(NVMe_Drive != NULL);

	BOOL ret = FALSE;
	ret = (CloseHandle(NVMe_Drive));
	printf("Handle Closing: %s\n", ((ret == TRUE) ? "SUCCESSFUL" : "FAILED"));
	return ret;
}

BOOL Do_IDFY() {
	BOOL    result = FALSE;
	PVOID   buffer = NULL;
	ULONG   bufferLength = 0;
	ULONG   returnedLength = 0;

	PSTORAGE_PROPERTY_QUERY query = NULL;
	PSTORAGE_PROTOCOL_SPECIFIC_DATA protocolData = NULL;
	PSTORAGE_PROTOCOL_DATA_DESCRIPTOR protocolDataDescr = NULL;

	//
	// Allocate buffer for use.
	//
	bufferLength = FIELD_OFFSET(STORAGE_PROPERTY_QUERY, AdditionalParameters) + sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA) + IDFY_PAYLOAD;
	buffer = malloc(bufferLength);

	if (buffer == NULL) {
		cout << "DeviceNVMeQueryProtocolDataTest: allocate buffer failed, exit.\n" << endl;
		return result;
	}

	//
	// Initialize query data structure to get Identify Controller Data.
	//
	ZeroMemory(buffer, bufferLength);

	query = (PSTORAGE_PROPERTY_QUERY)buffer;
	protocolDataDescr = (PSTORAGE_PROTOCOL_DATA_DESCRIPTOR)buffer;
	protocolData = (PSTORAGE_PROTOCOL_SPECIFIC_DATA)query->AdditionalParameters;

	query->PropertyId = StorageAdapterProtocolSpecificProperty;
	query->QueryType = PropertyStandardQuery;

	protocolData->ProtocolType = ProtocolTypeNvme;
	protocolData->DataType = NVMeDataTypeIdentify;
	protocolData->ProtocolDataRequestValue = NVME_IDENTIFY_CNS_CONTROLLER;
	protocolData->ProtocolDataRequestSubValue = 0;
	protocolData->ProtocolDataOffset = sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA);
	protocolData->ProtocolDataLength = IDFY_PAYLOAD;

	//
	// Send request down.
	//
	result = DeviceIoControl(NVMe_Drive,
		IOCTL_STORAGE_QUERY_PROPERTY,
		buffer,
		bufferLength,
		buffer,
		bufferLength,
		&returnedLength,
		NULL
	);

	//
	// Validate the returned data.
	//
	if ((protocolDataDescr->Version != sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR)) ||
		(protocolDataDescr->Size != sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR))) {
		cout << "DeviceNVMeQueryProtocolDataTest: Get Identify Controller Data - data descriptor header not valid.\n" << endl;
		return result;
	}

	protocolData = &protocolDataDescr->ProtocolSpecificData;

	if ((protocolData->ProtocolDataOffset < sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA)) ||
		(protocolData->ProtocolDataLength < IDFY_PAYLOAD)) {
		cout << "DeviceNVMeQueryProtocolDataTest: Get Identify Controller Data - ProtocolData Offset/Length not valid.\n" << endl;
		return result;
	}

	//
	// Identify Controller Data Parsing
	//
	{
		PNVME_IDENTIFY_CONTROLLER_DATA identifyControllerData = (PNVME_IDENTIFY_CONTROLLER_DATA)((PCHAR)protocolData + protocolData->ProtocolDataOffset);

		if ((identifyControllerData->VID == 0) ||
			(identifyControllerData->NN == 0)) {
			cout << "DeviceNVMeQueryProtocolDataTest: Identify Controller Data not valid.\n" << endl;

			return result;
		}
		else {
			//cout << "DeviceNVMeQueryProtocolDataTest: ***Identify Controller Data succeeded***.\n" << endl;

			UCHAR SN[sizeof(identifyControllerData->SN) + 1];
			SN[sizeof(identifyControllerData->SN)] = '\0';
			UCHAR MN[sizeof(identifyControllerData->MN) + 1];
			MN[sizeof(identifyControllerData->MN)] = '\0';
			UCHAR FW[sizeof(identifyControllerData->FR) + 1];
			FW[sizeof(identifyControllerData->FR)] = '\0';

			memcpy(&SN, &(identifyControllerData->SN), sizeof(identifyControllerData->SN));
			memcpy(&MN, &(identifyControllerData->MN), sizeof(identifyControllerData->MN));
			memcpy(&FW, &(identifyControllerData->FR), sizeof(identifyControllerData->FR));

			cout << "Serial Number: \t" << SN << endl;
			cout << "Model Number: \t" << MN << endl;
			cout << "FW Rev: \t" << FW << endl;

			cout << endl;

			//result = true;
		}
	}

	result = true;			// If code reaches here it means we oassed all the steps!
	return result;
}

BOOL Do_SMART() {

	BOOL    result = FALSE;
	PVOID   buffer = NULL;
	ULONG   bufferLength = 0;
	ULONG   returnedLength = 0;

	PSTORAGE_PROPERTY_QUERY query = NULL;
	PSTORAGE_PROTOCOL_SPECIFIC_DATA protocolData = NULL;
	PSTORAGE_PROTOCOL_DATA_DESCRIPTOR protocolDataDescr = NULL;

	//
	// Allocate buffer for use.
	//
	bufferLength = FIELD_OFFSET(STORAGE_PROPERTY_QUERY, AdditionalParameters) + sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA) + IDFY_PAYLOAD;
	buffer = malloc(bufferLength);

	if (buffer == NULL) {
		cout << "DeviceNVMeQueryProtocolDataTest: allocate buffer failed, exit.\n" << endl;
		return result;
	}

	ZeroMemory(buffer, bufferLength);
	query = (PSTORAGE_PROPERTY_QUERY)buffer;
	protocolDataDescr = (PSTORAGE_PROTOCOL_DATA_DESCRIPTOR)buffer;
	protocolData = (PSTORAGE_PROTOCOL_SPECIFIC_DATA)query->AdditionalParameters;

	query->PropertyId = StorageDeviceProtocolSpecificProperty;
	query->QueryType = PropertyStandardQuery;

	protocolData->ProtocolType = ProtocolTypeNvme;
	protocolData->DataType = NVMeDataTypeLogPage;
	protocolData->ProtocolDataRequestValue = NVME_LOG_PAGE_HEALTH_INFO;
	protocolData->ProtocolDataRequestSubValue = 0;
	protocolData->ProtocolDataOffset = sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA);
	protocolData->ProtocolDataLength = sizeof(NVME_HEALTH_INFO_LOG);

	//  
	// Send request down.  
	//  
	result = DeviceIoControl(NVMe_Drive,
		IOCTL_STORAGE_QUERY_PROPERTY,
		buffer,
		bufferLength,
		buffer,
		bufferLength,
		&returnedLength,
		NULL
	);

	//
	// Validate the returned data.
	//
	if ((protocolDataDescr->Version != sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR)) ||
		(protocolDataDescr->Size != sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR))) {
		cout << "DeviceNVMeQueryProtocolDataTest: Get SMART Data - data descriptor header not valid.\n" << endl;
		return result;
	}

	protocolData = &protocolDataDescr->ProtocolSpecificData;

	if ((protocolData->ProtocolDataOffset < sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA)) ||
		(protocolData->ProtocolDataLength < SMART_PAYLOAD)) {
		cout << "DeviceNVMeQueryProtocolDataTest: Get SMART Data - ProtocolData Offset/Length not valid.\n" << endl;
		return result;
	}

	//
	// SMART Health Data Pasing
	//
	{
		PNVME_HEALTH_INFO_LOG SMARTHealthData = (PNVME_HEALTH_INFO_LOG)((PCHAR)protocolData + protocolData->ProtocolDataOffset);

		uint8_t  *PercentageUsed = (uint8_t *)&(SMARTHealthData->PercentageUsed);
		uint16_t *PowerOnHours = (uint16_t *)&(SMARTHealthData->PowerOnHours);
		uint16_t *PowerCycle = (uint16_t *)&(SMARTHealthData->PowerCycle);
		uint16_t *UnsafeShutdowns = (uint16_t *)&(SMARTHealthData->UnsafeShutdowns);
		uint16_t *DataUnitWritten = (uint16_t *)&(SMARTHealthData->DataUnitWritten);
		uint16_t *DataUnitRead = (uint16_t *)&(SMARTHealthData->DataUnitRead);
		
		cout << "SMART Data: Percentage Used = "		<< (int)*PercentageUsed << "%" << endl;
		cout << "SMART Data: Power ON Hours  = "		<< (int)*PowerOnHours << endl;
		cout << "SMART Data: PowerCycle  = "			<< (int)*PowerCycle << endl;
		cout << "SMART Data: Ungraceful Shutdown  = "	<< (int)*UnsafeShutdowns << endl;

		cout << "SMART Data: Data Units Written  = " << (unsigned int)*DataUnitWritten	<< " (" << ((((unsigned int)*DataUnitWritten) * 1000 * 512) / (1024 * 1024 * 1024)) << "GB)" << endl;
		cout << "SMART Data: Data Units Read     = " << (unsigned int)*DataUnitRead		<< " (" << ((((unsigned int)*DataUnitRead)	  * 1000 * 512) / (1024 * 1024 * 1024)) << "GB)" << endl;

		cout << endl;

	}

	result = true;
	return result;
}