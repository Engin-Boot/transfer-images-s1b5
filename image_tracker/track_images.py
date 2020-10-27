import os
import pandas as pd 
import sys
import config

#status_filename_with_location = "Data.csv"

if(not os.path.isfile(config.ADDRESS_OF_DATA_CSV_FILE)):
    print("No csv file with that name at the given location")
    print("Create a new csv file if file not present and re-run the program with that file name")
    print("Else re-run the program with correct location and file name")
    exit()


# return type possible
def add_filename_to_csv(fileName):
    image_list = pd.read_csv(config.ADDRESS_OF_DATA_CSV_FILE)
    count_before_adding_row = len(image_list.axes[0])
    #print("count_before_adding_row:" ,count_before_adding_row)
    new_row = {'FileName': fileName, 'Status':'Pending for diagnosis'}
    image_list = image_list.append(new_row,ignore_index = True)
    image_list.to_csv(config.ADDRESS_OF_DATA_CSV_FILE, index=False)
    count_after_adding_row = len(image_list.axes[0])
    #print("count after adding row :", count_after_adding_row)
    if (count_after_adding_row-count_before_adding_row == 1):
        return True
    else:
        return False
        


def add_the_list(files_list):
    count =0;
    files_list_len = len(files_list)
    for i in files_list:
        if (add_filename_to_csv(i)):
            count+=1
    if (count == files_list_len):
        return True
    else:
        return False

# return type possible
def check_in_current_image_list(i,current_image_list):
    if(i not in current_image_list):
        return True
    else:
        return False

# done checking 
def check_if_it_is_a_img(split_str):
    if(len(split_str)==2):
        if(split_str[1]=="img"):
            return True
        else:
            return False
        

# no return type 
def check_for_new_imgfiles(path):
    files_list = os.listdir(path)
    image_list = pd.read_csv(config.ADDRESS_OF_DATA_CSV_FILE)
    current_image_list = image_list['FileName'].values.tolist()
    new_image_files=[]
    for i in files_list:
        split_str = i.split(".")
        is_a_img = check_if_it_is_a_img(split_str)
        if(is_a_img):
            is_not_in_current_image_list = check_in_current_image_list(i,current_image_list)
            if(is_not_in_current_image_list):
                new_image_files.append(str(i))
    return new_image_files
            

# no return type      
def show_all_img_with_status():
    image_list = pd.read_csv(config.ADDRESS_OF_DATA_CSV_FILE)
    print(image_list)

# return type possible    
def update_file_status(fileName, status):
    update_string = "Diagnosed"
    image_list = pd.read_csv(config.ADDRESS_OF_DATA_CSV_FILE)
    if(status != "yes"):
        update_string = "Pending for Diagnosis"
    image_list.loc[image_list['FileName'] == fileName, ('Status')] = update_string
    image_list.to_csv(config.ADDRESS_OF_DATA_CSV_FILE, index=False)
    image_row = image_list[image_list['FileName']==fileName]
    image_status = image_row['Status']    
    
   
#no return type 
def update_status():
    show_all_img_with_status()
    imageName = input("Enter the complete name of the file to be updated ")
    inputStr = input("Is this image diagnosed (yes/no)? ")
    if(input == "yes"):
        update_file_status(imageName , inputStr)
    else:
        update_file_status(imageName , inputStr)
    

# no return required
def menu(path):
    while 1:
        
        choice = input("press 1 to update and any key other than 1 to exit the application : ")
        if (choice == "1") :
            files_list =check_for_new_imgfiles(path)
            if files_list:
                is_added = add_the_list(files_list)
                if (is_added):
                    print ("All the new img files have to added to csv")
                else:
                    print("Few or all the new img files have not been added to csv")
            else:
                print("No new img files in the directory that are yet to be added to csv")
            update_status()
        else:
            break


def print_to_stderr(*a): 
    print(*a, file = sys.stderr)

if __name__ == "__main__":
     path = config.ADDRESS_OF_IMG_FILES
     isExist = os.path.exists(path)
     if (isExist):
         menu(path)
     else:
         print_to_stderr("Path Doesn't exists. Please re-enter the correct path!!!")
         