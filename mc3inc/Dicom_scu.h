#pragma once
/*
* Standard OS Includes
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <fcntl.h>
#endif
/*
* Merge DICOM Toolkit Includes
*/
#include "mc3media.h"
#include "mc3msg.h"
#include "mergecom.h"
#include "diction.h"
#include "mc3services.h"
#include "mc3items.h"

#include "general_util.h"

/*
* Module constants
*/

/* DICOM VR Lengths */
#define AE_LENGTH 16
#define UI_LENGTH 64
#define SVC_LENGTH 130
#define STR_LENGTH 100
#define WORK_SIZE (64*1024)

#define TIME_OUT 30

#if defined(_WIN32)
#define BINARY_READ "rb"
#define BINARY_WRITE "wb"
#define BINARY_APPEND "rb+"
#define BINARY_READ_APPEND "a+b"
#define BINARY_CREATE "w+b"
#define TEXT_READ "r"
#define TEXT_WRITE "w"
#else
#define BINARY_READ "r"
#define BINARY_WRITE "w"
#define BINARY_APPEND "r+"
#define BINARY_READ_APPEND "a+"
#define BINARY_CREATE "w+"
#define TEXT_READ "r"
#define TEXT_WRITE "w"
#endif

/*
* Structure to store local application information
*/
typedef struct stor_scu_options
{
    int     StartImage;
    int     StopImage;
    int     ListenPort; /* for StorageCommit */
    int     RemotePort;

    char    RemoteAE[AE_LENGTH + 2];
    char    LocalAE[AE_LENGTH + 2];
    char    RemoteHostname[STR_LENGTH];
    char    ServiceList[SVC_LENGTH + 2];
    char    FileList[1024];
    char    Username[STR_LENGTH];
    char    Password[STR_LENGTH];

    SAMP_BOOLEAN UseFileList;
    SAMP_BOOLEAN Verbose;
    SAMP_BOOLEAN StorageCommit;
    SAMP_BOOLEAN ResponseRequested;
    SAMP_BOOLEAN StreamMode;

    AssocInfo       asscInfo;
} STORAGE_OPTIONS;

typedef enum
{
    UNKNOWN_FORMAT = 0,
    MEDIA_FORMAT = 1,
    IMPLICIT_LITTLE_ENDIAN_FORMAT,
    IMPLICIT_BIG_ENDIAN_FORMAT,
    EXPLICIT_LITTLE_ENDIAN_FORMAT,
    EXPLICIT_BIG_ENDIAN_FORMAT
} FORMAT_ENUM;
/*
* Structure to maintain list of instances sent & to be sent.
* The structure keeps track of all instances and is used
* in a linked list.
*/
typedef struct instance_node
{
    int    msgID;                       /* messageID of for this node */
    char   fname[1024];                 /* Name of file */
    TRANSFER_SYNTAX transferSyntax;     /* Transfer syntax of file */

    char   SOPClassUID[UI_LENGTH + 2];    /* SOP Class UID of the file */
    char   serviceName[48];             /* Merge DICOM Toolkit service name for SOP Class */
    char   SOPInstanceUID[UI_LENGTH + 2]; /* SOP Instance UID of the file */

    size_t       imageBytes;            /* size in bytes of the file */

    unsigned int dicomMsgID;            /* DICOM Message ID in group 0x0000 elements */
    unsigned int status;                /* DICOM status value returned for this file. */
    char   statusMeaning[STR_LENGTH];   /* Textual meaning of "status" */
    SAMP_BOOLEAN responseReceived;      /* Bool indicating we've received a response for a sent file */
    SAMP_BOOLEAN failedResponse;        /* Bool saying if a failure response message was received */
    SAMP_BOOLEAN imageSent;             /* Bool saying if the image has been sent over the association yet */
    SAMP_BOOLEAN mediaFormat;           /* Bool saying if the image was originally in media format (Part 10) */

    struct instance_node* Next;         /* Pointer to next node in list */

} InstanceNode;

/*structure for storing the patient name, patient id, sop id*/

typedef struct patient_info
{
    char* patient_name;
    char* patient_id;
    char* SOPinstanceID;
}Patient_info;

/*
* CBinfo is used to callback functions when reading in stream objects
* and Part 10 format objects.
*/
typedef struct CALLBACKINFO
{
    FILE* fp;
    char    fileName[512];
    /*
    * Note! The size of this buffer impacts toolkit performance.
    *       Higher values in general should result in increased performance of reading files
    */
    size_t  bytesRead;
    size_t  bufferLength;

    char* buffer;
} CBinfo;

