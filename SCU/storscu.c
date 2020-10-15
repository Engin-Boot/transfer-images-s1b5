#include "Dicom_scu.h"
/****************************************************************************
*
*  Function    :   Main
*
*  Description :   Main routine for DICOM Storage Service Class SCU
*
****************************************************************************/
int  main(int argc, char** argv);
int main(int argc, char** argv)
{
    SAMP_BOOLEAN            sampBool;
    STORAGE_OPTIONS         options;
    MC_STATUS               mcStatus;
    int                     applicationID = -1, associationID = -1;
    int                     totalImages = 0L;
    double                  seconds = 0.0;
    void* startTime = NULL, *imageStartTime = NULL;
    char                    fname[512] = { 0 };  /* Extra long, just in case */
                                                 //    ServiceInfo             servInfo;
    size_t                  totalBytesRead = 0L;
    InstanceNode* instanceList = NULL, *node = NULL;
    Patient_info            patientinfo;
    FILE* fp = NULL;
    /*
    * Test the command line parameters, and populate the options
    * structure with these parameters
    */
    sampBool = TestCmdLine(argc, argv, &options, &patientinfo);
    if (sampBool == SAMP_FALSE)
    {
        return(EXIT_FAILURE);
    }
    /* ------------------------------------------------------- */
    /* This call MUST be the first call made to the library!!! */
    /* ------------------------------------------------------- */
    mcStatus = MC_Library_Initialization(NULL, NULL, NULL);
    CheckMCLibraryInitialization(mcStatus);

    /*
    *  Register this DICOM application
    */
    mcStatus = MC_Register_Application(&applicationID, options.LocalAE);
    CheckMCRegisterApplication(mcStatus, options);

    AddInstancesToFile(options, &instanceList, fname);

    totalImages = GetNumNodes(instanceList);

    startTime = GetIntervalStart();

    /*
    *   Open association and override hostname & port parameters if they were supplied on the command line.
    */
   /* mcStatus = MC_Open_Association(applicationID,
        &associationID,
        options.RemoteAE,
        options.RemotePort != -1 ? &options.RemotePort : NULL,
        options.RemoteHostname[0] ? options.RemoteHostname : NULL,
        options.ServiceList[0] ? options.ServiceList : NULL);*/

    mcStatus = MC_Open_Association(applicationID,
        &associationID,
        options.RemoteAE,
        &options.RemotePort,
        options.RemoteHostname,
        options.ServiceList);

    CheckMCOpenAssociation(options, mcStatus);

    mcStatus = MC_Get_Association_Info(associationID, &options.asscInfo);
    CheckMCGetAssociationInfo(mcStatus);

    printf("Connected to remote system [%s]\n\n", options.RemoteAE);

    fflush(stdout);


    /*
    *   Send all requested images.  Traverse through instanceList to
    *   get all files to send
    */
    node = instanceList;
    TraverseInstanceListNSend(options, &node,applicationID, associationID, totalImages, &patientinfo);

    /*
    * A failure on close has no real recovery.  Abort the association
    * and continue on.
    */
    mcStatus = MC_Close_Association(&associationID);
    CheckMCCloseAssociation(mcStatus, associationID);

    /*
    * Calculate the transfer rate.  Note, for a real performance
    * numbers, a function other than time() to measure elapsed
    * time should be used.
    */

    seconds = GetIntervalElapsed(startTime);

    printf("Data Transferred: %luMB\n", (unsigned long)(totalBytesRead / (1024 * 1024)));
    printf("    Time Elapsed: %.3fs\n", seconds);
    printf("   Transfer Rate: %.1fKB/s\n", ((float)totalBytesRead / seconds) / 1024.0);
    fflush(stdout);


    /*
    * Release the dICOM Application
    */
    mcStatus = MC_Release_Application(&applicationID);

    CheckMCReleaseApplication(mcStatus);
    /*
    * Free the node list's allocated memory
    */
    FreeList(&instanceList);

    /*
    * Release all memory used by the Merge DICOM Toolkit.
    */

    ReleaseMemory();

    fflush(stdout);

    return(EXIT_SUCCESS);
}

void CheckMCLibraryInitialization(MC_STATUS mcStatus)
{
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("Unable to initialize library", mcStatus);
        exit(0);
    }
}

