# garage-controller

Software and firmware for a esp8266 based garage controller

## Motivation and Purpose

Visitor parking is available in the car park of our residential complex.
To get in you need a remote control or a proximity card (which also gets
you to our floor).
To get out you need a remote (not a prox card), which means we need to
accompany any visitors to the car park and let them out or they have
to tailgate another car.

I discovered that we nearly have line of sight from a window in our
apartment to the garage and that activating the remote from there can
open the garage gates. Wouldn't it be nice if we could let trusted
visitors get out on their own?

The goal of this project is to be able to remotely activate the car park
entry and exit gates without requiring physical access to the remote control.

## Design

There were some constraints and requirements to the design.

* I have no access to or ability to change any of the existing hardware
relating to the gate operation. The only part of that system I can touch is
the remote control that I own.

* I wanted to be able to easily make changes to the solution if I'm unhappy
with it.

* The system should be operable with a smart phone.

There are hardware and software components to this solution.

## Hardware

The esp8266 is a wifi enabled controller that can easily be programmed
via the Arduino IDE. I picked up a NodeMCU board a while back and this
fit the bill as the means to interact with the actual garage remote
control.

I also had a couple of cheap relays in my parts box which would do for
simulating a press of the physical buttons on my existing garage remote.
With the relays connected to the button pads of the remote control,
activating a relay for a period of time is the same as pressing the button.

Having decided on an esp8266 and a couple of relays to form the garage
controller, I needed a way to trigger the system.

## Software

MQTT was an obvious mechanism to send messages from an external system
to the garage controller. CloudMQTT is a solution to this, offering an
internet-accessible MQTT broker.

As a surrogate remote control, a smart phone is ideal since nearly everyone
has one and there are numerous apps around that might be suitable without
even having to write any Android software. I needed a way to send a MQTT
message to the CloudMQTT broker from the smart phone. I didn't find any
cloud services offering a HTTP->MQTT gateway so I wrote a simple solution
with python + Flask and deployed it to AWS Lambda using Zappa. The last
piece is a mechanism to make a request to the HTTPS endpoint of the Lambda
function.

I looked at IFTTT's Do button at first as a means to trigger the system.
I found that it was taking at least 8 seconds for a relay to be activated,
which was much too slow for my liking. A quick search turned up an Android
app called HTTP Request Shortcuts. This resulted in very fast triggering
of the pipeline (< 1s from "button press" to relay activation).

The system flow is described in the diagram below.

## The code

There are 2 components: a python/Flask app running as an AWS Lambda
and the Arduino sketch running on the esp8266.

The python app is responsible for receiving a web request and publishing to
the correct topic on the MQTT broker. There's no authentication of requests
at the moment. I intend to add basic auth support, and either ship the
creds with the python code or persist them externally (perhaps DynamoDB).
Later I may add more features to make it easy to give visitors single day
access. This component is very simple to update to suit my needs.

The esp8266 code subscribes to garage/# on the broker and activates the entry
and exit gate relays depending on the received topic for each incoming message.
I based this off the MQTT examples included with PubSubClient. This library is
not included by default in the Arduino IDE. You can install it by going to
Sketch -> Include Library -> Manage Libraries and searching for PubSubClient.

## Diagram

Android phone
+--------------+
| Button press |     # A smart phone app is used to hit a web API endpoint.
+--------------+

  +
  | HTTPS request
  v

AWS APIGW / Lambda
+--------------+
| Web API      |     # The API is handled by an AWS Lambda function acting as a HTTP->MQTT proxy.
+--------------+     # It publishes to topic garage/entry or garage/exit as required.

  +
  | MQTT publish
  v

CloudMQTT
+--------------+
| MQTT Broker  |     # The broker publishes the received topic and payload to all subscribed clients.
+--------------+     # I used CloudMQTT as it is well known in the community and has a free plan.

  +
  | MQTT
  v

esp8266 (NodeMCU)
+--------------+
|  Controller  |     # When the controller receives a MQTT topic of garage/entry or garage/exit
+--------------+     # it activates the appropriate relay for 1 second.

  +
  | Relay
  v

Garage remote
+--------------+     # With the relay hooked up to the push button pads on the garage remote board,
| Remote press |     # a 1 second activation is the same as a 1 second button press.
+--------------+     # The chosen gate opens.
