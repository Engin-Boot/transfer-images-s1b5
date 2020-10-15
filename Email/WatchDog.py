import time
import smtplib
import config

from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler


class OnMyWatch:
    # Set the directory on watch
    watchDirectory = "C:/Users/hp/Pictures/Philips_BootCamp/Case_Study/DICOM/mc3apps"

    def __init__(self):
        self.observer = Observer()

    def run(self):
        event_handler = Handler()
        self.observer.schedule(event_handler, self.watchDirectory, recursive=True)
        self.observer.start()
        try:
            while True:
                time.sleep(5)
        except:
            self.observer.stop()
            print("Observer Stopped")

        self.observer.join()


class Handler(FileSystemEventHandler):

    @staticmethod
    def on_any_event(event):
        if event.is_directory:
            return None

        elif event.event_type == 'created':
            # Event is created, you can process it now
            print("Watchdog received created event - % s." % event.src_path)

            def send_email(subject, msg):
                try:
                    server = smtplib.SMTP('smtp.gmail.com:587')
                    server.ehlo()
                    server.starttls()
                    server.login(config.EMAIL_ADDRESS1, config.PASSWORD)
                    message = 'Subject: {}\n\n{}'.format(subject, msg)
                    server.sendmail(config.EMAIL_ADDRESS1, config.EMAIL_ADDRESS2, message)
                    server.quit()
                    print("Success: Email sent!")
                except:
                    print("Email failed to send.")

            subject = "DICOM Image Sent"
            msg = "Hello," \
                  "\n\n" \
                  "This mail is to inform you that the image to be diagnosed is sent with patient name and patient ID." \
                  "\n\n" \
                  "Thank You!"

            send_email(subject, msg)
        

if __name__ == '__main__':
    watch = OnMyWatch()
    watch.run()