void CheckMCRegisterApplication(MC_STATUS mcStatus, STORAGE_OPTIONS options)
{
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        printf("Unable to register \"%s\":\n", options.LocalAE);
        printf("\t%s\n", MC_Error_Message(mcStatus));
        fflush(stdout);
        exit(0);
    }
}

void AddInstancesToFile(STORAGE_OPTIONS options, InstanceNode** instanceList, char fname[])
{
    SAMP_BOOLEAN sampBool;
    int imageCurrent = 0;


    for (imageCurrent = options.StartImage; imageCurrent <= options.StopImage; imageCurrent++)
    {
        sprintf(fname, "%d.img", imageCurrent);
        sampBool = AddFileToList(&(*instanceList), fname);
        if (!sampBool)
        {
            printf("Warning, cannot add SOP instance to File List, image will not be sent [%s]\n", fname);
        }
    }
}

void CheckMCOpenAssociation(STORAGE_OPTIONS options, MC_STATUS mcStatus)
{
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        printf("Unable to open association with \"%s\":\n", options.RemoteAE);
        printf("\t%s\n", MC_Error_Message(mcStatus));
        fflush(stdout);
        exit(0);
    }
}

void CheckMCGetAssociationInfo( MC_STATUS mcStatus)
{
    if (mcStatus != MC_NORMAL_COMPLETION)
        PrintError("MC_Get_Association_Info failed", mcStatus);
}

void TraverseInstanceListNSend(STORAGE_OPTIONS options, InstanceNode** node,int applicationID, int associationID, int totalImages, Patient_info* patientinfo)
{
    void* imageStartTime = NULL;
    int imagesSent = 0;
    float seconds = 0.0;
    SAMP_BOOLEAN sampBool;
    MC_STATUS mcStatus;
    size_t totalBytesRead = 0L;
    while (node)
    {
        /*
        * Determine the image format and read the image in.  If the
        * image is in the part 10 format, convert it into a message.
        */
        sampBool = ReadImage(&options, applicationID, (*node), &(patientinfo));
        ReadImageCheckOpen(sampBool, *node);

        totalBytesRead += (*node)->imageBytes;

        /*
        * Send image read in with ReadImage.
        *
        * Because SendImage may not have actually sent an image even though it has returned success,
        * the calculation of performance data below may not be correct.
        */
        sampBool = SendImage(&options, associationID, (*node));
        SendImageCheck(sampBool, *node, associationID, applicationID);

        /*
        * Save image transfer information in list
        */
        sampBool = UpdateNode((*node));
        UpdateNodeCheck(sampBool, *node, associationID, applicationID);

        ImageSentCount(*node,&imagesSent);

        mcStatus = MC_Free_Message(&(*node)->msgID);
        CheckMCFreeMessage(mcStatus);

        seconds =(float)GetIntervalElapsed(imageStartTime);

        printf("\tSent %s image (%d of %d), elapsed time: %.3f seconds\n", (*node)->serviceName, imagesSent, totalImages, seconds);
        /*
        * Traverse through file list
        */
        node = (*node)->Next;

    }   /* END for loop for each image */
}

void ReadImageCheckOpen(SAMP_BOOLEAN sampBool, InstanceNode* node)
{
    if (!sampBool)
    {
        node->imageSent = SAMP_FALSE;
        printf("Can not open image file [%s]\n", node->fname);
        node = node->Next;
    }
}

void SendImageCheck(SAMP_BOOLEAN sampBool, InstanceNode* node, int associationID, int applicationID)
{
    if (!sampBool)
    {
        node->imageSent = SAMP_FALSE;
        printf("Failure in sending file [%s]\n", node->fname);
        MC_Abort_Association(&associationID);
        MC_Release_Application(&applicationID);
    }
}

void UpdateNodeCheck(SAMP_BOOLEAN sampBool, InstanceNode* node, int associationID, int applicationID)
{
    if (!sampBool)
    {
        printf("Warning, unable to update node with information [%s]\n", node->fname);

        MC_Abort_Association(&associationID);
        MC_Release_Application(&applicationID);
    }
}

void ImageSentCount(InstanceNode* node,int* imagesSent)
{
    if ((node)->imageSent == SAMP_TRUE)
    {
        imagesSent++;
    }
    else
    {
        node->responseReceived = SAMP_TRUE;
        node->failedResponse = SAMP_TRUE;
    }
}

