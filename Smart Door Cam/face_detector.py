# face recognition code is adopted from: https://github.com/indently/webcam_face_recognition 


import face_recognition
import os, sys
import cv2
import numpy as np
import math
import time 
import threading 
import queue 


class FaceDetector:
    detected_face_locations = []
    deteced_face_encodings = []
    face_names = []
    known_face_encodings = []
    known_face_names = []
    recognition_thread = None 
    is_recognition_finished_flag = False  
    stop_recognition_thread = True
    video_source = 0
    
    # recognition_queue = None

    def __init__(self, video_source = 0):
        self.encode_faces()
        self.video_source = video_source

    def encode_faces(self):
        for image in os.listdir('faces'):

            face_image = face_recognition.load_image_file(f"faces/{image}")
            face_encoding = face_recognition.face_encodings(face_image)[0]

            self.known_face_encodings.append(face_encoding)
            self.known_face_names.append(image.split(".")[0])

        print(f'faces: {self.known_face_names}')

    # get percentage of how much a face is recogrnized
    def calculate_confidence(face_distance, face_match_threshold=0.6):
        range = (1.0 - face_match_threshold)
        linear_val = (1.0 - face_distance) / (range * 2.0)

        if face_distance > face_match_threshold:
            return str(round(linear_val * 100, 2)) + '%'
        else:
            value = (linear_val + ((1.0 - linear_val) * math.pow((linear_val - 0.5) * 2, 0.2))) * 100
            return str(round(value, 2)) + '%'


    def start_face_recognition(self):

        # make sure previous thread is stopped
        self.reset_face_recognition() 
        try:
            self.recognition_thread.stop()
        except:
            pass 

        # start a new recognition thread
        # recognition_thread = threading.Thread(target=self.__run_recognition, args=(self.recognition_queue,))
        self.stop_recognition_thread = False
        self.recognition_thread = threading.Thread(target=self.__run_recognition)
        self.recognition_thread.start()
    
    # this function will stop the recognition thread as well as clear the flags and names from previous recognition, use with care !!!
    def reset_face_recognition(self):
        self.stop_recognition_thread = True
        self.is_recognition_finished_flag = False #reset flag
        self.face_names = [] #reset list

    def get_detected_faces(self):
        return self.face_names 

    # returns whether the recognition algorithm is still running or not 
    def is_recognition_finished(self):
        return self.is_recognition_finished_flag 


    # returns whether the recognition algorithm succeeded 
    def is_recognition_successful(self):
        return len(self.face_names) != 0 

    def __run_recognition(self):
        # start video capture
        video_capture = cv2.VideoCapture(self.video_source)

        if not video_capture.isOpened():
            sys.exit('Video source not found, try another ? ls /dev/video* may help !')

        # init 
        self.is_recognition_finished_flag = False 
        self.face_names = []

        while (not self.is_recognition_finished_flag) and (not self.stop_recognition_thread):

            ret, frame = video_capture.read()

            # Resize frame of video to 1/4 size for faster face recognition processing
            small_frame = cv2.resize(frame, (0, 0), fx=0.25, fy=0.25)

            # Convert the image from BGR color (which OpenCV uses) 
            # to RGB color (which face_recognition uses)
            rgb_small_frame = small_frame[:, :, ::-1]

            # Find all the faces and face encodings in the current frame of video
            self.detected_face_locations = face_recognition.face_locations(rgb_small_frame)
            self.deteced_face_encodings = face_recognition.face_encodings(rgb_small_frame, self.detected_face_locations)

            # clear prev face names list
            self.face_names = []


            # check if some face(s) are detected, if not skip to next iteration
            if len(self.detected_face_locations) == 0 : 
                # print("no faces found !") 
                continue
             


            # go through all faces and get the best match
            familar_face_detected = False 
            for face_encoding in self.deteced_face_encodings:
                # See if the face is a match for the known face(s)
                matches = face_recognition.compare_faces(self.known_face_encodings, face_encoding)

                # Calculate the shortest distance to face
                face_distances = face_recognition.face_distance(self.known_face_encodings, face_encoding)

                best_match_index = np.argmin(face_distances)
                if matches[best_match_index]:
                    name = self.known_face_names[best_match_index]
                    # confidence = self.calculate_confidence(face_distances[best_match_index])
                    # self.face_names.append(f'{name} ({confidence})') 
                    self.face_names.append(f'{name}')
                    familar_face_detected = True
                else :
                    self.face_names.append("unknown person !")

            if familar_face_detected:
                print(f'familiar faces detected: {self.face_names}')
                self.is_recognition_finished_flag = True
                break


            # Display the results
            # TODO: uncomment this to show GUI

            # for (top, right, bottom, left), name in zip(self.detected_face_locations, self.face_names):
            #     # Scale back up face locations since the frame we detected in was scaled to 1/4 size
            #     top *= 4
            #     right *= 4
            #     bottom *= 4
            #     left *= 4

            #     # Create the frame with the name
            #     cv2.rectangle(frame, (left, top), (right, bottom), (0, 0, 255), 2)
            #     cv2.rectangle(frame, (left, bottom - 35), (right, bottom), (0, 0, 255), cv2.FILLED)
            #     cv2.putText(frame, name, (left + 6, bottom - 6), cv2.FONT_HERSHEY_DUPLEX, 0.8, (255, 255, 255), 1)

            # # Display the resulting image
            # cv2.imshow('Face Recognition', frame)

            # Hit 'q' on the keyboard to quit!
            # if cv2.waitKey(1) == ord('q'):
            #     break
            

        # Release handle to the webcam
        video_capture.release()
        cv2.destroyAllWindows()
