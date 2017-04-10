#!/usr/bin/env python
# -*- coding: utf-8 -*-

from flask import Flask
import paho.mqtt.publish as mqtt_publish


app = Flask(__name__)

mqtt_server = 'cloudmqtt_broker_hostname'  # provided by cloudmqtt
mqtt_port = 12345  # provided by cloudmqtt
mqtt_user = 'myUser'  # as configured on cloudmqtt
mqtt_password = 'myPassword'  # as configured on cloudmqtt


def publish(topic, message):
    mqtt_publish.single(topic, message, hostname=mqtt_server, port=mqtt_port,
                        auth={'username': mqtt_user, 'password': mqtt_password})


@app.route('/')
def index():
    return 'This is the gateway'


# 'gate' is either 'entry' or 'exit'
# The 'who' fragment is a poor way to indicate triggered the gate. This will
# be removed in a future version that supports basic auth.
@app.route('/garage/<string:gate>/<string:who>')
def open_gate(gate, who):
    if gate not in ['entry', 'exit']:
        return 'Invalid gate', 400
    publish('garage/{}'.format(gate), who)
    return 'Opening {} gate'.format(gate)


if __name__ == '__main__':
    app.run()
