#include "stor_scu.h"
int main(int argc, char** argv)
{
    SAMP_BOOLEAN            sampBool;
    STORAGE_OPTIONS         options;
    MC_STATUS               mcStatus;
    int                     applicationID = -1, associationID = -1, imageCurrent = 0;
    int                     imagesSent = 0L, totalImages = 0L, fstatus = 0;
    double                  seconds = 0.0;
    void                    *startTime = NULL, *imageStartTime = NULL;
    char                    fname[512] = {0};  /* Extra long, just in case */
    ServiceInfo             servInfo;
    size_t                  totalBytesRead = 0L;
    InstanceNode            *instanceList = NULL, *node = NULL;
    FILE*                   fp = NULL;

    /*
     * Test the command line parameters, and populate the options
     * structure with these parameters
     */
    sampBool = TestCmdLine( argc, argv, &options );
    if ( sampBool == SAMP_FALSE )
    {
        return(EXIT_FAILURE);
    }

    /* ------------------------------------------------------- */
    /* This call MUST be the first call made to the library!!! */
    /* ------------------------------------------------------- */
    mcStatus = MC_Library_Initialization ( NULL, NULL, NULL );
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("Unable to initialize library", mcStatus);
        return ( EXIT_FAILURE );
    }

    /*
     *  Register this DICOM application
     */
    mcStatus = MC_Register_Application(&applicationID, options.LocalAE);
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        printf("Unable to register \"%s\":\n", options.LocalAE);
        printf("\t%s\n", MC_Error_Message(mcStatus));
        fflush(stdout);
        return(EXIT_FAILURE);
    }

    /*
     * Create a linked list of all files to be transferred.
     * Retrieve the list from a specified file on the command line,
     * or generate the list from the start & stop numbers on the
     * command line
     */
   /* Traverse through the possible names and add them to the list based on the start/stop count */
        for (imageCurrent = options.StartImage; imageCurrent <= options.StopImage; imageCurrent++)
        {
            sprintf(fname, "%d.img", imageCurrent);
            sampBool = AddFileToList( &instanceList, fname );
            if (!sampBool)
            {
                printf("Warning, cannot add SOP instance to File List, image will not be sent [%s]\n", fname);
            }
        }
    

    totalImages = GetNumNodes( instanceList );

    
    startTime = GetIntervalStart();

    /*
     * Open the association with user identity information if specified
     * on the command line.  User Identity negotiation was defined
     * in DICOM Supplement 99.
     */
    
        /*
         *   Open association and override hostname & port parameters if they were supplied on the command line.
         */
        mcStatus = MC_Open_Association( applicationID, 
                                        &associationID,
                                        options.RemoteAE,
                                        options.RemotePort != -1 ? &options.RemotePort : NULL,
                                        options.RemoteHostname[0] ? options.RemoteHostname : NULL,
                                        options.ServiceList[0] ? options.ServiceList : NULL );
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        printf("Unable to open association with \"%s\":\n", options.RemoteAE);
        printf("\t%s\n", MC_Error_Message(mcStatus));
        fflush(stdout);
        return(EXIT_FAILURE);
    }

    mcStatus = MC_Get_Association_Info( associationID, &options.asscInfo);
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("MC_Get_Association_Info failed", mcStatus);
    }

    
    printf("Connected to remote system [%s]\n\n", options.RemoteAE);

    /*
     * Check User Identity Negotiation and for response
     */
    if (options.ResponseRequested)
    {
        if (!options.asscInfo.PositiveResponseReceived)
        {
            printf("WARNING: Positive response for User Identity requested from\n\tserver, but not received.\n\n");
        }
    }
    
    fflush(stdout);

    /*
     *   Send all requested images.  Traverse through instanceList to
     *   get all files to send
     */
    node = instanceList;
    while ( node )
    {
        imageStartTime = GetIntervalStart();

        
            /*
            * Determine the image format and read the image in.  If the
            * image is in the part 10 format, convert it into a message.
            */
            sampBool = ReadImage( &options, applicationID, node);
            if (!sampBool)
            {
                node->imageSent = SAMP_FALSE;
                printf("Can not open image file [%s]\n", node->fname);
                node = node->Next;
                continue;
            }

            totalBytesRead += node->imageBytes;

            /*
             * Send image read in with ReadImage.
             *
             * Because SendImage may not have actually sent an image even though it has returned success, 
             * the calculation of performance data below may not be correct.
             */
            sampBool = SendImage( &options, associationID, node);
            if (!sampBool)
            {
                node->imageSent = SAMP_FALSE;
                printf("Failure in sending file [%s]\n", node->fname);
                MC_Abort_Association(&associationID);
                MC_Release_Application(&applicationID);
                break;
            }

            /*
             * Save image transfer information in list
             */
            sampBool = UpdateNode( node );
            if (!sampBool)
            {
                printf("Warning, unable to update node with information [%s]\n", node->fname);

                MC_Abort_Association(&associationID);
                MC_Release_Application(&applicationID);
                break;
            }

        if ( node->imageSent == SAMP_TRUE )
        {
            imagesSent++;
        }
        else
        {
            node->responseReceived = SAMP_TRUE;
            node->failedResponse = SAMP_TRUE;
        }

            mcStatus = MC_Free_Message(&node->msgID);
            if (mcStatus != MC_NORMAL_COMPLETION)
            {
                PrintError("MC_Free_Message failed for request message", mcStatus);
            }

            /*
            * The following is the core code for handling DICOM asynchronous
            * transfers.  With asynchronous communications, the SCU is allowed
            * to send multiple request messages to the server without
            * receiving a response message.  The MaxOperationsInvoked is
            * negotiated over the association, and determines how many request
            * messages the SCU can send before a response must be read.
            *
            * In this code, we first poll to see if a response message is
            * available.  This means data is readily available to be read over
            * the connection.  If there is no data to be read & asychronous
            * operations have been negotiated, and we haven't reached the max
            * number of operations invoked, we can just go ahead and send
            * the next request message.  If not, we go into the loop below
            * waiting for the response message from the server.
            *
            * This code alows network transfer speeds to improve.  Instead of
            * having to wait for a response message, the SCU can immediately
            * send the next request message so that the connection bandwidth
            * is better utilized.
            */
            sampBool = ReadResponseMessages( &options, associationID, 0, &instanceList, NULL );
            if (!sampBool)
            {
                printf("Failure in reading response message, aborting association.\n");

                MC_Abort_Association(&associationID);
                MC_Release_Application(&applicationID);
                break;
            }

            /*
            * 0 for MaxOperationsInvoked means unlimited operations.  don't poll if this is the case, just
            * go to the next request to send.
            */
            if ( options.asscInfo.MaxOperationsInvoked > 0 )
            {
                while ( GetNumOutstandingRequests( instanceList ) >= options.asscInfo.MaxOperationsInvoked )
                {
                    sampBool = ReadResponseMessages( &options, associationID, 10, &instanceList, NULL );
                    if (!sampBool)
                    {
                        printf("Failure in reading response message, aborting association.\n");
                        MC_Abort_Association(&associationID);
                        MC_Release_Application(&applicationID);
                        break;
                    }
                }
            }
        

        seconds = GetIntervalElapsed(imageStartTime);
        if ( options.Verbose )
            printf("     Time: %.3f seconds\n\n", (float)seconds);
        else
            printf("\tSent %s image (%d of %d), elapsed time: %.3f seconds\n", node->serviceName, imagesSent, totalImages, seconds);

        /*
         * Traverse through file list
         */
        node = node->Next;

    }   /* END for loop for each image */

    /*
     * Wait for any remaining C-STORE-RSP messages.  This will only happen
     * when asynchronous communications are used.
     */
    while ( GetNumOutstandingRequests( instanceList ) > 0 )
    {
        sampBool = ReadResponseMessages( &options, associationID, 10, &instanceList, NULL );
        if (!sampBool)
        {
            printf("Failure in reading response message, aborting association.\n");
            MC_Abort_Association(&associationID);
            MC_Release_Application(&applicationID);
            break;
        }
    }

    /*
     * A failure on close has no real recovery.  Abort the association
     * and continue on.
     */
    mcStatus = MC_Close_Association(&associationID);
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("Close association failed", mcStatus);
        MC_Abort_Association(&associationID);
    }

    /*
     * Calculate the transfer rate.  Note, for a real performance
     * numbers, a function other than time() to measure elapsed
     * time should be used.
     */
    if (options.Verbose)
    {
        printf("Association Closed.\n" );
    }

    seconds = GetIntervalElapsed(startTime);
    
    printf("Data Transferred: %luMB\n", (unsigned long) (totalBytesRead / (1024 * 1024)) );
    printf("    Time Elapsed: %.3fs\n", seconds);
    printf("   Transfer Rate: %.1fKB/s\n", ((float)totalBytesRead / seconds) / 1024.0);
    fflush(stdout);

    /*
     * Release the dICOM Application
     */
    mcStatus = MC_Release_Application(&applicationID);
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("MC_Release_Application failed", mcStatus);
    }

    /*
     * Free the node list's allocated memory
     */
    FreeList( &instanceList );

    /*
     * Release all memory used by the Merge DICOM Toolkit.
     */
    if (MC_Library_Release() != MC_NORMAL_COMPLETION)
        printf("Error releasing the library.\n");
    
    fflush(stdout);

    return(EXIT_SUCCESS);
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
static void PrintCmdLine(void)
{
    printf("\nUsage stor_scu remote_ae start stop -f filename -a local_ae -b local_port -c -n remote_host -p remote_port -l service_list -v -u username -w password -q\n");
    printf("\n");
    printf("\t remote_ae       name of remote Application Entity Title to connect with\n");
    printf("\t start           start image number (not required if -f specified)\n");
    printf("\t stop            stop image number (not required if -f specified)\n");
    printf("\t -f filename     (optional) specify a file containing a list of images to transfer\n");
    printf("\t -a local_ae     (optional) specify the local Application Title (default: MERGE_STORE_SCU)\n");
    printf("\t -b local_port   (optional) specify the local TCP listen port for commitment (default: found in the mergecom.pro file)\n");
    printf("\t -c              Do storage commitment for the transferred files\n");
    printf("\t -n remote_host  (optional) specify the remote hostname (default: found in the mergecom.app file for remote_ae)\n");
    printf("\t -p remote_port  (optional) specify the remote TCP listen port (default: found in the mergecom.app file for remote_ae)\n");
    printf("\t -l service_list (optional) specify the service list to use when negotiating (default: Storage_SCU_Service_List)\n");
    printf("\t -s              Transfer the data using stream (raw) mode\n");
    printf("\t -v              Execute in verbose mode, print negotiation information\n");
    printf("\t -u username     (optional) specify a username to negotiate as defined in DICOM Supplement 99\n");
    printf("\t                 Note: just a username can be specified, or a username and password can be specified\n");
    printf("\t                       A password alone cannot be specified.\n");
    printf("\t -w password     (optional) specify a password to negotiate as defined in DICOM Supplement 99\n");
    printf("\t -q              Positive response to user identity requested from SCP\n");
    printf("\n");
    printf("\tImage files must be in the current directory if -f is not used.\n");
    printf("\tImage files must be named 0.img, 1.img, 2.img, etc if -f is not used.\n");

} /* end PrintCmdLine() */


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
static SAMP_BOOLEAN TestCmdLine( int A_argc, char* A_argv[], STORAGE_OPTIONS* A_options )
{
    int       i = 0, argCount=0;

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
    for (i = 1; i < A_argc; i++)
    {
        if ( !strcmp(A_argv[i], "-h") || !strcmp(A_argv[i], "/h") ||
             !strcmp(A_argv[i], "-H") || !strcmp(A_argv[i], "/H") ||
             !strcmp(A_argv[i], "-?") || !strcmp(A_argv[i], "/?"))
        {
            PrintCmdLine();
            return SAMP_FALSE;
        }
        else if ( !strcmp(A_argv[i], "-a") || !strcmp(A_argv[i], "-A"))
        {
            /*
             * Set the Local AE
             */
            i++;
            strcpy(A_options->LocalAE, A_argv[i]);
        }
        else if ( !strcmp(A_argv[i], "-b") || !strcmp(A_argv[i], "-B"))
        {
            /*
             * Local Port Number
             */
            i++;
            A_options->ListenPort = atoi(A_argv[i]);

        }
        else if ( !strcmp(A_argv[i], "-c") || !strcmp(A_argv[i], "-C"))
        {
            /*
             * StorageCommit mode
             */
            A_options->StorageCommit = SAMP_TRUE;
        }
        else if ( !strcmp(A_argv[i], "-f") || !strcmp(A_argv[i], "-F"))
        {
            /*
             * Config file with filenames
             */
            i++;
            A_options->UseFileList = SAMP_TRUE;
            strcpy(A_options->FileList,A_argv[i]);
        }
        else if ( !strcmp(A_argv[i], "-l") || !strcmp(A_argv[i], "-L"))
        {
            /*
             * Service List
             */
            i++;
            strcpy(A_options->ServiceList,A_argv[i]);
        }
        else if ( !strcmp(A_argv[i], "-n") || !strcmp(A_argv[i], "-N"))
        {
            /*
             * Remote Host Name
             */
            i++;
            strcpy(A_options->RemoteHostname,A_argv[i]);
        }
        else if ( !strcmp(A_argv[i], "-p") || !strcmp(A_argv[i], "-P"))
        {
            /*
             * Remote Port Number
             */
            i++;
            A_options->RemotePort = atoi(A_argv[i]);

        }
        else if ( !strcmp(A_argv[i], "-q") || !strcmp(A_argv[i], "-Q"))
        {
            /*
             * Positive response requested from server.
             */
            A_options->ResponseRequested = SAMP_TRUE;
        }
        else if ( !strcmp(A_argv[i], "-s") || !strcmp(A_argv[i], "-S"))
        {
            /*
             * Stream mode
             */
            A_options->StreamMode = SAMP_TRUE;
        }
        else if ( !strcmp(A_argv[i], "-u") || !strcmp(A_argv[i], "-U"))
        {
            /*
             * Username
             */
            i++;
            strcpy(A_options->Username,A_argv[i]);
        }
        else if ( !strcmp(A_argv[i], "-v") || !strcmp(A_argv[i], "-V"))
        {
            /*
             * Verbose mode
             */
            A_options->Verbose = SAMP_TRUE;
        }
        else if ( !strcmp(A_argv[i], "-w") || !strcmp(A_argv[i], "-W"))
        {
            /*
             * Username
             */
            i++;
            strcpy(A_options->Password,A_argv[i]);
        }
        else
        {
            /*
             * Parse through the rest of the options
             */

            argCount++;
            switch (argCount)
            {
                case 1:
                    strcpy(A_options->RemoteAE, A_argv[i]);
                    break;
                case 2:
                    A_options->StartImage = A_options->StopImage = atoi(A_argv[i]);
                    break;
                case 3:
                    A_options->StopImage = atoi(A_argv[i]);
                    break;
                default:
                    printf("Unkown option: %s\n",A_argv[i]);
                    break;
            }
        }
    }

    /*
     * If the hostname & port are specified on the command line,
     * the user may not have the remote system configured in the
     * mergecom.app file.  In this case, force the default service
     * list, so we can attempt to make a connection, or else we would
     * fail.
     */
    if ( A_options->RemoteHostname[0]
    &&  !A_options->ServiceList[0]
     && ( A_options->RemotePort != -1))
    {
        strcpy(A_options->ServiceList, "Storage_SCU_Service_List");
    }


    if (A_options->StopImage < A_options->StartImage)
    {
        printf("Image stop number must be greater than or equal to image start number.\n");
        PrintCmdLine();
        return SAMP_FALSE;
    }

    return SAMP_TRUE;

}/* TestCmdLine() */


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
static SAMP_BOOLEAN AddFileToList(InstanceNode** A_list, char* A_fname)
{
    InstanceNode*    newNode;
    InstanceNode*    listNode;
    printf("----------------------");
    newNode = (InstanceNode*)malloc(sizeof(InstanceNode));
    if (!newNode)
    {
        PrintError("Unable to allocate object to store instance information", MC_NORMAL_COMPLETION);
        return ( SAMP_FALSE );
    }

    memset( newNode, 0, sizeof(InstanceNode) );

    strncpy(newNode->fname, A_fname, sizeof(newNode->fname));
    newNode->fname[sizeof(newNode->fname)-1] = '\0';

    newNode->responseReceived = SAMP_FALSE;
    newNode->failedResponse = SAMP_FALSE;
    newNode->imageSent = SAMP_FALSE;
    newNode->msgID = -1;
    newNode->transferSyntax = IMPLICIT_LITTLE_ENDIAN;

    if ( !*A_list )
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

        while ( listNode->Next )
            listNode = listNode->Next;

        listNode->Next = newNode;
    }

    return ( SAMP_TRUE );
}