void CheckMCFreeMessage(MC_STATUS mcStatus)
{
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("MC_Free_Message failed for request message", mcStatus);
    }
}

void CheckMCCloseAssociation(MC_STATUS mcStatus, int associationID)
{
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("Close association failed", mcStatus);
        MC_Abort_Association(&associationID);
    }
}

void CheckMCReleaseApplication(MC_STATUS mcStatus)
{
    if (mcStatus != MC_NORMAL_COMPLETION)
        PrintError("MC_Release_Application failed", mcStatus);
}

void ReleaseMemory()
{
    if (MC_Library_Release() != MC_NORMAL_COMPLETION)
        printf("Error releasing the library.\n");
}

/*************************************************************************
*
*  Function    :   TestCmdLine
*
*  Parameters  :   Aargc   - Command line arguement count
*                  Aargv   - Command line arguements
*                  A_options - Local application options read in.
*
*  Return value:   SAMP_TRUE
*                  SAMP_FALSE
*
*  Description :   Test command line for valid arguements.  If problems
*                  are found, display a message and return SAMP_FALSE
*
*************************************************************************/
void  CheckcmdArgsStopImage(char* A_argv[], int i, STORAGE_OPTIONS* A_options)
{
    switch (i)
    {
    case 3:
        A_options->StopImage = atoi(A_argv[i]);
        break;
    default:
        printf("Unkown option: %s\n", A_argv[i]);
        break;
    }
}

void  CheckImageDetails(char* A_argv[], int i, STORAGE_OPTIONS* A_options)
{
    switch (i)
    {
    case 1:
        strcpy(A_options->RemoteAE, A_argv[i]);
        break;
    case 2:
        A_options->StartImage = A_options->StopImage = atoi(A_argv[i]);
        break;
    default:
        CheckcmdArgsStopImage(A_argv, i, A_options);
    }
}

int CheckcmdArgsPatientDetails(char* A_argv[], int i, STORAGE_OPTIONS* A_options, Patient_info* patientinfo)
{

    if (!strcmp(A_argv[i], "-i"))
    {
        /*
        * Remote Host Name
        */
        i++;
        strcpy(patientinfo->patient_id, A_argv[i]);
        return i;
    }
    else if (!strcmp(A_argv[i], "-t"))
    {
        /*
        * Remote Host Name
        */
        i++;
        strcpy(patientinfo->patient_name, A_argv[i]);
        return i;
    }
        else
    {
        i= CheckPatientSOP(A_argv, i, A_options, patientinfo);
        return i;
    }

}

int CheckPatientSOP(char* A_argv[], int i, STORAGE_OPTIONS* A_options, Patient_info* patientinfo)
{
    if (!strcmp(A_argv[i], "-a"))
    {
        /*
        * Remote Host Name
        */
        i++;
        strcpy(patientinfo->SOPinstanceID, A_argv[i]);
        return i;
    }
    else
    {
        CheckImageDetails(A_argv, i, A_options);
        return i;
    }
}

int checkcmdArgsHostDetails(char* A_argv[], int i, STORAGE_OPTIONS* A_options, Patient_info* patientinfo)
{
    if (!strcmp(A_argv[i], "-n"))
    {
        /*
        * Remote Host Name
        */
        i++;
        strcpy(A_options->RemoteHostname, A_argv[i]);
        return i;
    }
    else if (!strcmp(A_argv[i], "-p"))
    {
        /*
        * Remote Port Number
        */
        i++;
        A_options->RemotePort = atoi(A_argv[i]);
        return i;
    }
    else
    {
        /*
        * Parse through the rest of the options
        */
        i = CheckcmdArgsPatientDetails(A_argv, i, A_options, patientinfo);
        return i;
    }
}


