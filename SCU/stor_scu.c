#include "stor_scu.h"
extern SAMP_BOOLEAN TestCmdLine(int A_argc, char* A_argv[], STORAGE_OPTIONS* A_options);
extern SAMP_BOOLEAN AddFileToList(InstanceNode** A_list, char* A_fname);
extern void PrintCmdLine(void);
extern SAMP_BOOLEAN UpdateNode(InstanceNode* A_node);
extern void FreeList(InstanceNode** A_list);
extern int GetNumNodes(InstanceNode* A_list);
extern int GetNumOutstandingRequests(InstanceNode* A_list);
extern SAMP_BOOLEAN ReadImage(STORAGE_OPTIONS* A_options, int A_appID, InstanceNode* A_node);
extern SAMP_BOOLEAN SendImage(STORAGE_OPTIONS* A_options, int A_associationID, InstanceNode* A_node);
extern SAMP_BOOLEAN CheckResponseMessage(int A_responseMsgID, unsigned int* A_status, char* A_statusMeaning, size_t A_statusMeaningLength);
extern SAMP_BOOLEAN ReadResponseMessages(STORAGE_OPTIONS* A_options, int A_associationID, int A_timeout, InstanceNode** A_list, InstanceNode* A_node);
extern MC_STATUS NOEXP_FUNC StreamToMsgObj(int        A_msgID,
    void* A_CBinformation,
    int        A_isFirst,
    int* A_dataSize,
    void** A_dataBuffer,
    int* A_isLast);
extern SAMP_BOOLEAN ReadMessageFromFile(STORAGE_OPTIONS* A_options,
    char* A_filename,
    FORMAT_ENUM       A_format,
    int* A_msgID,
    TRANSFER_SYNTAX* A_syntax,
    size_t* A_bytesRead);
extern void PrintError(char* A_string, MC_STATUS A_status);
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













