#include "stor_scu.h"
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