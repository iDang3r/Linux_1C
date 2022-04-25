#!/bin/bash

rmmod catalog_device
rmmod catalog_module
make
insmod catalog_module.ko
insmod catalog_device.ko
