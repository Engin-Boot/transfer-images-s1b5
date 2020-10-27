import os
import pandas as pd
import sys
import config

if not os.path.isfile(config.ADDRESS_OF_DATA_CSV_FILE):
    print("No csv file with that name at the given location")
    print("Create a new csv file if file not present and re-run the program with that file name")
    print("Else re-run the program with correct location and file name")
    exit()


# return type possible
def add_filename_to_csv(fileName):
    image_list = pd.read_csv(config.ADDRESS_OF_DATA_CSV_FILE)
    new_row = {'FileName': fileName, 'Status': 'Pending for diagnosis'}
    image_list = image_list.append(new_row, ignore_index=True)
    image_list.to_csv(config.ADDRESS_OF_DATA_CSV_FILE, index=False)


# return type possible
def check_in_current_image_list_and_add(i, current_image_list):
    if i not in current_image_list:
        add_filename_to_csv(i)


# done checking
def check_if_it_is_a_img(split_str):
    if len(split_str) == 2:
        if split_str[1] == "img":
            return True
        else:
            return False


# no return type
def check_for_new_imgfiles(path):
    files_list = os.listdir(path)
    image_list = pd.read_csv(config.ADDRESS_OF_DATA_CSV_FILE)
    current_image_list = image_list['FileName'].values.tolist()

    for i in files_list:
        split_str = i.split(".")
        is_a_img = check_if_it_is_a_img(split_str)
        if is_a_img:
            check_in_current_image_list_and_add(i, current_image_list)


# no return type
def show_all_img_with_status():
    image_list = pd.read_csv(config.ADDRESS_OF_DATA_CSV_FILE)
    print(image_list)


# return type possible
def update_file_status(fileName, status):
    update_string = "Diagnosed"
    image_list = pd.read_csv(config.ADDRESS_OF_DATA_CSV_FILE)
    if status != "yes":
        update_string = "Pending for Diagnosis"
    image_list.loc[image_list['FileName'] == fileName, ('Status')] = update_string
    image_list.to_csv(config.ADDRESS_OF_DATA_CSV_FILE, index=False)
    image_row = image_list[image_list['FileName'] == fileName]
    image_status = image_row['Status']


# no return type
def update_status():
    show_all_img_with_status()
    imagename = input("Enter the complete name of the file to be updated ")
    inputstr = input("Is this image diagnosed (yes/no)? ")
    if (input == "yes"):
        update_file_status(imagename, inputstr)
    else:
        update_file_status(imagename, inputstr)


# no return required
def menu (path):
    while 1:

        choice = input("press 1 to update and any key other th1an 1 to exit the application : ")
        if choice == "1":
            check_for_new_imgfiles(path)
            update_status()
        else:
            break


# no return type
def print_to_stderr(*a):
    print(*a, file=sys.stderr)


if __name__ == "__main__":

    path = config.ADDRESS_OF_IMG_FILES
    isExist = os.path.exists(path)
    if (isExist):
        menu(path)
    else:
        print_to_stderr("Path Doesn't exists. Please re-enter the correct path!!!")