SAMP_BOOLEAN TestCmdLine(int A_argc, char* A_argv[], STORAGE_OPTIONS* A_options, Patient_info* patientinfo)
{
    patientinfo->patient_id = "Patient ID";
    patientinfo->patient_name = "Last First";
    patientinfo->SOPinstanceID = "\0";
    if (A_argc < 3)
    {
        PrintCmdLine();
        return SAMP_FALSE;
    }

    /*
    * Set default values
    */
    A_options->StartImage = 0;
    A_options->StopImage = 0;

    strcpy(A_options->LocalAE, "MERGE_STORE_SCU");
    strcpy(A_options->RemoteAE, "MERGE_STORE_SCP");
    strcpy(A_options->ServiceList, "Storage_SCU_Service_List");

    A_options->RemoteHostname[0] = '\0';
    A_options->RemotePort = -1;

    A_options->Verbose = SAMP_FALSE;
    A_options->StorageCommit = SAMP_FALSE;
    A_options->ListenPort = 1115;
    A_options->ResponseRequested = SAMP_FALSE;
    A_options->StreamMode = SAMP_FALSE;
    A_options->Username[0] = '\0';
    A_options->Password[0] = '\0';

    A_options->UseFileList = SAMP_FALSE;
    A_options->FileList[0] = '\0';

    /*
    * Loop through each arguement
    */
    CheckcmdArgs(A_argc, A_argv, A_options, patientinfo);
    /*
    * If the hostname & port are specified on the command line,
    * the user may not have the remote system configured in the
    * mergecom.app file.  In this case, force the default service
    * list, so we can attempt to make a connection, or else we would
    * fail.
    */

    Copy_ServiceList(A_options);
    if (A_options->StopImage < A_options->StartImage)
    {
        printf("Image stop number must be greater than or equal to image start number.\n");
        PrintCmdLine();
        return SAMP_FALSE;
    }

    return SAMP_TRUE;

}/* TestCmdLine() */

void Copy_ServiceList(STORAGE_OPTIONS* A_options)
{
    if (CheckHostDetails(A_options)
        && !A_options->ServiceList[0])
    {
        strcpy(A_options->ServiceList, "Storage_SCU_Service_List");
    }
}

SAMP_BOOLEAN CheckHostDetails(STORAGE_OPTIONS* A_options)
{
    if (A_options->RemoteHostname[0]
        && (A_options->RemotePort != -1))
    {
        return SAMP_TRUE;
    }
    return SAMP_FALSE;

}
void CheckcmdArgs(int A_argc, char* A_argv[], STORAGE_OPTIONS* A_options, Patient_info* patientinfo)
{
    int i = 0;
    for (i = 1; i < A_argc; i++)
    {
        int j = checkcmdArgsHostDetails(A_argv, i, A_options, patientinfo);
        if (j > i)
        {
            i++;
        }
    }
}



/****************************************************************************
*
*  Function    :   AddFileToList
*
*  Parameters  :   A_list     - List of nodes.
*                  A_fname    - The name of file to add to the list
*
*  Returns     :   SAMP_TRUE
*                  SAMP_FALSE
*
*  Description :   Create a node in the instance list for a file to be sent
*                  on this association.  The node is added to the end of the
*                  list.
*
****************************************************************************/
SAMP_BOOLEAN AddFileToList(InstanceNode** A_list, char* A_fname)
{
    InstanceNode* newNode;
    newNode = (InstanceNode*)malloc(sizeof(InstanceNode));
    if (!newNode)
    {
        PrintError("Unable to allocate object to store instance information", MC_NORMAL_COMPLETION);
        return (SAMP_FALSE);
    }
    SAMP_BOOLEAN sampbool = AddFileToListInstanceInfo(A_list, A_fname, newNode);
    return (sampbool);
}

SAMP_BOOLEAN AddFileToListInstanceInfo(InstanceNode** A_list, char* A_fname, InstanceNode* newNode)
{
    InstanceNode* listNode;
    memset(newNode, 0, sizeof(InstanceNode));

    strncpy(newNode->fname, A_fname, sizeof(newNode->fname));
    newNode->fname[sizeof(newNode->fname) - 1] = '\0';

    newNode->responseReceived = SAMP_FALSE;
    newNode->failedResponse = SAMP_FALSE;
    newNode->imageSent = SAMP_FALSE;
    newNode->msgID = -1;
    newNode->transferSyntax = IMPLICIT_LITTLE_ENDIAN;

    if (!*A_list)
    {
        /*
        * Nothing in the list
        */
        newNode->Next = *A_list;
        *A_list = newNode;
    }
    else
    {
        /*
        * Add to the tail of the list
        */
        listNode = *A_list;

        while (listNode->Next)
            listNode = listNode->Next;

        listNode->Next = newNode;
    }
    return SAMP_TRUE;
}


