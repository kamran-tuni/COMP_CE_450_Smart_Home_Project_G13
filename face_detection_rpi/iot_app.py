from face_detector import *

def detection_callback(names, timestamp):
    print(f'{names} : at {timestamp}')

if __name__ == '__main__':
    fr = FaceDetector(detection_callback)
    fr.run_recognition()

