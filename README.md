# Bluetooth LE scanner

This package provides a Bluetooth LE scanner.
BLE advertisements are published as ROS blescan/BleScan messages to the blescan topic.

The python blescan library includes a helper class to decode iBeacon advertisements.

To compile, you will need `libbluetooth-dev`:

    apt get install libbluetooth-dev

Note: after compiling, you will need to ensure scanner is setuid root. i.e., run the following commands as root:

    chown root scanner
    chmod u+s scanner

