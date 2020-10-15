#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"
#include "scutest.h"
#include<string.h>

TEST_CASE("Test to check copying of default servicelist") {
    STORAGE_OPTIONS A_options;
    dicom_scu::Copy_ServiceList(&A_options);
    string s(A_options.ServiceList);
    int i=(s).compare("Storage_SCU_Service_List");
    REQUIRE(i==0);
    }
    
TEST_CASE("when RemoteHostname and Remote port are not set"){
    STORAGE_OPTIONS A_options;
 //int i = scu::CheckHostDetails(&A_options);
REQUIRE(dicom_scu::CheckHostDetails(&A_options) == SAMP_FALSE);
}

TEST_CASE("when RemoteHostname and Remote port are set") {
    STORAGE_OPTIONS A_options;
    strcpy(A_options.RemoteHostname,"localhost");
    A_options.RemotePort = 4;
    REQUIRE(dicom_scu::CheckHostDetails(&A_options) == SAMP_TRUE);
}

TEST_CASE("When MC_Stream_To_Message syntax used is wrong") {
    int * A_msgID;
    MC_STATUS mcStatus;
    REQUIRE(dicom_scu::ReadMessageFromFileStreamError(mcStatus, A_msgID) == 0);
}

TEST_CASE("when no of nodes is empty the function getnumnodes returns 0")
{
    STORAGE_OPTIONS A_options;
    char fname[512] = { 0 };
    A_options.StartImage = 10;
    A_options.StopImage = 0;
    InstanceNode* instanceList = NULL;
    dicom_scu::AddInstancesToFile(A_options, &instanceList, fname);

    REQUIRE(dicom_scu::GetNumNodes(instanceList) == 0);

}


TEST_CASE("when no of nodes is non empty the function getnumnodes returns no of nodes present")
{
    STORAGE_OPTIONS A_options;
    char fname[512] = { 0 };
    A_options.StartImage = 0;
    A_options.StopImage = 0;
    InstanceNode* instanceList = NULL;
    dicom_scu::AddInstancesToFile(A_options, &instanceList, fname);

    REQUIRE(dicom_scu::GetNumNodes(instanceList) == 1);

}

TEST_CASE("travese node if start and stop numbers are correct") {
    STORAGE_OPTIONS A_options;
    char fname[512] = { 0 };
    A_options.StartImage = 0;
    A_options.StopImage = 0;
    InstanceNode* instanceList = NULL;

    REQUIRE(dicom_scu::AddInstancesToFile(A_options, &instanceList, fname) == SAMP_TRUE);
}

TEST_CASE("donot travese node if start and stop numbers are incorrect") {
    STORAGE_OPTIONS A_options;
    char fname[512] = { 0 };
    A_options.StartImage = 10;
    A_options.StopImage = 0;
    InstanceNode* instanceList = NULL;

    REQUIRE(dicom_scu::AddInstancesToFile(A_options, &instanceList, fname) == SAMP_FALSE);
}

TEST_CASE("incement imagesent variable only when the node has imageSent field true")
{
    InstanceNode*    newNode1;
    newNode1 = (InstanceNode*)malloc(sizeof(InstanceNode));
    memset(newNode1, 0, sizeof(InstanceNode));
    strncpy(newNode1->fname, "0.img", sizeof(newNode1->fname));
    newNode1->fname[sizeof(newNode1->fname) - 1] = '\0';
    newNode1->imageSent = SAMP_TRUE;
    int imagesSent = dicom_scu::ImageSentCount(newNode1);
    REQUIRE(imagesSent == 1);
}

TEST_CASE("donot incement imagesent variable when the node has imageSent field false")
{
    InstanceNode*    newNode1;
    newNode1 = (InstanceNode*)malloc(sizeof(InstanceNode));
    memset(newNode1, 0, sizeof(InstanceNode));
    strncpy(newNode1->fname, "0.img", sizeof(newNode1->fname));
    newNode1->fname[sizeof(newNode1->fname) - 1] = '\0';
    newNode1->imageSent = SAMP_FALSE;
    int imagesSent = dicom_scu::ImageSentCount(newNode1);
    REQUIRE(imagesSent == 0);
}

TEST_CASE("File is opened and initial input buffer is set") {
    CBinfo callbackInfo;
    char* A_filename;
    int* A_msgID;
    REQUIRE(dicom_scu::ReadMessageFromFileOpenNStream(A_filename, A_msgID) == SAMP_FALSE);
}

int listLength(InstanceNode* item)
{
    InstanceNode* cur = item;
    int size = 0;

    while (cur != NULL)
    {
        ++size;
        cur = cur->Next;
    }

    return size;
}

TEST_CASE("Memory allocated is released") {
    InstanceNode* A_list;
    A_list = (InstanceNode*)malloc(sizeof(InstanceNode));
    memset(A_list, 0, sizeof(InstanceNode));
    strncpy(A_list->fname, "0.img", sizeof(A_list->fname));
    dicom_scu::FreeList(&A_list);
    REQUIRE(listLength(A_list) == 0);
}
