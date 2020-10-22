#include "stor_scu.h"

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
SAMP_BOOLEAN TestCmdLine(int A_argc, char* A_argv[], STORAGE_OPTIONS* A_options)
{
    int       i = 0, argCount = 0;

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
        if (!strcmp(A_argv[i], "-h") || !strcmp(A_argv[i], "/h") ||
            !strcmp(A_argv[i], "-H") || !strcmp(A_argv[i], "/H") ||
            !strcmp(A_argv[i], "-?") || !strcmp(A_argv[i], "/?"))
        {
            PrintCmdLine();
            return SAMP_FALSE;
        }
        else if (!strcmp(A_argv[i], "-a") || !strcmp(A_argv[i], "-A"))
        {
            /*
             * Set the Local AE
             */
            i++;
            strcpy(A_options->LocalAE, A_argv[i]);
        }
        else if (!strcmp(A_argv[i], "-b") || !strcmp(A_argv[i], "-B"))
        {
            /*
             * Local Port Number
             */
            i++;
            A_options->ListenPort = atoi(A_argv[i]);

        }
        else if (!strcmp(A_argv[i], "-c") || !strcmp(A_argv[i], "-C"))
        {
            /*
             * StorageCommit mode
             */
            A_options->StorageCommit = SAMP_TRUE;
        }
        else if (!strcmp(A_argv[i], "-f") || !strcmp(A_argv[i], "-F"))
        {
            /*
             * Config file with filenames
             */
            i++;
            A_options->UseFileList = SAMP_TRUE;
            strcpy(A_options->FileList, A_argv[i]);
        }
        else if (!strcmp(A_argv[i], "-l") || !strcmp(A_argv[i], "-L"))
        {
            /*
             * Service List
             */
            i++;
            strcpy(A_options->ServiceList, A_argv[i]);
        }
        else if (!strcmp(A_argv[i], "-n") || !strcmp(A_argv[i], "-N"))
        {
            /*
             * Remote Host Name
             */
            i++;
            strcpy(A_options->RemoteHostname, A_argv[i]);
        }
        else if (!strcmp(A_argv[i], "-p") || !strcmp(A_argv[i], "-P"))
        {
            /*
             * Remote Port Number
             */
            i++;
            A_options->RemotePort = atoi(A_argv[i]);

        }
        else if (!strcmp(A_argv[i], "-q") || !strcmp(A_argv[i], "-Q"))
        {
            /*
             * Positive response requested from server.
             */
            A_options->ResponseRequested = SAMP_TRUE;
        }
        else if (!strcmp(A_argv[i], "-s") || !strcmp(A_argv[i], "-S"))
        {
            /*
             * Stream mode
             */
            A_options->StreamMode = SAMP_TRUE;
        }
        else if (!strcmp(A_argv[i], "-u") || !strcmp(A_argv[i], "-U"))
        {
            /*
             * Username
             */
            i++;
            strcpy(A_options->Username, A_argv[i]);
        }
        else if (!strcmp(A_argv[i], "-v") || !strcmp(A_argv[i], "-V"))
        {
            /*
             * Verbose mode
             */
            A_options->Verbose = SAMP_TRUE;
        }
        else if (!strcmp(A_argv[i], "-w") || !strcmp(A_argv[i], "-W"))
        {
            /*
             * Username
             */
            i++;
            strcpy(A_options->Password, A_argv[i]);
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
                printf("Unkown option: %s\n", A_argv[i]);
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
    if (A_options->RemoteHostname[0]
        && !A_options->ServiceList[0]
        && (A_options->RemotePort != -1))
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