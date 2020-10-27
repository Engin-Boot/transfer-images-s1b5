import os
import pandas as pd
import sys
import config


def print_to_stderr(*a):
    print(*a, file=sys.stderr)


# check if the img path exists or not

def check():
    if os.path.exists(config.ADDRESS_OF_IMG_FILES):
        print("Welcome to Image Tracker!!!")
    else:
        print_to_stderr("Path Doesn't exists. Please re-check the path in config file!!!")
        exit()


# Adds an img file to csv and returns true if successfully added and false if the operation is failed
def add_filename_to_csv(fileName):
    image_list = pd.read_csv(config.ADDRESS_OF_DATA_CSV_FILE)
    count_before_adding_row = len(image_list.axes[0])
    new_row = {'FileName': fileName, 'Status': 'Pending for diagnosis'}
    image_list = image_list.append(new_row, ignore_index=True)
    image_list.to_csv(config.ADDRESS_OF_DATA_CSV_FILE, index=False)
    count_after_adding_row = len(image_list.axes[0])
    if count_after_adding_row - count_before_adding_row == 1:
        return True
    else:
        return False


def check_for_add_the_list(count, files_list_len):
    if count == files_list_len:
        return True
    else:
        return False


# A level of abstraction is implemented and returns true if all the img files are added, and false upon failing
def add_the_list(files_list):
    count = 0
    files_list_len = len(files_list)
    for i in files_list:
        if add_filename_to_csv(i):
            count += 1
    return check_for_add_the_list(count, files_list_len)


# check if the new img file is in current image list and returns true if not present
def check_in_current_image_list(i, current_image_list):
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
    if check_in_current_image_list(i, current_image_list):
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
def update_file_status(fileName, status):
    update_string = status
    image_list = pd.read_csv(config.ADDRESS_OF_DATA_CSV_FILE)
    image_list.loc[image_list['FileName'] == fileName, 'Status'] = update_string
    image_list.to_csv(config.ADDRESS_OF_DATA_CSV_FILE, index=False)


# Level of abstraction implemented. Ask for fileName and what it needs to be updated with
def update_status():
    show_all_img_with_status()
    imageName = input("Enter the complete name of the file to be updated ")
    inputStr = input("Is this image diagnosed (Diagnosed/Pending for Diagnosis)? ")
    update_file_status(imageName, inputStr)


def check_for_is_added_is_true(is_added):
    if is_added:
        print("All the new img files have been added to csv")
    else:
        print("Few or all the new img files have not been added to csv")
        print("Check if the CSV has reached maximum limt")
        print("If maximum limt reached create new csv and change the config file")


# check files_list and if it is not empty add those files
def check_file_list_and_add(files_list):
    if files_list:
        is_added = add_the_list(files_list)
        check_for_is_added_is_true(is_added)
    else:
        print("No new img files in the directory that are yet to be added to csv")


# no return required
def menu():
    while 1:
        choice = input("press 1 to update and any key other than 1 to exit the application : ")
        if choice == "1":
            files_list = check_for_new_imgfiles()
            check_file_list_and_add(files_list)
            update_status()
        else:
            break


if __name__ == "__main__":
    if os.path.isfile(config.ADDRESS_OF_DATA_CSV_FILE):
        check()
        menu()
    else:
        print_to_stderr("No csv file with that name at the given location")
        print_to_stderr("Once check if file name and path is correct in config.py file")
        print_to_stderr("If no csv is present create a new csv file, update the config file and re-run the program")
        exit()
