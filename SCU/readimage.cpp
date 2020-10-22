#include "stor_scu.h"
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
 SAMP_BOOLEAN ReadMessageFromFile(STORAGE_OPTIONS*  A_options,
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
     
     *A_syntax = IMPLICIT_LITTLE_ENDIAN;
         
    

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

SAMP_BOOLEAN ReadImage(STORAGE_OPTIONS* A_options, int A_appID, InstanceNode* A_node)
{
    FORMAT_ENUM             format = IMPLICIT_LITTLE_ENDIAN_FORMAT;
    SAMP_BOOLEAN            sampBool = SAMP_FALSE;
    MC_STATUS               mcStatus;



    A_node->mediaFormat = SAMP_FALSE;
    sampBool = ReadMessageFromFile(A_options, A_node->fname, format, &A_node->msgID, &A_node->transferSyntax, &A_node->imageBytes);


    if (sampBool == SAMP_TRUE)
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
