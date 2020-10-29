import os
import pandas as pd
import sys
import config


def print_to_stderr(*a):
    print(*a, file=sys.stderr)


# checks if the csv file is present
if not os.path.isfile(config.ADDRESS_OF_DATA_CSV_FILE):
    print_to_stderr("No csv file with that name at the given location")
    print_to_stderr("Once check if file name and path is correct in config.py file")
    print_to_stderr("If no csv is present create a new csv file, update the config file and re-run the program")
    exit()

# check if the img path exists or not
if not os.path.exists(config.ADDRESS_OF_IMG_FILES):
    print_to_stderr("Path Doesn't exists. Please re-check the path in config file!!!")
    exit()


# Adds an img file to csv and throws and exception if csv is read only
def add_filename_to_csv(fileName):
    image_list = pd.read_csv(config.ADDRESS_OF_DATA_CSV_FILE)
    new_row = {'FileName': fileName, 'Status': 'Pending for diagnosis'}
    image_list = image_list.append(new_row, ignore_index=True)
    try:
        image_list.to_csv(config.ADDRESS_OF_DATA_CSV_FILE, index=False)
    except PermissionError:
        print_to_stderr("Permission Denied!! Read only csv file. Can to write mode and try")
    
    
# A level of abstraction is implemented adds the file in files list to csv file
def add_the_list_to_file(files_list):
    for i in files_list:
        add_filename_to_csv(i)
            
# checks if the new img file can be append to current image list
def check_if_imgfile_can_be_append_to_current_image_list(i, current_image_list):
    if i not in current_image_list:
        return True
    else:
        return False

# check if new file is an img
def check_if_it_is_a_img(split_str):
    if len(split_str) == 2:
        if split_str[1] == "img":
            return True
        else:
            return False


# Appends files in it is not present in current image list
def check_in_currennt_image_list_and_append(i, current_image_list, new_image_files):
    if check_if_imgfile_can_be_append_to_current_image_list(i, current_image_list):
        new_image_files.append(str(i))
    return new_image_files


# check for new img files and returns a list of new img files
def check_for_new_imgfiles():
    files_list = os.listdir(config.ADDRESS_OF_IMG_FILES)
    image_list = pd.read_csv(config.ADDRESS_OF_DATA_CSV_FILE)
    current_image_list = image_list['FileName'].values.tolist()
    new_image_files = []
    for i in files_list:
        split_str = i.split(".")
        is_a_img = check_if_it_is_a_img(split_str)
        if is_a_img:
            new_image_files = check_in_currennt_image_list_and_append(i, current_image_list, new_image_files)

    return new_image_files


# shows all the img files in csv files
def show_all_img_with_status():
    image_list = pd.read_csv(config.ADDRESS_OF_DATA_CSV_FILE)
    print(image_list)

# update the status of the img file with the given status
def update_file_status_in_csv(fileName, status):
    update_string = status
    image_list = pd.read_csv(config.ADDRESS_OF_DATA_CSV_FILE)
    image_list.loc[image_list['FileName'] == fileName, 'Status'] = update_string
    image_list.to_csv(config.ADDRESS_OF_DATA_CSV_FILE, index=False)


# Level of abstraction implemented. Ask for fileName and what it needs to be updated with
def update_status():
    show_all_img_with_status()
    imageName = input("Enter the complete name of the file to be updated ")
    inputStr = input("Is this image diagnosed (Diagnosed/Pending for Diagnosis)? ")
    update_file_status_in_csv(imageName, inputStr)

# check files_list and if it is not empty pass the files list to add the list to file function
def check_files_list_and_add_to_file(files_list):
    if files_list:
        add_the_list_to_file(files_list)
    else:
        print("No new img files in the directory that are yet to be added to csv")

def menu():
    while 1:
        choice = input("press 1 to update and any key other than 1 to exit the application : ")
        if choice == "1":
            files_list = check_for_new_imgfiles()
            check_files_list_and_add_to_file(files_list)
            update_status()
        else:
            break


if __name__ == "__main__":
    print("Welcome to Image Tracker!!!")
    menu()

