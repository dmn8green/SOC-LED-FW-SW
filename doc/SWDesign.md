| <span style="color: #A00;">THIS DOCUMENT IS THE CONFIDENTIAL PROPERTY OF MN8 Energy. NO RIGHTS ARE GRANTED TO USE THIS DOCUMENT FOR ANY PURPOSE OTHER THAN FURNISHING SERVICES AND SUPPLIES TO MN8 Energy.</span> |
| :----------------------------------------------------------: |

# Software Design Document: State of Charge LED Display System

This document describes and provides supporting details for the software design of the SOC (State of Charge) LED indicator system. 

## Scope

This document describes the solution for MN8 Energy's request for a State of Charge (SOC) LED indicator for an EV Charging Station.  

This document applies to the entire product. This includes an embedded controller as well as cloud-based software packages. 

A conceptual rendering of the LED Indicators at a charging station was provided by MN8 at project start. This photo is for reference only, and does not define any project requirements. For example, the smaller secondary indicator to the left of the larger LED strip is not part of this project.

![](./images/MN8 Station.png)

​					**Figure 1: Conceptual Rendering of LEDs at four Stations**



## Reference Documents

All documents below are available in [Basecamp](https://3.basecamp.com/3162450/projects/33639386): 

* *"ChargePoint Web Services API Programmer’s Guide, v5.1*", ChargePoint
* Espressif *"led_strip_example"* project delivered with ESP IDF v5.2, Espressif  
* *"WS2815 Intelligent control LED integrated light source"*, World Semi
* *"SOC LED Light Requirements and Standards"*, MN8 Energy

## Operating Environment

This section will describe the software running on each component of the system.

### Station Controller

The Station Controller is an on-site ESP32 micro controller delivered on an AppliedLogix designed PCB in a custom enclosure.  Each charging station will support one or two charging ports (typically to the left and right of a central pillar). 

The Station Controller is connected to the Internet via Ethernet and WiFi. The Station Controller is powered by PoE, and the WiFi connection is used as a backup for Ethernet. The Station Controller provides a method for registering the station with the AWS database. During normal operation, the Station Controller will  interpret MQTT messages from the Proxy Broker and conrol the LED status bar(s). 

### Proxy Broker

The initial design called for the Station Controller to query the ChargePoint API directly. Initial testing showed this would likely be inefficient and expensive. A new design included the Proxy Broker, which makes requests of the ChargePoint API and delivers status updates to Station Controllers via MQTT. 

#### AWS Lambda

AWS Lambda provides a server-less mechanism for executing code on demand. It is used in this design to handle registration requests from Station Controllers. 

#### AWS DynamoDB

Provides key-value storage for all known Station Controller IDs within AWS. Written by Lambda, read by Proxy Broker. 

## Structural Design

This section describes software components and interfaces, and the lineage of design decisions leading to the resulting structure.

![](./images/MN8 Architecture.png)

​							**Figure 2:  System Block Diagram**



### Station Controller Software

Initial prototype testing started from the ESP SDK "led_strip_example" project, and expanded to include: 

- External serial port (115200/N/8/1) providing Command Line Interface (CLI) for config/debug
- Ability to register Station with AWS via serial port CLI
- MQTT Client to interpret station state from Proxy Broker
- Changed LED driver to SPI-based interface to work around known ESP example bug 
- Day/Night sensor connected via GPIO to drive LED brightness settings
- Source code is https://gitlab.appliedlogix.com/mn8-energy/light-controller-firmware

### AWS Infrastructure

This section describes the components implemented in Amazon Web Services (AWS). Currently everything is running under an Applied Logix account (Acct ID: 704412387318). 

#### Proxy Broker

This is a t2.micro (low-cost, single CPU, 1GB memory) EC2 instance running the following: 

- Ubuntu 20.04
- "asdf" runtime version manager 
- "node" version 18.18.0 
- "npm" version 9.8.1
- SSH server, port 22 opened to specific IP ranges as needed (via Security/Inbound Rules)
- Source code from here: https://gitlab.appliedlogix.com/mn8-energy/chargepoint-poll-proxy
- File ".env" Specifying the following: 

```
	`AWS_ARN=arn:aws:iot:us-east-1:704412387318:thing/502B838D3A08` 
	`AWS_CLIENTID=al-site-proxy` 
	`AWS_HOST=a12nujv5ze1vit-ats.iot.us-east-1.amazonaws.com` 
	`AWS_PORT=8883` 
	`AWS_KEY=./.secrets/private.pem.key` 
	`AWS_CERT=./.secrets/certificate.pem.crt` 
	`AWS_CA=./.secrets/AmazonRootCA1.pem`
	`CP_ENDPOINT=https://webservices.chargepoint.com/webservices/chargepoint/service`
	`CP_USERNAME=<hidden>`
	`CP_PASSWORD=<hidden>`
```

* Directory ".secrets" with the following files allowing for access to AWS 

  ```
  `AmazonRootCA1.pem`  
  `certificate.pem.crt`  
  `private.pem.key`  
  `public.pem.key`
  ```

  

#### AWS Lambda 

AWS Lambda functions allow for single-shot event handling without need for a dedicated server. We use this for device registration. 

- Code is here: https://gitlab.appliedlogix.com/mn8-energy/cloud-lambdas

## Detailed Design

### Station Controller Software

* This is an embedded C application developed using the ESP32 IDF development kit. FreeRTOS is commonly incorporated into ESP32 designs, and is used here as a low level operating system. 

* FreeRTOS Threads 

  * LED Threads (2)
    * Thread affinity used to keep these two threads on a single controller
  * CLI Thread 
  * MQTT Monitor Thread
    * Listen for station state updates via MqTT 
  * Day/Night Sensor Monitor 
  * Network Status Monitor

* Networking

  The default operational mode will be using wired Ethernet. The WiFi interface is used as a backup in the event of connection or other failure detected on the Ethernet interface. 

* CLI 

​	A command-line interface is available via Serial port.  Debug messages will be logged to this port, and 	input from a user can be done as well.  

​	Input is done at this command prompt: 

```
		mn8-tools>
```

​	<u>The following commands allow technicians to configure the Station Controller:</u>

​		**provision_cp**  <group or site name> <station id>   

​			<group or site name>  Site name where this station is located   

​			<station id>  Charge point station id  

​		**provision_iot**  <url> <username> <password>   provision device as an IOT thing          

​			<url>  Url to provisioning server    

​			<username>  API username     

​			<password>  API password



​	<u>The following commands provide troubleshooting and debugging for the Station Controller:</u> 

​		**help**    Print the list of available commands  

​		**tasks**    Get information about running tasks/threads  

​		**wifi**  [--timeout=<t>] <join/leave> [<ssid>] [<pass>]     

​			--timeout=<t>  Connection timeout, ms   		<join/leave>  Command to execute         

​			<ssid>  SSID of AP         

​			<pass>  PSK of AP  

​		**ifconfig  **[<eth/wifi>] [<up/down/dhcp/manual/reset>] [<ip>] [<netmask>] [<gateway>] [<dns>]   		Operate on network interfaces (wifi/ethernet)     

​			<eth/wifi>  Which interface to configure   			

​			<up/down/dhcp/**manual/reset>  Command to execute           

​			<ip>  IP address

​			<netmask>  Netmask     

​			<gateway>  Gateway

​			<dns>  DNS  

​		**reboot**    Reboot board  

​		**ping**  [-W <t>] [-i <t>] [-s <n>] [-c <n>] [-Q <n>] <host>   send ICMP ECHO_REQUEST to network 			hosts   		

​			-W, --timeout=<t>  Time to wait for a response, in seconds   

​			-i, --interval=<t>  Wait interval seconds between sending each packet   

​			-s, --size=<n>  Specify the number of data bytes to be sent   

​			-c, --count=<n>  Stop after sending count packets   

​			-Q, --tos=<n>  Set Type of Service related bits in IP datagrams         

​			<host>  Host address  

​		**led**  <on/off/pattern> [-p <p>] [-s <1/2>]   Control LED strips   

​			<on/off/pattern>  Command to execute   

​			-p, --pattern=<p>  Index to the desired pattern, 0 by default   

​			-s, --strip=<1/2>  Which strip to control. If no value is given, both strips will be controlled  

* LED Driver 

The LED strips use the WS8215 protocol. Each LED strip has multiple controllers wired in series along the length of the strip. The LED controllers are not aware of their physical position in the strip (this allows for  easy cutting, resizing, and joining strips). 

Each LED controller has a DIN for input signaling and a DO for cascading the signal to the next downstream LED controller. Each LED controller will interpret the first 24 bits of data it sees at DIN as a command and set its color accordingly; all bits following the first 24 are then forwarded via DO the next LED's DIN, which will repeat the process. 

The reduced bit sequence will be propagated along the LED strip until signal is exhausted or no more LEDs are present. Each LED controller maintains current color and forwards signals until a RESET is seen. 

Our LED strip uses 33 controllers, each driving three LEDs.  Each 24-bit sequence is interpreted by an LED controller in GRB (Green, Red, Blue) as follows: 



| G7   | G6   | G5   | G4   | G3   | G2   | G1   | G0   | R7   | R6   | R5   | R4   | R3   | R2   | R1   | R0   | B7   | B6   | B5   | B4   | B3   | B2   | B1   | B0   |
| ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- |

​								**Figure 3:  GRB Bit Sequencing**



Because there is only a single data line to the LEDs (no clock signal), the line must be driven high/low for reach bit transmitted. The duration of the high signal during each cycle indicates whether the bit should be interpreted as 0 or 1. 

The LED controller expects the following duty cycles: 

|         | Code                        | Duration         |
| ------- | --------------------------- | ---------------- |
| T0H     | 0-code, High-level time     | 220ns-380ns      |
| **T1H** | **1-code, High-level time** | **580ns-1600ns** |
| T0L     | 0-code, Low-level time      | 580ns-1600ns     |
| **T1L** | **1-code, Low-level time**  | **220ns-420ns**  |
| *RESET* | *Drive line low*            | *> 280000ns*     |

​						**Figure 4:  LED Controller Timing Requirements** (WS8215)



Cycles must be between 900ns (220ns+580ns) and ~2000ns (380/420ns + 1600ns). We found example code that used a pulse width of 1200ns, which falls on the faster side of the range but comfortably above the minimum. 

Our design makes use of ESP SDK's SPI library to drive the LED signal line. SPI is usually used for peripheral communications; in normal use the SPI uses several signals including clock and data out (MOSI). For our LED implementation, we will use only the MOSI line. 

Typical SPI signaling for 8-bit sequence (10100101) looks like this (CLK and MOSI lines only):



![](./images/MN8-SPI-Timing.png)

​								**Figure 5:  SPI Timing (CLK/MOSI)**



With an SPI clock speed of 6.66MHz , each SPI CLK cycle is 150ns.  We can hold the MOSI line high or low for multiples of 150ns by "sending" one of two bit sequences to the SPI driver:

```
static const uint8_t mZero = 0b11000000; // 300:900 ns 
static const uint8_t mOne  = 0b11111100; // 900:300 ns 
```

Note that transmission of a single bit to the LED strip requires eight SPI CLK cycles. With the sequences above, the MOSI line will be high for 300ns or 900ns, then low for 900ns or 300ns. 

This provides the desired LED pulse width of 1200ns and high/low durations well within the specification. 

The time required to address all LEDs is minimal and not visible. With 33 LED controllers using 24 bits each, and 1.2us required for each LED bit, we can update an LED strip in less than 0.1 milliseconds. 

* Day/Night sensor 

The Day/Night sensor will be wired to a GPIO pin on the ESP32 board. The sensor will drive one of two LED thresholds for brightness across all supported patterns and animations. 

### Proxy Broker Software

The Proxy Broker acts as an intermediary between the ChargePoint API and the set of Station Controllers being used. This application is written in Javascript and run via "npm" on an AWS EC2 instance. 

#### Proxy Broker: Connecting to Instance

Connecting to this instance will require: 

* AppliedLogix Account ID and IAM user name
* SSH client application
* SSH private Key 
* If accessing outside of Winton Place, an inbound rule on AWS allowing your public IP to connect to port 22 of the Proxy Broker instance. 

#### Proxy Broker: Code Organization

The EC2 instance has a dedicated directory for the Proxy Broker: 

```
	/home/ubuntu/proxy-broker/chargepoint-poll-proxy
```

During development, this directory is pointed at the AppliedLogix gitlab Proxy Broker repo (see link earlier in this document). This allows developers to pull and deploy changes quickly. 

#### Proxy Broker: Run/Debug Modes

Running in "Normal Mode", the Proxy Broker will query DynamoDB for list of known stations, then enter a loop requesting status from ChargePoint and sending updates to Station Controllers via MQTT. 

```
		% npm start poll 
```

Running in "Debug Mode". In this mode, several debug commands may be used to emulate ChargePoint device changes. 

```
		% npm start cli 
```

In this mode, the Proxy Broker will not automatically poll ChargePoint API (unless commanded to). This mode allows developers to emulate various state changes as if they are coming from ChargePoint.  This mode shows a user prompt: 

```
	? prompt>
```

The following commands are supported: 

​	**poll**	 Make a single query to ChargePoint and update Station Controller(s)

​	**state** <LED ID> <state string> 

​			Sets the internal state of an LED ID to a state string. Valid states (and numeric values). LED 			bars are numbered 1 and 2 within the Proxy Broker.  See table below: 

​	**m** <LED ID> <Numeric State Value> 

​			Sets the internal state of an LED ID to a numeric value. Identical to **state** but easier to 			type.  See table below:

```
   STATE STRING             NUMERIC VALUE
  
   * available				(0)
   * waiting_for_power 		(1)
   * charging				(2)    // May supply charging value [0-100]
   * charging_complete		(3)
   * out_of_service			(4)
   * disable				(5)
   * booting_up				(6)
   * offline				(7)
   * reserved				(8)
   * test_charging			(9)
   * unknown					
```

Examples: 

```
? prompt> state 1 available        // Sets LED 1 to green
? prompt> m 2 2 50                 // Sets LED2 to "charging" with level of 50% 
```





## Glossary

​	

| Term   | Meaning                                                      |
| ------ | ------------------------------------------------------------ |
| API    | Application Programming Interface                            |
| AWS    | Amazon Web Services                                          |
| CLI    | Command Line Interface                                       |
| CP     | ChargePoint                                                  |
| DIN    | Data In (LED Controller)                                     |
| DO     | Data Out (LED Controller)                                    |
| DNS    | Domain Name Service. Resolves domain (e.g. "www.appliedlogix.com") to IP address. |
| EC/EC2 | Elastic Compute                                              |
| ESP    | Espressif Systems, a microcontroller manufacturer and SDK provider |
| EV     | Electric Vehicle                                             |
| GPIO   | General Purpose Input/Output                                 |
| LED    | Light Emitting Diode (Strip of LEDs in this case)            |
| MOSI   | SPI line driving output                                      |
| MQTT   | Lightweight publish/subscribe network protocol; used to manage IOT devices |
| PoE    | Power over Ethernet                                          |
| SOC    | State of Charge                                              |
| SPI    | Serial Peripheral Interface                                  |
| SSH    | Secure Shell                                                 |