/****************************************************************************
 *
 *  Function    :   UpdateNode
 *
 *  Parameters  :   A_node     - node to update
 *
 *  Returns     :   SAMP_TRUE
 *                  SAMP_FALSE
 *
 *  Description :   Update an image node with info about a file transferred
 *
 ****************************************************************************/
static SAMP_BOOLEAN UpdateNode(InstanceNode* A_node)
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

    return ( SAMP_TRUE );
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
static void FreeList( InstanceNode**    A_list )
{
    InstanceNode*    node;

    /*
     * Free the instance list
     */
    while (*A_list)
    {
        node = *A_list;
        *A_list = node->Next;

        if ( node->msgID != -1 )
            MC_Free_Message(&node->msgID);

        free( node );
    }
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
static int GetNumNodes( InstanceNode*       A_list)

{
    int            numNodes = 0;
    InstanceNode*  node;

    node = A_list;
    while (node)
    {
        numNodes++;
        node = node->Next;
    }

    return numNodes;
}


/****************************************************************************
 *
 *  Function    :   GetNumOutstandingRequests
 *
 *  Parameters  :   A_list     - Pointer to head of node list to get count for
 *
 *  Returns     :   int, num messages we're waiting for c-store responses for
 *
 *  Description :   Checks the list of instances sent over the association &
 *                  returns the number of responses we're waiting for.
 *
 ****************************************************************************/
static int GetNumOutstandingRequests(InstanceNode* A_list)

{
    int            outstandingResponseMsgs = 0;
    InstanceNode*  node;

    node = A_list;
    while (node)
    {
        if ( ( node->imageSent == SAMP_TRUE ) && ( node->responseReceived == SAMP_FALSE ) )
            outstandingResponseMsgs++;

        node = node->Next;
    }
    return outstandingResponseMsgs;
}

/****************************************************************************
 *
 *  Function    :   ReadImage
 *
 *  Parameters  :   A_options  - Pointer to structure containing input
 *                               parameters to the application
 *                  A_appID    - Application ID registered
 *                  A_node     - The node in our list of instances
 *
 *  Returns     :   SAMP_TRUE
 *                  SAMP_FALSE
 *
 *  Description :   Determine the format of a DICOM file and read it into
 *                  memory.  Note that in a production application, the
 *                  file format should be predetermined (and not have to be
 *                  "guessed" by the CheckFileFormat function).  The
 *                  format for this application was chosen to show how both
 *                  DICOM Part 10 format files and "stream" format objects
 *                  can be sent over the network.
 *
 ****************************************************************************/
static SAMP_BOOLEAN ReadImage(STORAGE_OPTIONS* A_options, int A_appID, InstanceNode* A_node)
{
    FORMAT_ENUM             format = IMPLICIT_LITTLE_ENDIAN_FORMAT;
    SAMP_BOOLEAN            sampBool = SAMP_FALSE;
    MC_STATUS               mcStatus;

   
   
    A_node->mediaFormat = SAMP_FALSE;
    sampBool = ReadMessageFromFile( A_options, A_node->fname, format, &A_node->msgID, &A_node->transferSyntax, &A_node->imageBytes );
           

    if ( sampBool == SAMP_TRUE )
    {
        mcStatus = MC_Get_Value_To_String(A_node->msgID, MC_ATT_SOP_CLASS_UID, sizeof(A_node->SOPClassUID), A_node->SOPClassUID);
        if (mcStatus != MC_NORMAL_COMPLETION)
        {
            PrintError("MC_Get_Value_To_String for SOP Class UID failed", mcStatus);
        }

        mcStatus = MC_Get_Value_To_String(A_node->msgID, MC_ATT_SOP_INSTANCE_UID, sizeof(A_node->SOPInstanceUID), A_node->SOPInstanceUID);
        if (mcStatus != MC_NORMAL_COMPLETION)
        {
            PrintError("MC_Get_Value_To_String for SOP Instance UID failed", mcStatus);
        }
    }
    fflush(stdout);
    return sampBool;
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
static SAMP_BOOLEAN SendImage(STORAGE_OPTIONS* A_options, int A_associationID, InstanceNode* A_node)
{
    MC_STATUS       mcStatus;
    printf("4444444444444444444444444");
    A_node->imageSent = SAMP_FALSE;

    /* Get the SOP class UID and set the service */
    mcStatus = MC_Get_MergeCOM_Service(A_node->SOPClassUID, A_node->serviceName, sizeof(A_node->serviceName));
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("MC_Get_MergeCOM_Service failed", mcStatus);
        fflush(stdout);
        return ( SAMP_TRUE );
    }

    mcStatus = MC_Set_Service_Command(A_node->msgID, A_node->serviceName, C_STORE_RQ);
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("MC_Set_Service_Command failed", mcStatus);
        fflush(stdout);
        return ( SAMP_TRUE );
    }

    /* set affected SOP Instance UID */
    mcStatus = MC_Set_Value_From_String(A_node->msgID, MC_ATT_AFFECTED_SOP_INSTANCE_UID, A_node->SOPInstanceUID);
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("MC_Set_Value_From_String failed for affected SOP Instance UID", mcStatus);
        fflush(stdout);
        return ( SAMP_TRUE );
    }

    /*
     *  Send the message
     */
    if (A_options->Verbose)
    {
        printf("     File: %s\n", A_node->fname);

        if ( A_node->mediaFormat )
            printf("   Format: DICOM Part 10 Format(%s)\n", GetSyntaxDescription(A_node->transferSyntax));
        else
            printf("   Format: Stream Format(%s)\n", GetSyntaxDescription(A_node->transferSyntax));

        printf("SOP Class: %s (%s)\n", A_node->SOPClassUID, A_node->serviceName);
        printf("      UID: %s\n", A_node->SOPInstanceUID);
        printf("     Size: %lu bytes\n", (unsigned long) A_node->imageBytes);
    }

    mcStatus = MC_Send_Request_Message(A_associationID, A_node->msgID);
    if ((mcStatus == MC_ASSOCIATION_ABORTED) || (mcStatus == MC_SYSTEM_ERROR) || (mcStatus == MC_UNACCEPTABLE_SERVICE))
    {
        /* At this point, the association has been dropped, or we should drop it in the case of error. */
        PrintError("MC_Send_Request_Message failed", mcStatus);
        fflush(stdout);
        return ( SAMP_FALSE );
    }
    else if (mcStatus != MC_NORMAL_COMPLETION)
    {
        /* This is a failure condition we can continue with */
        PrintError("Warning: MC_Send_Request_Message failed", mcStatus);
        fflush(stdout);
        return ( SAMP_TRUE );
    }

    A_node->imageSent = SAMP_TRUE;
    fflush(stdout);

    return ( SAMP_TRUE );
}




