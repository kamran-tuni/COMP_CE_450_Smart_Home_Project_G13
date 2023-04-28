package com.example.cognitosheildclient;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;
import org.eclipse.paho.android.service.MqttAndroidClient;
import org.eclipse.paho.client.mqttv3.IMqttActionListener;
import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.IMqttToken;
import org.eclipse.paho.client.mqttv3.MqttCallbackExtended;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.json.JSONException;
import org.json.JSONObject;


public class MQTTSubService extends Service {

    private static final String TAG = "MQTTSubService";

    private MqttAndroidClient mqttAndroidClient;
    final String serverUri = "tcp://broker.hivemq.com:1883";
    String clientId = "SmartDoorCamAndroidClient_BackgroundService";
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
    private final int ONGOING_NOTIFICATION_ID = 7;
    private PendingIntent doorcam_pintent;
    private PendingIntent service_notif_pintent;

    public MQTTSubService() {
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.d(TAG, "onStartCommand: again.. !");
        //initialize MQTT connection and register callbacks
        initMqttConnection();
        // try connecting to the MQTT server
        connectToMqttServer();

        // If the notification supports a direct reply action, use
        // PendingIntent.FLAG_MUTABLE instead.
        Intent notificationIntent = new Intent(this, MainActivity.class);
        service_notif_pintent =
                PendingIntent.getActivity(this, 77, notificationIntent,
                        PendingIntent.FLAG_IMMUTABLE);

        doorcam_pintent = PendingIntent.getActivity(this, 79, notificationIntent,
                        PendingIntent.FLAG_CANCEL_CURRENT | PendingIntent.FLAG_IMMUTABLE);

        Notification notification =
                new Notification.Builder(MQTTSubService.this, App.NOTIFICATION_CHANNEL_ID)
                        .setContentTitle("SmartDoorCAM client")
                        .setContentText("Connecting to server...")
                        .setContentIntent(service_notif_pintent)
                        .setStyle(new Notification.BigTextStyle())
                        .build();

        // Notification ID cannot be 0.
        startForeground(ONGOING_NOTIFICATION_ID, notification);

        // If we get killed, after returning from here, restart
        return START_NOT_STICKY;

    }

    @Override
    public void onDestroy() {
        super.onDestroy();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    void initMqttConnection(){
        mqttAndroidClient = new MqttAndroidClient(getApplicationContext(), serverUri, clientId);
        mqttAndroidClient.setCallback(new MqttCallbackExtended() {
            @Override
            public void connectComplete(boolean reconnect, String serverURI) {
                if (reconnect) subscribeToTopic();
                connectionSuccessful = true;
                updateServiceNotification("Successfully connected! waiting for requests...");
            }

            @Override
            public void connectionLost(Throwable cause) {
                updateServiceNotification("Connection lost ! retrying ...");
                connectionSuccessful = false;
            }

            @Override
            public void messageArrived(String topic, MqttMessage message)  {
                parseMessage(message.toString());
            }

            @Override
            public void deliveryComplete(IMqttDeliveryToken token) {
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
                    subscribeToTopic();
                    connectionSuccessful = true ;
                }

                @Override
                public void onFailure(IMqttToken asyncActionToken, Throwable exception) {
                    connectionSuccessful = false;
                    mqttAndroidClient.close();
                    stopSelf();

                }
            });
        } catch (MqttException ex){
            ex.printStackTrace();
        }
    }


    void parseMessage(String message) {
        try {
            JSONObject jsonObject = new JSONObject(message);
            String sender = jsonObject.getString("bn");
            if(sender.equals("ThingsBoardIoT/")) {
                JSONObject dataObject = jsonObject.getJSONArray("e").getJSONObject(0);
                String request = dataObject.getString("n");
                if(request.equals("access_request")){
                    pushNotification("Request to open the door from "+ dataObject.getString("vs"));
                }
            }
        }catch (JSONException e){
            e.printStackTrace();
        }

    }

    void subscribeToTopic(){
        try {
            mqttAndroidClient.subscribe(subscriptionTopic, 0, null, new IMqttActionListener() {
                @Override
                public void onSuccess(IMqttToken asyncActionToken) {
                    Log.d(TAG, "onSuccess: subscription complete");
                }

                @Override
                public void onFailure(IMqttToken asyncActionToken, Throwable exception) {
                    Log.d(TAG, "onFailure: subscription failed");
                }
            });

        } catch (MqttException ex){
            System.err.println("Exception whilst subscribing");
            ex.printStackTrace();
        }
    }

    void updateServiceNotification(String content){
        NotificationManager  manager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
        manager.notify(ONGOING_NOTIFICATION_ID,
                new Notification.Builder(this, App.NOTIFICATION_CHANNEL_ID)
                .setContentTitle("SmartDoorCAM client")
                .setContentText(content)
                .setSmallIcon(R.drawable.ic_outline_lock_open_24)
                .setContentIntent(service_notif_pintent)
                .build()
        );
    }

    void pushNotification(String content){ //TODO: flag cancel current
        NotificationManager  manager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
        manager.notify(ONGOING_NOTIFICATION_ID+1,
                new Notification.Builder(this, App.NOTIFICATION_CHANNEL_ID)
                        .setContentTitle("SmartDoorCAM client")
                        .setSmallIcon(R.drawable.ic_outline_lock_open_24)
                        .setContentText(content)
                        .setContentIntent(PendingIntent.getActivity(this, 777, new Intent(this, MainActivity.class),PendingIntent.FLAG_UPDATE_CURRENT| PendingIntent.FLAG_IMMUTABLE | PendingIntent.FLAG_NO_CREATE))
                        .setContentIntent(doorcam_pintent)
                        .build()
        );
    }
}