/****************************************************************************
*
*  Function    :   GetNumNodes
*
*  Parameters  :   A_list     - Pointer to head of node list to get count for
*
*  Returns     :   int, num node entries in list
*
*  Description :   Gets a count of the current list of instances.
*
****************************************************************************/
int GetNumNodes(InstanceNode* A_list)

{
    int            numNodes = 0;
    InstanceNode* node;

    node = A_list;
    while (node)
    {
        numNodes++;
        node = node->Next;
    }

    return numNodes;
}

SAMP_BOOLEAN ReadImage(STORAGE_OPTIONS* A_options, int A_appID, InstanceNode* A_node, Patient_info* patientinfo)
{
    FORMAT_ENUM             format = IMPLICIT_LITTLE_ENDIAN_FORMAT;
    SAMP_BOOLEAN            sampBool;

    sampBool = ReadMessageFromFile(A_options, A_node->fname, format, &A_node->msgID, &A_node->transferSyntax, &A_node->imageBytes);
    SAMP_BOOLEAN sampboolean = TRUE;
    if (sampBool == TRUE)
    {
        sampboolean = ReadImageChangeID(A_node);
    }
    SAMP_BOOLEAN sampbool = ChangePatientInfo(A_node, patientinfo);
    if (sampbool == FALSE)
    {
        return sampbool;
    }
    fflush(stdout);
    return sampboolean;
}

SAMP_BOOLEAN ReadImageChangeID(InstanceNode* A_node)
{
    MC_STATUS               mcStatus;
    mcStatus = MC_Get_Value_To_String(A_node->msgID, MC_ATT_SOP_CLASS_UID, sizeof(A_node->SOPClassUID), A_node->SOPClassUID);
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("MC_Get_Value_To_String for SOP Class UID failed", mcStatus);
        return FALSE;
    }

    mcStatus = MC_Get_Value_To_String(A_node->msgID, MC_ATT_SOP_INSTANCE_UID, sizeof(A_node->SOPInstanceUID), A_node->SOPInstanceUID);
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("MC_Get_Value_To_String for SOP Instance UID failed", mcStatus);
        return FALSE;
    }
    return TRUE;
}
SAMP_BOOLEAN ChangePatientInfo(InstanceNode* A_node, Patient_info* patientinfo)
{
    MC_STATUS               mcStatus;
    mcStatus = MC_Set_Value_From_String(A_node->msgID, MC_ATT_PATIENTS_NAME, patientinfo->patient_name);
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("MC_Set_Value_From_String for Patient name failed", mcStatus);
        return FALSE;
    }
    mcStatus = MC_Set_Value_From_String(A_node->msgID, MC_ATT_PATIENT_ID, patientinfo->patient_id);
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("MC_Set_Value_From_String for Patient id failed", mcStatus);
        return FALSE;
    }
    return changePatientSOP(A_node,patientinfo);
}

SAMP_BOOLEAN changePatientSOP(InstanceNode* A_node, Patient_info* patientinfo)
{
    MC_STATUS               mcStatus;
    mcStatus = MC_Set_Value_From_String(A_node->msgID, MC_ATT_SOP_INSTANCE_UID, patientinfo->SOPinstanceID);
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("MC_Set_Value_From_String for SOPinstanceID failed", mcStatus);
        return FALSE;
    }
    return TRUE;
}

/****************************************************************************
*
*  Function    :   SendImage
*
*  Parameters  :   A_options  - Pointer to structure containing input
*                               parameters to the application
*                  A_associationID - Association ID registered
*                  A_node     - The node in our list of instances
*
*  Returns     :   SAMP_TRUE
*                  SAMP_FALSE on failure where association must be aborted
*
*  Description :   Send message containing the image in the node over
*                  the association.
*
*                  SAMP_TRUE is returned on success, or when a recoverable
*                  error occurs.
*
****************************************************************************/