/****************************************************************************
 *
 *  Function    :   ReadResponseMessages
 *
 *  Parameters  :   A_options  - Pointer to structure containing input
 *                               parameters to the application
 *                  A_associationID - Association ID registered
 *                  A_timeout  - 
 *                  A_list     - List of nodes among which to identify
 *                               the message for the request with which
 *                               the response is associated.
 *                  A_node     - A node in our list of instances.
 *
 *  Returns     :   SAMP_TRUE
 *                  SAMP_FALSE on failure where association must be aborted
 *
 *  Description :   Read the response message and determine which request
 *                  message is being responded to.
 *
 *                  SAMP_TRUE is returned on success, or when a recoverable
 *                  error occurs.
 *
 ****************************************************************************/
static SAMP_BOOLEAN ReadResponseMessages(STORAGE_OPTIONS*  A_options, int A_associationID, int A_timeout, InstanceNode** A_list, InstanceNode* A_node)
{
    MC_STATUS       mcStatus;
    SAMP_BOOLEAN    sampBool;
    int             responseMessageID;
    char*           responseService;
    MC_COMMAND      responseCommand;
    static char     affectedSOPinstance[UI_LENGTH+2];
    unsigned int    dicomMsgID;
    InstanceNode*   node = (InstanceNode*)A_node;
    printf("66666666666666");
    /*
     *  Wait for response
     */
    mcStatus = MC_Read_Message(A_associationID, A_timeout, &responseMessageID, &responseService, &responseCommand);
    if (mcStatus == MC_TIMEOUT)
        return ( SAMP_TRUE );

    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("MC_Read_Message failed", mcStatus);
        fflush(stdout);
        return ( SAMP_FALSE );
    }

    mcStatus = MC_Get_Value_To_UInt(responseMessageID, MC_ATT_MESSAGE_ID_BEING_RESPONDED_TO, &dicomMsgID);
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("MC_Get_Value_To_UInt for Message ID Being Responded To failed.  Unable to process response message.", mcStatus);
        fflush(stdout);
        return(SAMP_TRUE);
    }

    mcStatus = MC_Get_Value_To_String(responseMessageID, MC_ATT_AFFECTED_SOP_INSTANCE_UID, sizeof(affectedSOPinstance), affectedSOPinstance);
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("MC_Get_Value_To_String for affected SOP instance failed.  Unable to process response message.", mcStatus);
        fflush(stdout);
        return(SAMP_TRUE);
    }

    if (!A_options->StreamMode)
    {
        node = *A_list;
        while (node)
        {
            if ( node->dicomMsgID == dicomMsgID )
            {
                if (!strcmp(affectedSOPinstance, node->SOPInstanceUID))
                {
                    break;
                }
            }
            node = node->Next;
        }
    }

    if ( !node )
    {
        printf( "Message ID Being Responded To tag does not match message sent over association: %d\n", dicomMsgID );
        MC_Free_Message(&responseMessageID);
        fflush(stdout);
        return ( SAMP_TRUE );
    }

    node->responseReceived = SAMP_TRUE;

    sampBool = CheckResponseMessage ( responseMessageID, &node->status, node->statusMeaning, sizeof(node->statusMeaning) );
    if (!sampBool)
    {
        node->failedResponse = SAMP_TRUE;
    }

    if ( ( A_options->Verbose ) || ( node->status != C_STORE_SUCCESS ) )
        printf("   Status: %s\n", node->statusMeaning);

    node->failedResponse = SAMP_FALSE;

    mcStatus = MC_Free_Message(&responseMessageID);
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("MC_Free_Message failed for response message", mcStatus);
        fflush(stdout);
        return ( SAMP_TRUE );
    }
    fflush(stdout);
    return ( SAMP_TRUE );
}


