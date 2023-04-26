from face_detector import *
import paho.mqtt.client as mqtt
import RPi.GPIO as GPIO
import json
import time

TIMEOUT = 10  # seconds
LED_GREEN_BLINKING_TIME = 0.2
LED_TURN_ON_TIME = 5
SLEEP_TIME = 0.05

# RPI BOARD PINS
RED_LED_PIN = 7
GREEN_LED_PIN = 8
BUTTON_PIN = 10

# MQTT setup and configurations
_USER_NAME = ""
_PASSWORD = ""
_ACCESS_TOKEN = "y9922LZdtIOlit0BJuNo"
CLIENT_ID = "rpi-cam-01"
MQTT_TOPIC = "v1/devices/uplink"
MQTT_BROKER = "broker.mqttdashboard.com"
MQTT_PORT = 1883


# function returns epoch time in ms
def get_time():
    curr_time = round(time.time())
    return curr_time


# callback for mqtt connect() function call
def on_connect(_client, _userdata, _flags, rc):
    if rc == 0:
        print("Client connected to MQTT Server")
    else:
        print("Client cannot connect to MQTT Server : ", rc)


# callback for mqtt publish() function call
def on_publish(_client, _userdata, _result):
    print("Published the data successfully")


# callback for mqtt disconnect() function call
def on_disconnect(client, _userdata, _rc):
    print("Client disconnected from Server")


if __name__ == "__main__":
    # Ignore gpio warnings and use physical board pin numbering
    GPIO.setwarnings(False)
    GPIO.setmode(GPIO.BOARD)

    # set pin 10 to be an input pin and set initial value to be pulled low (off)
    GPIO.setup(BUTTON_PIN, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)

    # configure leds as output
    GPIO.setup(GREEN_LED_PIN, GPIO.OUT)
    GPIO.setup(RED_LED_PIN, GPIO.OUT)

    # turn off leds
    GPIO.output(RED_LED_PIN, GPIO.LOW)
    GPIO.output(GREEN_LED_PIN, GPIO.LOW)

    print("gpios initialization done")

    # create new client instance
    client = mqtt.Client(CLIENT_ID)

    # bind callbacks to mqtt client
    client.on_connect = on_connect
    client.on_publish = on_publish
    client.on_disconnect = on_disconnect

    # # set access token to connect to thingsboard platform
    # client.username_pw_set(ACCESS_TOKEN)

    # init face detector
    fd = FaceDetector(video_source=0)
    try:
        while True:
            # read the button pin
            button_state = GPIO.input(BUTTON_PIN)
            # check button state to high, if not sleep for 50ms
            if button_state == GPIO.HIGH:
                print("!!!Button was pressed!!!")

                '''Face Recognition'''
                # start face detection
                fd.start_face_recognition()

                # check if the face recognition is finished or the time is out !
                # if not true continue waiting (looping)
                prev_time = time.time()
                while (not fd.is_recognition_finished() and (time.time() - prev_time) < TIMEOUT):
                    if GPIO.input(GREEN_LED_PIN) == GPIO.HIGH:
                        GPIO.output(GREEN_LED_PIN, GPIO.LOW)
                    else:
                        GPIO.output(GREEN_LED_PIN, GPIO.HIGH)
                    time.sleep(LED_GREEN_BLINKING_TIME)

                # JSON data
                cam_data = {
                    "bn": "SmartDoorCam/",
                    "e": [
                        {
                            "n": "face_detect",
                            "vs": list(),
                            "t": 0
                        }
                    ]
                }

                # updates the camera data with the recognized faces if recognition is successful and sets LED pin to high (green)
                if fd.is_recognition_successful():
                    cam_data["e"][0]["t"]  = get_time()
                    cam_data["e"][0]["vs"] = ",".join(map(str, fd.get_detected_faces()))
                    GPIO.output(GREEN_LED_PIN, GPIO.HIGH)
                # if recognition is unsuccessful, sets the camera data to "Unrecognized person" and sets LED pin to high (red).
                else:
                    cam_data["e"][0]["t"]  = get_time()
                    cam_data["e"][0]["vs"] = "unrecognized"
                    GPIO.output(RED_LED_PIN, GPIO.HIGH)

                # stop face recognition and reset to default state
                fd.reset_face_recognition()

                '''MQTT'''
                # establish a connection to a MQTT broker.
                client.connect(MQTT_BROKER, MQTT_PORT)
                client.loop_start() # auto reconnects (runs in background)
                 
                # check if the client is connected to server or the time is out !
                # if not true continue waiting
                prev_time = time.time()
                while (not client.is_connected() and (time.time() - prev_time) < TIMEOUT):
                    pass

                # check the client connection to mqtt server.
                if client.is_connected():
                    client.publish(
                        MQTT_TOPIC, json.dumps(cam_data), 0
                    )  # publish the data to mqtt server
                    print(
                        "Published message " +
                        str(cam_data) + " to topic " + MQTT_TOPIC
                    )
                    client.loop_stop() # # stop client loop
                    client.disconnect()  # disconnect client from mqtt server
                    
                else:
                    # TODO : add functionality store and send the data whenever the client next connects to mqtt server
                    pass

                # sleep for 5s
                time.sleep(LED_TURN_ON_TIME)

                # turn off leds
                GPIO.output(GREEN_LED_PIN, GPIO.LOW)
                GPIO.output(RED_LED_PIN, GPIO.LOW)
            else:
                # sleep for a short period before reading the button again (reduces CPU usage)
                time.sleep(SLEEP_TIME)

    except KeyboardInterrupt:
        print("*program interrupted by user*")

    except:
        print("***unknown error, need to be handled by user.***")

    finally:
        GPIO.cleanup()
