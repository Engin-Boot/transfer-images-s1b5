# InstallationManual
## About
This application is a Service Class User(SCU) built over DICOM Storage Service Class supported by MergeCom toolkit. SCU sends message to a DICOM SCP (Service Class Provider). Any third party SCP can be used along with this SCU. SCU acts as a client and SCP as a server which receives message from client.

Storage Service Class defines context of transfer of images from one DICOM application entity to another. This Storage Service does not specify that the receiver of the images take ownership for the safekeeping of the images. In this SCU storage commitment from SCP can not be negotiated.

To Transfer images SCU does the following actions:

Open Association with third party SCP
Read DICOM data to be transferred
Format data to message format
Send image to SCP
Close association
## Setup
The following steps can be followed to run this SCU:

Build stor_scu.vcxproj, which is in SCU directory of this repo, using msbuild by setting the configuration as Debug/x64.
Copy all files from mc3lib directory to the SCU directory
copy stor_scu.exe from x64Debug folder to scu directory

## Execution
Execute the off the shelf scp
Now open mergecom.pro and enter the license key in LICENSE.

In command prompt reach path to stor_scu.exe, now enter command stor_scu a list of command syntax of features supported will be displayed. Enter the command you want. The media file(.img) to be sent should be in the same directory. Example:

stor_scu MERGE_STORE_SCP 0 1: This command will send file 0.img to 1.img to this SCP