SAMP_BOOLEAN SendImage(STORAGE_OPTIONS* A_options, int A_associationID, InstanceNode* A_node)
{
    MC_STATUS       mcStatus;
    SAMP_BOOLEAN    sampBool = SAMP_TRUE;

    A_node->imageSent = SAMP_FALSE;

    /* Get the SOP class UID and set the service */
    mcStatus = MC_Get_MergeCOM_Service(A_node->SOPClassUID, A_node->serviceName, sizeof(A_node->serviceName));

    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("MC_Get_MergeCOM_Service failed", mcStatus);
        fflush(stdout);
        return (SAMP_TRUE);
    }

    mcStatus = MC_Set_Service_Command(A_node->msgID, A_node->serviceName, C_STORE_RQ);
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("MC_Set_Service_Command failed", mcStatus);
        fflush(stdout);
        return (SAMP_TRUE);
    }

    /* set affected SOP Instance UID */
    mcStatus = MC_Set_Value_From_String(A_node->msgID, MC_ATT_AFFECTED_SOP_INSTANCE_UID, A_node->SOPInstanceUID);

    /* Check whether SOP Instance UID is set to correct value */
    SendImageSetSOPInstanceUID(mcStatus);

    /*
    *  Send the message
    */

    sampBool = SendImageRequestMessage(A_options, A_associationID, A_node);

    A_node->imageSent = SAMP_TRUE;
    fflush(stdout);

    return sampBool;
}

void SendImageSetSOPInstanceUID(MC_STATUS mcStatus)
{
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("MC_Set_Value_From_String failed for affected SOP Instance UID", mcStatus);
        fflush(stdout);
    }

}

SAMP_BOOLEAN SendImageRequestMessage(STORAGE_OPTIONS* A_options, int A_associationID, InstanceNode* A_node)
{
    MC_STATUS       mcStatus;
    mcStatus = MC_Send_Request_Message(A_associationID, A_node->msgID);
    if ((mcStatus == MC_ASSOCIATION_ABORTED) || (mcStatus == MC_SYSTEM_ERROR))
    {
        /* At this point, the association has been dropped, or we should drop it in the case of error. */
        PrintError("MC_Send_Request_Message failed", mcStatus);
        fflush(stdout);
        return (SAMP_FALSE);
    }

    return SendImageCheckNormal(A_associationID, A_node);
}

SAMP_BOOLEAN SendImageCheckNormal(int A_associationID, InstanceNode* A_node)
{
    MC_STATUS       mcStatus;
    mcStatus = MC_Send_Request_Message(A_associationID, A_node->msgID);
    if (mcStatus == MC_UNACCEPTABLE_SERVICE)
    {
        /* At this point, the association has been dropped, or we should drop it in the case of error. */
        PrintError("MC_Send_Request_Message failed", mcStatus);
        fflush(stdout);
        return (SAMP_FALSE);
    }
    else if (mcStatus != MC_NORMAL_COMPLETION)
    {
        /* This is a failure condition we can continue with */
        PrintError("Warning: MC_Send_Request_Message failed", mcStatus);
        fflush(stdout);
        return (SAMP_TRUE);
    }

    return SAMP_TRUE;
}

SAMP_BOOLEAN UpdateNode(InstanceNode* A_node)
{
    MC_STATUS        mcStatus;

    /*
    * Get DICOM msgID for tracking of responses
    */
    mcStatus = MC_Get_Value_To_UInt(A_node->msgID, MC_ATT_MESSAGE_ID, &(A_node->dicomMsgID));
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("MC_Get_Value_To_UInt for Message ID failed", mcStatus);
        A_node->responseReceived = SAMP_TRUE;
        return(SAMP_FALSE);
    }

    A_node->responseReceived = SAMP_FALSE;
    A_node->failedResponse = SAMP_FALSE;
    A_node->imageSent = SAMP_TRUE;

    return (SAMP_TRUE);
}


/****************************************************************************
*
*  Function    :   PrintError
*
*  Description :   Display a text string on one line and the error message
*                  for a given error on the next line.
*
****************************************************************************/
void PrintError(char* A_string, MC_STATUS A_status)
{
    char        prefix[30] = { 0 };
    /*
    *  Need process ID number for messages
    */
#ifdef UNIX
    sprintf(prefix, "PID %d", getpid());
#endif
    if (A_status == -1)
    {
        printf("%s\t%s\n", prefix, A_string);
    }
    else
    {
        printf("%s\t%s:\n", prefix, A_string);
        printf("%s\t\t%s\n", prefix, MC_Error_Message(A_status));
    }
}


/****************************************************************************
*
*  Function    :   FreeList
*
*  Parameters  :   A_list     - Pointer to head of node list to free.
*
*  Returns     :   nothing
*
*  Description :   Free the memory allocated for a list of nodesransferred
*
****************************************************************************/
void FreeList(InstanceNode** A_list)
{
    InstanceNode* node;

    /*
    * Free the instance list
    */
    while (*A_list)
    {
        node = *A_list;
        *A_list = node->Next;

        if (node->msgID != -1)
            MC_Free_Message(&node->msgID);

        free(node);
    }
}

