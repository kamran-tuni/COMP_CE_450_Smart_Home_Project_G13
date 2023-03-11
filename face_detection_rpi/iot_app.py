from face_detector import *
import paho.mqtt.client as mqtt
import RPi.GPIO as GPIO
import json, time

TIMEOUT = 10  # seconds

# RPI BOARD PINS
RED_LED_PIN = 7
GREEN_LED_PIN = 8
BUTTON_PIN = 10

# MQTT setup and configurations
_USER_NAME = ""
_PASSWORD = ""
CLIENT_ID = "rpi-cam-01"
ACCESS_TOKEN = "y9922LZdtIOlit0BJuNo"
MQTT_TOPIC = "v1/devices/me/telemetry"
MQTT_BROKER = "thingsboard.cloud"
MQTT_PORT = 1883

# JSON data
CAM_DATA = {"ts": 0, "names": list()}


# function returns epoch time in ms
def get_time():
    curr_time = round(time.time() * 1000)
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
def on_disconnect(_client, _userdata, _rc):
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

    # set access token to connect to thingsboard platform
    client.username_pw_set(ACCESS_TOKEN)

    # init face detector
    fd = FaceDetector()
    try:
        while True:
            # read the button pin
            button_state = GPIO.input(BUTTON_PIN)

            # check button state to high, if not sleep for 50ms
            if button_state == GPIO.HIGH:
                print("!!!Button was pressed!!!")

                # counter/timer for face detection
                face_detect_timer = 0

                # start face detection
                fd.start_face_recognition()

                # check if face recognition is finished and times out after a set duration, while blinking a red LED.
                while (not fd.is_recognition_finished()) and (
                    face_detect_timer < TIMEOUT
                ):
                    face_detect_timer += 1
                    GPIO.output(RED_LED_PIN, GPIO.HIGH)
                    time.sleep(0.2)
                    GPIO.output(RED_LED_PIN, GPIO.LOW)
                    time.sleep(0.2)

                # stop face recognition and reset to default state
                fd.reset_face_recognition()

                # updates the camera data with the recognized faces if recognition is successful and sets LED pin to high (green)
                if fd.is_recognition_successful():
                    recognized_faces = fd.get_detected_faces()
                    CAM_DATA["ts"] = get_time()
                    CAM_DATA["names"] = recognized_faces
                    GPIO.output(GREEN_LED_PIN, GPIO.HIGH)
                # if recognition is unsuccessful, sets the camera data to "Unrecognized person" and sets LED pin to high (red).
                else:
                    CAM_DATA["ts"] = get_time()
                    CAM_DATA["names"] = "Unrecognized person"
                    GPIO.output(RED_LED_PIN, GPIO.HIGH)

                # establish a connection to a MQTT broker and sets up the client to listen to messages with a 0.1 second timeout.
                client.connect(MQTT_BROKER, MQTT_PORT)
                client.loop(0.1)

                # check the client connection to mqtt server.
                if client.is_connected():
                    client.publish(
                        MQTT_TOPIC, json.dumps(CAM_DATA), 0
                    )  # publish the data to mqtt server
                    print(
                        "Published message " + str(CAM_DATA) + " to topic " + MQTT_TOPIC
                    )
                    client.loop_stop()  # stop client from mqtt server
                    client.disconnect()  # disconnect client from mqtt server
                else:
                    # TODO : add functionality store and send the data whenever the client next connects to mqtt server
                    pass

                # sleep for 10s
                time.sleep(10)

                # turn off leds
                GPIO.output(GREEN_LED_PIN, GPIO.LOW)
                GPIO.output(RED_LED_PIN, GPIO.LOW)
            else:
                # sleep for 50s
                time.sleep(0.05)

    except KeyboardInterrupt:
        print("*program interrupted by user*")

    except:
        print("***unknown error, need to be handled by user.***")

    finally:
        GPIO.cleanup()