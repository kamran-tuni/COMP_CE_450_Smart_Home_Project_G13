package com.example.cognitosheildclient;

import androidx.appcompat.app.AppCompatActivity;

import android.app.ActivityManager;
import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.os.Build;
import android.os.Bundle;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.util.Log;
import android.widget.TextView;
import android.widget.Toast;

import com.google.android.material.snackbar.Snackbar;

import org.eclipse.paho.android.service.MqttAndroidClient;
import org.eclipse.paho.client.mqttv3.IMqttActionListener;
import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.IMqttToken;
import org.eclipse.paho.client.mqttv3.MqttCallbackExtended;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;


// Code adapted from: https://github.com/eclipse/paho.mqtt.android/blob/master/paho.mqtt.android.example/src/main/java/paho/mqtt/java/example/PahoExampleActivity.java

public class MainActivity extends AppCompatActivity {
    private static final String TAG = "MainActivity";
    private MqttAndroidClient mqttAndroidClient;
    final String serverUri = "tcp://broker.hivemq.com:1883";
    String clientId = "SmartDoorCamAndroidClient";
    final String subscriptionTopic = "v1/devices/downlink";
    final String publishTopic = "v1/devices/uplink";
    final String publishMessage =
            "{ \n" +
            "    \"bn\": \"DoorCamApp/\",\n" +
            "    \"e\": [\n" +
            "        {\n" +
            "            \"n\": \"unlock_request\"\n" +
            "        }\n" +
            "    ]\n" +
            "}";
    private boolean connectionSuccessful = false;
    private MqttConnectOptions mqttConnectOptions;
    private TextView status_view;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        status_view = findViewById(R.id.status_txt);

        //initialize MQTT connection and register callbacks
        initMqttConnection();
        // try connecting to the MQTT server
        connectToMqttServer();
        // start the background service
        if (!isMyServiceRunning(MQTTSubService.class)){
            Context applicationContext= getApplicationContext();
            Intent service_intent = new Intent(this, MQTTSubService.class);
            applicationContext.startForegroundService(service_intent);
        }

        // if button is clicked publish an unlock message
        findViewById(R.id.unlock_fab).setOnClickListener(view -> {
            vibrate();
            if (connectionSuccessful) publishUnlockMessage();
            else Toast.makeText(this, "Please make sure you're connected to the internet!", Toast.LENGTH_SHORT).show();
        });

    }

    void initMqttConnection(){
        mqttAndroidClient = new MqttAndroidClient(getApplicationContext(), serverUri, clientId);
        mqttAndroidClient.setCallback(new MqttCallbackExtended() {
            @Override
            public void connectComplete(boolean reconnect, String serverURI) {

                Log.d(TAG, "connectComplete: " + "Connected to: " + serverURI);
                show_status(ConnectionStatus.CONNECTED);
                connectionSuccessful = true;
            }

            @Override
            public void connectionLost(Throwable cause) {
                Log.d(TAG, "connectionLost: Connection Lost !");
                show_status(ConnectionStatus.LOST);
                connectionSuccessful = false;
            }

            @Override
            public void messageArrived(String topic, MqttMessage message) throws Exception {
                Log.d(TAG, "messageArrived: " + message.toString());
            }

            @Override
            public void deliveryComplete(IMqttDeliveryToken token) {
                Snackbar.make(findViewById(R.id.imageView),"Unlock request successfully delivered !",Snackbar.LENGTH_SHORT).show();
                Log.d(TAG, "deliveryComplete: Unlock message delivered with success !");
            }


        });

        mqttConnectOptions = new MqttConnectOptions();
        mqttConnectOptions.setAutomaticReconnect(true);
        mqttConnectOptions.setCleanSession(true);
    }


    void connectToMqttServer(){
        try {
            mqttAndroidClient.connect(mqttConnectOptions, null, new IMqttActionListener() {

                @Override
                public void onSuccess(IMqttToken asyncActionToken) {
                    Log.d(TAG, "onSuccess: Connection established !");
//                    show_status(ConnectionStatus.CONNECTED);
                    connectionSuccessful = true ;
                }

                @Override
                public void onFailure(IMqttToken asyncActionToken, Throwable exception) {
                    Log.d(TAG, "onFailure: Connection failed !");
                    connectionSuccessful = false;
                    show_status(ConnectionStatus.UNCONNECTED);
                    mqttAndroidClient.close();
                }
            });
        } catch (MqttException ex){
            ex.printStackTrace();
        }
    }
    void publishUnlockMessage(){
        try {
            MqttMessage message = new MqttMessage();
            message.setPayload(publishMessage.getBytes());
            mqttAndroidClient.publish(publishTopic, message);
            Log.d(TAG, "publishUnlockMessage: Message published");
            Snackbar.make(findViewById(R.id.imageView),"Sending unlock request...",Snackbar.LENGTH_SHORT).show();
        } catch (MqttException e) {
            System.err.println("Error Publishing: " + e.getMessage());
            e.printStackTrace();
        }
    }

    private boolean isMyServiceRunning(Class<?> serviceClass) {
        ActivityManager manager = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
        for (ActivityManager.RunningServiceInfo service : manager.getRunningServices(Integer.MAX_VALUE)) {
            if (serviceClass.getName().equals(service.service.getClassName())) {
                return true;
            }
        }
        return false;
    }

    private void vibrate(){
        Vibrator v = (Vibrator) getSystemService(Context.VIBRATOR_SERVICE);
        v.vibrate(VibrationEffect.createOneShot(250, VibrationEffect.DEFAULT_AMPLITUDE));
    }

    enum ConnectionStatus {CONNECTED, LOST, UNCONNECTED}
    void show_status(ConnectionStatus status){
        switch (status){
            case CONNECTED:
                status_view.setText("Connected !");
                status_view.setTextColor(Color.rgb(38,166,154));
                break;
            case LOST:
                status_view.setText("Connection Lost ! retrying...");
                status_view.setTextColor(Color.rgb(255,112,67));
                break;
            case UNCONNECTED:
                status_view.setText("Failed to connect :(\nVerify your internet and restart the app!");
                status_view.setTextColor(Color.rgb(239,83,80));
                break;
        }
    }

    @Override
    public void onBackPressed() {
//        super.onBackPressed();
        this.finish();
        System.exit(0);
    }
}