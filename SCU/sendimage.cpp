#include "stor_scu.h"
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
    if (mcStatus != MC_NORMAL_COMPLETION)
    {
        PrintError("MC_Set_Value_From_String failed for affected SOP Instance UID", mcStatus);
        fflush(stdout);
        return (SAMP_TRUE);
    }

    /*
     *  Send the message
     */
    if (A_options->Verbose)
    {
        printf("     File: %s\n", A_node->fname);

        if (A_node->mediaFormat)
            printf("   Format: DICOM Part 10 Format(%s)\n", GetSyntaxDescription(A_node->transferSyntax));
        else
            printf("   Format: Stream Format(%s)\n", GetSyntaxDescription(A_node->transferSyntax));

        printf("SOP Class: %s (%s)\n", A_node->SOPClassUID, A_node->serviceName);
        printf("      UID: %s\n", A_node->SOPInstanceUID);
        printf("     Size: %lu bytes\n", (unsigned long)A_node->imageBytes);
    }

    mcStatus = MC_Send_Request_Message(A_associationID, A_node->msgID);
    if ((mcStatus == MC_ASSOCIATION_ABORTED) || (mcStatus == MC_SYSTEM_ERROR) || (mcStatus == MC_UNACCEPTABLE_SERVICE))
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

    A_node->imageSent = SAMP_TRUE;
    fflush(stdout);

    return (SAMP_TRUE);
}
