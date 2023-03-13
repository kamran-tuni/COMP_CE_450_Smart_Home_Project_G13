from face_detector import *
import time 
import keyboard


# magic numbers go here:
TIMEOUT = 10 # seconds
LED_GREEN_BLINKING_TIME = 0.2
LED_GREEN_TURN_ON_TIME = 5
LED_RED_TURN_ON_TIME = 5


led_status = False 

# simulates xor function to toggle led pin
def toggle_green_led():
    global led_status

    if led_status == True :
        led_status = False 
        print('GREEN LED OFF')
    else :
        led_status = True 
        print('GREEN LED ON')

if __name__ == '__main__':


    # init face detector
    fd = FaceDetector(video_source=0) #WARNING: change source according to your OS


    while True:
        # TODO: check if button is clicked
        
        if input("enter the letter 'c' to simulate a button click: ") == 'c':
            # start face recognition 
            fd.start_face_recognition()

            
            # check if the face recognition is finished or the time is out !
            # if not true continue waiting (looping) 
            prev_time = time.time()
            while (not fd.is_recognition_finished() and (time.time() - prev_time) < TIMEOUT):
                
                # TODO: toggle the led and wait for 200ms
                # toggle means if it was on put it off, or if it was off make it on, not both
                toggle_green_led()
                print("sleeping for 200ms, recognition in progress...")
                time.sleep(LED_GREEN_BLINKING_TIME)

            # if the recognition is successful publish success, otherwize publish failure
            if fd.is_recognition_successful() :
                recognized_names = fd.get_detected_faces()
                # do not forget to stop the algorithm as soon as you get the names 
                fd.reset_face_recognition()
                # TODO: publish  success
                print(f'some faces were detected ! -> {recognized_names}')
                # TODO: turn on the green led
                time.sleep(LED_GREEN_TURN_ON_TIME) 
                # TODO: tun off the green led 
            else :
                # do not forget to stop the algorithm as soon as possible 
                fd.reset_face_recognition()
                # TODO: publish  failure
                print('TIMOUT ERROR: face recognition failed, request from unknown person !')
                # TODO: turn on the red led
                time.sleep(LED_RED_TURN_ON_TIME) 
                # TODO: tun off the red led 
        else : 
            time.sleep(0.05) # sleep for a short period before reading the button again (reduces CPU usage)  
       
        



