# For email utility:
## Description:
This is designed to send a email to radiologists when a new image is received

## usage:

- Run WatchDog.py from email directory
- After that specify sender and receiver mail in config.py
- Turn on option specified in below link
https://myaccount.google.com/lesssecureapps?pli=1&rapt=AEjHL4N7SJHvNSopLh6047nQeXAmWFr0eW-LdSXc5dz15wm6iCBsrg9wWnx2xOK0JBf9yn3O-uuGUTJ1soFeJoJNr-6p8SAE_g

# For image_tracker:
## Description:
This module is designed for keep tracking of images generated and their status. It checks for new images and append it to csv file. It keep track of image status in a csv file. Whenever a user change the status of a image it is stored in Data.csv. 

## usage:

- Specify the address of image files and address of csv file in config file 
- Then run track_images.py 