/****************************************************************************
 *
 *  Function    :   CheckResponseMessage
 *
 *  Parameters  :   A_responseMsgID  - The message ID of the response message
 *                                     for which we want to check the status
 *                                     tag.
 *                  A_status         - Return argument for the response status
 *                                     value.
 *                  A_statusMeaning  - The meaning of the response status is
 *                                     returned here.
 *                  A_statusMeaningLength - The size of the buffer at
 *                                     A_statusMeaning.
 *
 *  Returns     :   SAMP_TRUE on success or warning status
 *                  SAMP_FALSE on failure status
 *
 *  Description :   Examine the status tag in the response to see if we
 *                  the C-STORE-RQ was successfully received by the SCP.
 *
 ****************************************************************************/
static SAMP_BOOLEAN CheckResponseMessage(int A_responseMsgID, unsigned int* A_status, char* A_statusMeaning, size_t A_statusMeaningLength )
{
    MC_STATUS mcStatus;
    SAMP_BOOLEAN returnBool = SAMP_TRUE;
    printf("7777777777777777777");
    mcStatus = MC_Get_Value_To_UInt ( A_responseMsgID, MC_ATT_STATUS, A_status );
    if ( mcStatus != MC_NORMAL_COMPLETION )
    {
        /* Problem with MC_Get_Value_To_UInt */
        PrintError ( "MC_Get_Value_To_UInt for response status failed", mcStatus );
        strncpy( A_statusMeaning, "Unknown Status", A_statusMeaningLength );
        fflush(stdout);
        return SAMP_FALSE;
    }

    /* MC_Get_Value_To_UInt worked.  Check the response status */

    switch ( *A_status )
    {
        /* Success! */
        case C_STORE_SUCCESS:
            strncpy( A_statusMeaning, "C-STORE Success.", A_statusMeaningLength );
            break;

        /* Warnings.  Continue execution. */

        case C_STORE_WARNING_ELEMENT_COERCION:
            strncpy( A_statusMeaning, "Warning: Element Coersion... Continuing.", A_statusMeaningLength );
            break;

        case C_STORE_WARNING_INVALID_DATASET:
            strncpy( A_statusMeaning, "Warning: Invalid Dataset... Continuing.", A_statusMeaningLength );
            break;

        case C_STORE_WARNING_ELEMENTS_DISCARDED:
            strncpy( A_statusMeaning, "Warning: Elements Discarded... Continuing.", A_statusMeaningLength );
            break;

        /* Errors.  Abort execution. */

        case C_STORE_FAILURE_REFUSED_NO_RESOURCES:
            strncpy( A_statusMeaning, "ERROR: REFUSED, NO RESOURCES.  ASSOCIATION ABORTING.", A_statusMeaningLength );
            returnBool = SAMP_FALSE;
            break;

        case C_STORE_FAILURE_INVALID_DATASET:
            strncpy( A_statusMeaning, "ERROR: INVALID_DATASET.  ASSOCIATION ABORTING.", A_statusMeaningLength );
            returnBool = SAMP_FALSE;
            break;

        case C_STORE_FAILURE_CANNOT_UNDERSTAND:
            strncpy( A_statusMeaning, "ERROR: CANNOT UNDERSTAND.  ASSOCIATION ABORTING.", A_statusMeaningLength );
            returnBool = SAMP_FALSE;
            break;

        case C_STORE_FAILURE_PROCESSING_FAILURE:
            strncpy( A_statusMeaning, "ERROR: PROCESSING FAILURE.  ASSOCIATION ABORTING.", A_statusMeaningLength );
            returnBool = SAMP_FALSE;
            break;

        default:
            sprintf( A_statusMeaning, "Warning: Unknown status (0x%04x)... Continuing.", *A_status );
            returnBool = SAMP_FALSE;
            break;
    }
    fflush(stdout);

    return returnBool;
}


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
static SAMP_BOOLEAN ReadMessageFromFile(STORAGE_OPTIONS*  A_options,
                                        char*             A_filename,
                                        FORMAT_ENUM       A_format,
                                        int*              A_msgID,
                                        TRANSFER_SYNTAX*  A_syntax,
                                        size_t*           A_bytesRead )
{
    MC_STATUS       mcStatus;
    unsigned long   errorTag = 0;
    CBinfo          callbackInfo = {0};
    int             retStatus = 0;
    printf("9999999999999999999999");
    /*
     * Determine the format
     */
    switch( A_format )
    {
        case IMPLICIT_LITTLE_ENDIAN_FORMAT:
            *A_syntax = IMPLICIT_LITTLE_ENDIAN;
            break;
        default:
            return SAMP_FALSE;
    }

    if (A_options->Verbose)
        printf("Reading DICOM \"stream\" format file in %s: %s\n", GetSyntaxDescription(*A_syntax), A_filename);

    /*
     * Open an empty message object to load the image into
     */
    mcStatus = MC_Open_Empty_Message(A_msgID);
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("Unable to open empty message", mcStatus);
        fflush(stdout);
        return SAMP_FALSE;
    }

    /*
     * Open and stream message from file
     */
    callbackInfo.fp = fopen(A_filename, BINARY_READ);
    if (!callbackInfo.fp)
    {
        printf("ERROR: Unable to open %s.\n", A_filename);
        MC_Free_Message(A_msgID);
        fflush(stdout);
        return SAMP_FALSE;
    }

    retStatus = setvbuf(callbackInfo.fp, (char *)NULL, _IOFBF, 32768);
    if ( retStatus != 0 )
    {
        printf("WARNING:  Unable to set IO buffering on input file.\n");
    }

    if (callbackInfo.bufferLength == 0)
    {
        int length = 0;
        
        mcStatus = MC_Get_Int_Config_Value(WORK_BUFFER_SIZE, &length);
        if (mcStatus != MC_NORMAL_COMPLETION)
        {
            length = WORK_SIZE;
        }
        callbackInfo.bufferLength =  length;
    }

    callbackInfo.buffer = malloc( callbackInfo.bufferLength );
    if( callbackInfo.buffer == NULL )
    {
        printf("ERROR: failed to allocate file read buffer [%d] kb", (int)callbackInfo.bufferLength);
        fflush(stdout);
        return SAMP_FALSE;
    }

    mcStatus = MC_Stream_To_Message(*A_msgID, 0x00080000, 0xFFFFFFFF, *A_syntax, &errorTag, (void*) &callbackInfo, StreamToMsgObj);
    
    if (callbackInfo.fp)
        fclose(callbackInfo.fp);

    if (callbackInfo.buffer)
        free(callbackInfo.buffer);

    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("MC_Stream_To_Message error, possible wrong transfer syntax guessed", mcStatus);
        MC_Free_Message(A_msgID);
        fflush(stdout);
        return SAMP_FALSE;
    }

    *A_bytesRead = callbackInfo.bytesRead;
    fflush(stdout);

    return SAMP_TRUE;

} /* ReadMessageFromFile() */