/********************************************************************
*
*  Function    :   PrintCmdLine
*
*  Parameters  :   none
*
*  Returns     :   nothing
*
*  Description :   Prints program usage
*
********************************************************************/
void PrintCmdLine(void)
{
    printf("\nUsage stor_scu remote_ae start stop -f filename -a local_ae -b local_port -c -n remote_host -p remote_port -l service_list -v -u username -w password -q\n");
    printf("\n");
    printf("\t remote_ae       name of remote Application Entity Title to connect with\n");
    printf("\t start           start image number (not required if -f specified)\n");
    printf("\t stop            stop image number (not required if -f specified)\n");
    printf("\t -n remote_host  (optional) specify the remote hostname (default: found in the mergecom.app file for remote_ae)\n");
    printf("\t -p remote_port  (optional) specify the remote TCP listen port (default: found in the mergecom.app file for remote_ae)\n");
    printf("\t -t patient_name Patient name\n");
    printf("\t -i patient id   Patient Id \n");
    printf("\n");
    printf("\tImage files must be in the current directory if -f is not used.\n");
    printf("\tImage files must be named 0.img, 1.img, 2.img, etc if -f is not used.\n");

} /* end PrintCmdLine() */


  /****************************************************************************
  *
  *  Function    :   ReadMessageFromFile
  *
  *  Parameters  :   A_options  - Pointer to structure containing input
  *                               parameters to the application
  *                  A_filename - Name of file to open
  *                  A_format   - Enum containing the format of the object
  *                  A_msgID    - The message ID of the message to be opened
  *                               returned here.
  *                  A_syntax   - The transfer syntax read is returned here.
  *                  A_bytesRead- Total number of bytes read in image.  Used
  *                               only for display and to calculate the
  *                               transfer rate.
  *
  *  Returns     :   SAMP_TRUE  on success
  *                  SAMP_FALSE on failure to open the file
  *
  *  Description :   This function reads a file in the DICOM "stream" format.
  *                  This format contains no DICOM part 10 header information.
  *                  The transfer syntax of the object is contained in the
  *                  A_format parameter.
  *
  *                  When this function returns failure, the caller should
  *                  not do any cleanup, A_msgID will not contain a valid
  *                  message ID.
  *
  ****************************************************************************/
SAMP_BOOLEAN ReadMessageFromFile(STORAGE_OPTIONS* A_options,
    char* A_filename,
    FORMAT_ENUM A_format,
    int* A_msgID,
    TRANSFER_SYNTAX* A_syntax,
    size_t* A_bytesRead)
{

    MC_STATUS mcStatus;
    unsigned long errorTag = 0;
    CBinfo callbackInfo = { 0 };
    SAMP_BOOLEAN sampBool;

    /*
    * Determine the format
    */

    * A_syntax = IMPLICIT_LITTLE_ENDIAN;

    /*
    * Open an empty message object to load the image into
    */

    sampBool = ReadMessageFromFileEmptyMessage(A_msgID);

    /*
    * Open and stream message from file
    */

    callbackInfo.fp = fopen(A_filename, BINARY_READ);
    sampBool = ReadMessageFromFileOpenNStream(A_filename, A_msgID);

    /*Allocating Buffer*/
    if (callbackInfo.bufferLength == 0)
    {
        int length = 0;

        mcStatus = MC_Get_Int_Config_Value(WORK_BUFFER_SIZE, &length);
        if (mcStatus != MC_NORMAL_COMPLETION)
        {
            length = WORK_SIZE;
        }
        callbackInfo.bufferLength = length;
    }

    callbackInfo.buffer = malloc(callbackInfo.bufferLength);

    /*Checking if buffer is created and allocated*/
    sampBool = ReadMessageFromFileBufferAllocate(callbackInfo);

    mcStatus = MC_Stream_To_Message(*A_msgID, 0x00080000, 0xFFFFFFFF, *A_syntax, &errorTag, (void*)&callbackInfo, StreamToMsgObj);

    /* Close the file */
    ReadMessageFromFileClose(callbackInfo);

    sampBool = ReadMessageFromFileStreamError(mcStatus, A_msgID);

    *A_bytesRead = callbackInfo.bytesRead;
    fflush(stdout);

    return sampBool;

} /* ReadMessageFromFile() */

