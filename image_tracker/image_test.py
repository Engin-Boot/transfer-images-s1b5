import unittest
from track_images import *


class Image_Tracker(unittest.TestCase):

    def test_check_if_imgfile_can_be_append_to_current_image_list(self):
        current_image_list = ["2.img"]
        bool1 = check_if_imgfile_can_be_append_to_current_image_list("1.img", current_image_list)
        self.assertEqual(bool1, True)

    def test_check_if_imgfile_cannot_be_append_to_current_image_list(self):
        current_image_list = ["1.img"]
        bool1 = check_if_imgfile_can_be_append_to_current_image_list("1.img", current_image_list)
        self.assertEqual(bool1, False)

    def test_check_if_file_is_a_image(self):
        str = ["2", "img"]
        bool1 = check_if_it_is_a_img(str)
        self.assertEqual(bool1, True)

    def test_check_if_file_is_not_a_image(self):
        str = ["2", "im"]
        bool1 = check_if_it_is_a_img(str)
        self.assertEqual(bool1, False)

    def test_append_in_new_image_files_if_file_is_not_present_in_current_list(self):
        file = "2.img"
        current_list = ["1.img", "3.img"]
        new_image_files = []
        new = check_in_currennt_image_list_and_append(file, current_list, new_image_files)
        self.assertEqual(new, ["2.img"])

    def test_not_append_in_new_image_files_if_file_is_present_in_current_list(self):
        file = "1.img"
        current_list = ["1.img", "3.img"]
        new_image_files = []
        new = check_in_currennt_image_list_and_append(file, current_list, new_image_files)
        self.assertEqual(new, [])


if __name__ == '__main__':
    unittest.main()