/*************************************************************************
 *
 *  Function    :  StreamToMsgObj
 *
 *  Parameters  :  A_msgID         - Message ID of message being read
 *                 A_CBinformation - user information passwd to callback
 *                 A_isFirst       - flag to tell if this is the first call
 *                 A_dataSize      - length of data read
 *                 A_dataBuffer    - buffer where read data is stored
 *                 A_isLast        - flag to tell if this is the last call
 *
 *  Returns     :  MC_NORMAL_COMPLETION on success
 *                 any other return value on failure.
 *
 *  Description :  Reads input stream data from a file, and places the data
 *                 into buffer to be used by the MC_Stream_To_Message function.
 *
 **************************************************************************/
static MC_STATUS NOEXP_FUNC StreamToMsgObj( int        A_msgID,
                                            void*      A_CBinformation,
                                            int        A_isFirst,
                                            int*       A_dataSize,
                                            void**     A_dataBuffer,
                                            int*       A_isLast)
{
    size_t          bytesRead;
    CBinfo*         callbackInfo = (CBinfo*)A_CBinformation;
    printf("1212121212");
    if (A_isFirst)
        callbackInfo->bytesRead = 0L;

    bytesRead = fread(callbackInfo->buffer, 1, callbackInfo->bufferLength, callbackInfo->fp);
    if (ferror(callbackInfo->fp))
    {
        perror("\tRead error when streaming message from file.\n");
        return MC_CANNOT_COMPLY;
    }

    if (feof(callbackInfo->fp))
    {
        *A_isLast = 1;
        fclose(callbackInfo->fp);
        callbackInfo->fp = NULL;
    }
    else
        *A_isLast = 0;

    *A_dataBuffer = callbackInfo->buffer;
    *A_dataSize = (int)bytesRead;
    callbackInfo->bytesRead += bytesRead;

    return MC_NORMAL_COMPLETION;
} /* StreamToMsgObj() */

/****************************************************************************
 *
 *  Function    :   PrintError
 *
 *  Description :   Display a text string on one line and the error message
 *                  for a given error on the next line.
 *
 ****************************************************************************/
static void PrintError(char* A_string, MC_STATUS A_status)
{
    char        prefix[30] = {0};
    /*
     *  Need process ID number for messages
     */
#ifdef UNIX
    sprintf(prefix, "PID %d", getpid() );
#endif
    if (A_status == -1)
    {
        printf("%s\t%s\n",prefix,A_string);
    }
    else
    {
        printf("%s\t%s:\n",prefix,A_string);
        printf("%s\t\t%s\n", prefix,MC_Error_Message(A_status));
    }
}


