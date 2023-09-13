# How does the freeRTOS mqtt stack should be integrated

## First step is to initialize the context (Certs stuff for SSL connection)

* Demo code uses the ESP Secure Certificate Manager.  The partition is encrypted using some
  special keys (not sure how it all works yet, TO BE INVESTIGATED).

* Should the certs be stored in the cert store using the esp32 cert api?
  - Maybe do that later once we have a better idea of the manufacturing process.

## Starting mqtt listener and agent tasks (from the mn8 app)

* MN8 App register MQTT connection callback to get connection status.

* Start the manager connection task.
  This task ensure the connection stays on.  It relyes on the state of the wifi connection.

* Start the listener task 
  Subscribe to the mqtt topics.
  Wait for incoming message
    * On message fire events to state machine

* 

##

We will have a networkagent task to monitor the connection and have the network logic