int main(int argc, char** argv);
void CheckMCLibraryInitialization(MC_STATUS mcStatus);
void CheckMCRegisterApplication(MC_STATUS mcStatus, STORAGE_OPTIONS options);
void AddInstancesToFile(STORAGE_OPTIONS options, InstanceNode** instanceList, char fname[]);
void CheckMCOpenAssociation(STORAGE_OPTIONS options, MC_STATUS mcStatus);
void CheckMCGetAssociationInfo(MC_STATUS mcStatus);
void TraverseInstanceListNSend(STORAGE_OPTIONS options, InstanceNode** node, int applicationID, int associationID, int totalImages, Patient_info* patientinfo);
void ReadImageCheckOpen(SAMP_BOOLEAN sampBool, InstanceNode* node);
void SendImageCheck(SAMP_BOOLEAN sampBool, InstanceNode* node, int associationID, int applicationID);
void UpdateNodeCheck(SAMP_BOOLEAN sampBool, InstanceNode* node, int associationID, int applicationID);
void ImageSentCount(InstanceNode* node);
void CheckMCFreeMessage(MC_STATUS mcStatus);
void CheckMCCloseAssociation(MC_STATUS mcStatus, int associationID);
void CheckMCReleaseApplication(MC_STATUS mcStatus);
void ReleaseMemory();
void PrintCmdLine(void);
SAMP_BOOLEAN TestCmdLine(int A_argc, char* A_argv[], STORAGE_OPTIONS* A_options, Patient_info* patientinfo);
void Copy_ServiceList(STORAGE_OPTIONS* A_options);
SAMP_BOOLEAN CheckHostDetails(STORAGE_OPTIONS* A_options);
void CheckcmdArgs(int A_argc, char* A_argv[], STORAGE_OPTIONS* A_options, Patient_info* patientinfo);
int checkcmdArgsHostDetails(char* A_argv[], int i, STORAGE_OPTIONS* A_options, Patient_info* patientinfo);
int CheckcmdArgsPatientDetails(char* A_argv[], int i, STORAGE_OPTIONS* A_options, Patient_info* patientinfo);
int CheckPatientSOP(char* A_argv[], int i, STORAGE_OPTIONS* A_options, Patient_info* patientinfo);
void  CheckImageDetails(char* A_argv[], int i, STORAGE_OPTIONS* A_options);
void  CheckcmdArgsStopImage(char* A_argv[], int i, STORAGE_OPTIONS* A_options);
SAMP_BOOLEAN AddFileToList(InstanceNode** A_list, char* A_fname);
SAMP_BOOLEAN AddFileToListInstanceInfo(InstanceNode** A_list, char* A_fname, InstanceNode* newNode);
SAMP_BOOLEAN UpdateNode(InstanceNode* A_node);
void FreeList(InstanceNode** A_list);
int GetNumNodes(InstanceNode* A_list);
SAMP_BOOLEAN ReadImage(STORAGE_OPTIONS* A_options, int A_appID, InstanceNode* A_node, Patient_info* patientinfo);
SAMP_BOOLEAN ReadImageChangeID(InstanceNode* A_node);
SAMP_BOOLEAN SendImage(STORAGE_OPTIONS* A_options, int A_associationID, InstanceNode* A_node);
SAMP_BOOLEAN SendImageSetSOPInstanceUID(MC_STATUS mcStatus);
SAMP_BOOLEAN SendImageRequestMessage(STORAGE_OPTIONS* A_options, int A_associationID, InstanceNode* A_node);
SAMP_BOOLEAN SendImageCheckNormal(int A_associationID, InstanceNode* A_node);
SAMP_BOOLEAN ChangePatientInfo(InstanceNode* A_node, Patient_info* patientinfo);
SAMP_BOOLEAN changePatientSOP(InstanceNode* A_node, Patient_info* patientinfo);
MC_STATUS NOEXP_FUNC StreamToMsgObj(int AmsgID, void* AcBinformation, int AfirstCall, int* AdataLen, void** AdataBuffer, int* AisLast);
int StreamToMsgObj1(CBinfo* callbackInfo);
void PrintError(char* A_string, MC_STATUS A_status);
SAMP_BOOLEAN ReadMessageFromFile(STORAGE_OPTIONS* A_options,
    char* A_fileName,
    FORMAT_ENUM         A_format,
    int* A_msgID,
    TRANSFER_SYNTAX* A_syntax,
    size_t* A_bytesRead);

SAMP_BOOLEAN ReadMessageFromFileEmptyMessage(int* A_msgID);
SAMP_BOOLEAN ReadMessageFromFileOpenNStream(char* A_filename, int* A_msgID);
SAMP_BOOLEAN ReadMessageFromFileBufferAllocate(CBinfo callbackInfo);
SAMP_BOOLEAN ReadMessageFromFileStreamError(MC_STATUS mcStatus, int* A_msgID);
void ReadMessageFromFileClose(CBinfo callbackInfo);
