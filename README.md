# Epitech - M - IoT II (M-IOT-200) - Project

School term: 2017 - 2018

## Team
Benjamin Lépine

Marvin Mottet 

Tsy-Jon Lau

## Description
A card/badge control system using a RFID module, an Adafruit Huzzah! ESP8266 microcontroller and LCD display.

Using MQTT protocol (mosquitto) with SSL/TSL protocole with secure communication and Node RED.

## Flow
On the LCD screen is ask to use a RFID on the reader.
When ask by the LCD screen, the user put a RFID card on the reader module.
The microcontroller get the UID on the card and send it throw MQTT to the server.