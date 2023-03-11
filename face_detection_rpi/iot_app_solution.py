from face_detector import *
import time 
import keyboard

TIMEOUT = 10 # seconds

if __name__ == '__main__':

    # init face detector
    fd = FaceDetector()

    while True:
        # TODO: check if button is clicked
        if input() == 'c':
            # start face recognition 
            fd.start_face_recognition()

            
            # check if the face recognition is finished or the time is out !
            # if not true continue waiting (looping) 
            prev_time = time.time()
            while (not fd.is_recognition_finished() and (time.time() - prev_time) < TIMEOUT):
                prev_time = time.time()

                # TODO: toggle the led and wait for 200ms
                print("sleeping for 200ms, recognition in progress...")
                time.sleep(0.2)

            # if the recognition is successful publish success, otherwize publish failure
            if fd.is_recognition_successful() :
                recognized_names = fd.get_detected_faces()
                # do not forget to stop the algorithm as soon as you get the names 
                fd.reset_face_recognition()
                # TODO: publish  success
                print(f'some faces were detected ! -> {recognized_names}')
                # TODO: turn on the green led
                time.sleep(5) 
                # TODO: tun off the green led 
            else :
                # do not forget to stop the algorithm as soon as possible 
                fd.reset_face_recognition()
                # TODO: publish  failure
                print('face recognition failed, request from unknown person !')
                # TODO: turn on the red led
                time.sleep(5) 
                # TODO: tun off the red led 
        else : 
            time.sleep(0.05) # sleep for a short period before reading the button again (reduces CPU usage)  
       
        