SAMP_BOOLEAN ReadMessageFromFileEmptyMessage(int* A_msgID)
{
    MC_STATUS mcStatus;
    //SAMP_BOOLEAN sampBool;

    mcStatus = MC_Open_Empty_Message(A_msgID);
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("Unable to open empty message", mcStatus);
        fflush(stdout);
        return SAMP_FALSE;
    }

    return SAMP_TRUE;
}

SAMP_BOOLEAN ReadMessageFromFileOpenNStream(char* A_filename, int* A_msgID)
{
    CBinfo callbackInfo = { 0 };
    int retStatus = 0;
    //SAMP_BOOLEAN sampBool;

    callbackInfo.fp = fopen(A_filename, BINARY_READ);
    if (!callbackInfo.fp)
    {
        printf("ERROR: Unable to open %s.\n", A_filename);
        MC_Free_Message(A_msgID);
        fflush(stdout);
        return SAMP_FALSE;
    }

    retStatus = setvbuf(callbackInfo.fp, (char*)NULL, _IOFBF, 32768);
    if (retStatus != 0)
    {
        printf("WARNING: Unable to set IO buffering on input file.\n");
    }

    return SAMP_TRUE;
}

SAMP_BOOLEAN ReadMessageFromFileBufferAllocate(CBinfo callbackInfo)
{

    callbackInfo.buffer = malloc(callbackInfo.bufferLength);
    if (callbackInfo.buffer == NULL)
    {
        printf("ERROR: failed to allocate file read buffer [%d] kb", (int)callbackInfo.bufferLength);
        fflush(stdout);
        return SAMP_FALSE;
    }

    return SAMP_TRUE;
}

SAMP_BOOLEAN ReadMessageFromFileStreamError(MC_STATUS mcStatus, int* A_msgID)
{
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("MC_Stream_To_Message error, possible wrong transfer syntax guessed", mcStatus);
        MC_Free_Message(A_msgID);
        fflush(stdout);
        return SAMP_FALSE;
    }

    return SAMP_TRUE;
}

void ReadMessageFromFileClose(CBinfo callbackInfo)
{
    if (callbackInfo.fp)
        fclose(callbackInfo.fp);

    if (callbackInfo.buffer)
        free(callbackInfo.buffer);
}

/*************************************************************************
*
* Function : StreamToMsgObj
*
* Parameters : A_msgID - Message ID of message being read
* A_CBinformation - user information passwd to callback
* A_isFirst - flag to tell if this is the first call
* A_dataSize - length of data read
* A_dataBuffer - buffer where read data is stored
* A_isLast - flag to tell if this is the last call
*
* Returns : MC_NORMAL_COMPLETION on success
* any other return value on failure.
*
* Description : Reads input stream data from a file, and places the data
* into buffer to be used by the MC_Stream_To_Message function.
*
**************************************************************************/
MC_STATUS NOEXP_FUNC StreamToMsgObj(int A_msgID,
    void* A_CBinformation,
    int A_isFirst,
    int* A_dataSize,
    void** A_dataBuffer,
    int* A_isLast)
{
    size_t bytesRead;
    CBinfo* callbackInfo = (CBinfo*)A_CBinformation;

    if (A_isFirst)
        callbackInfo->bytesRead = 0L;

    bytesRead = fread(callbackInfo->buffer, 1, callbackInfo->bufferLength, callbackInfo->fp);
    if (ferror(callbackInfo->fp))
    {
        perror("\tRead error when streaming message from file.\n");
        return MC_CANNOT_COMPLY;
    }
    *A_isLast = StreamToMsgObj1(callbackInfo);
    *A_dataBuffer = callbackInfo->buffer;
    *A_dataSize = (int)bytesRead;
    callbackInfo->bytesRead += bytesRead;

    return MC_NORMAL_COMPLETION;
} /* StreamToMsgObj() */

int StreamToMsgObj1(CBinfo* callbackInfo)
{
    int i;
    if (feof(callbackInfo->fp))
    {
        i = 1;
        fclose(callbackInfo->fp);
        callbackInfo->fp = NULL;
    }
    else
        i = 0;
    return i;
}
