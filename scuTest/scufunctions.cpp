#include "scutest.h"
void dicom_scu::Copy_ServiceList(STORAGE_OPTIONS* A_options)
{
    if (!A_options->ServiceList[0])
    {
        strcpy(A_options->ServiceList, "Storage_SCU_Service_List");
    }
}

SAMP_BOOLEAN dicom_scu::CheckHostDetails(STORAGE_OPTIONS* A_options)
{
    if (A_options->RemoteHostname[0]
        && (A_options->RemotePort != -1))
    {
        return SAMP_TRUE;
    }
    return SAMP_FALSE;

}

SAMP_BOOLEAN dicom_scu::ReadMessageFromFileStreamError(MC_STATUS mcStatus, int* A_msgID)
{
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        //PrintError("MC_Stream_To_Message error, possible wrong transfer syntax guessed", mcStatus);
        //MC_Free_Message(A_msgID);
        fflush(stdout);
        return SAMP_FALSE;
    }

    return SAMP_TRUE;
}

int dicom_scu::GetNumNodes(InstanceNode* A_list)
{
    int numNodes = 0;
    InstanceNode* node;

    node = A_list;
    while (node)
    {
        numNodes++;
        node = node->Next;
    }

    return numNodes;
}

SAMP_BOOLEAN dicom_scu::AddInstancesToFile(STORAGE_OPTIONS options, InstanceNode** instanceList, char fname[])
{
    SAMP_BOOLEAN sampBool;
    int imageCurrent = 0;


    for (imageCurrent = options.StartImage; imageCurrent <= options.StopImage; imageCurrent++)
    {
        sprintf(fname, "%d.img", imageCurrent);
        sampBool = dicom_scu::AddFileToList(&(*instanceList), fname);
        if (!sampBool)
        {
            //printf("Warning, cannot add SOP instance to File List, image will not be sent [%s]\n", fname);
            sampBool = dicom_scu::AddFileToList(&(*instanceList), fname);
        }
        return sampBool;
    }
}

SAMP_BOOLEAN dicom_scu::AddFileToList(InstanceNode** A_list, char* A_fname)
{
    InstanceNode* newNode;
    newNode = (InstanceNode*)malloc(sizeof(InstanceNode));
    if (!newNode)
    {
        //PrintError("Unable to allocate object to store instance information", MC_NORMAL_COMPLETION);
        return (SAMP_FALSE);
    }
    SAMP_BOOLEAN sampbool = dicom_scu::AddFileToListInstanceInfo(A_list, A_fname, newNode);
    return (sampbool);
}

SAMP_BOOLEAN dicom_scu::AddFileToListInstanceInfo(InstanceNode** A_list, char* A_fname, InstanceNode* newNode)
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

int  dicom_scu::ImageSentCount(InstanceNode* node)
{
    int imagesSent = 0;
    if ((node)->imageSent == SAMP_TRUE)
    {
        imagesSent++;
    }
    else
    {
        node->responseReceived = SAMP_TRUE;
        node->failedResponse = SAMP_TRUE;
    }
    return imagesSent;
}

SAMP_BOOLEAN dicom_scu::ReadMessageFromFileOpenNStream(char* A_filename, int* A_msgID)
{
    CBinfo callbackInfo = { 0 };
    int retStatus = 0;
    //SAMP_BOOLEAN sampBool;

    callbackInfo.fp = fopen(A_filename, BINARY_READ);
    if (!callbackInfo.fp)
    {
       // printf("ERROR: Unable to open %s.\n", A_filename);
       // MC_Free_Message(A_msgID);
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
void dicom_scu::FreeList(InstanceNode**    A_list)
{
    InstanceNode*    node;

    /*
    * Free the instance list
    */
    while (*A_list)
    {
        node = *A_list;
        *A_list = node->Next;

        if (node->msgID != -1)
            // MC_Free_Message(&node->msgID);

            free(node);
    }
}
