# NeuroFlappy Cardputer-Adv

AI Flappy Bird for M5Stack Cardputer-Adv.

## Build automatically

Upload this folder to a GitHub repository.

Every push starts GitHub Actions and builds:

`neuroflappy_cardputer.bin`

The file is uploaded as the artifact:

`neuroflappy-cardputer-firmware`

## Controls on Cardputer

- `T` starts neuroevolution training
- `P` plays the saved best brain
- `Q` stops the current mode

The firmware stores the best brain in ESP32 Preferences so the trained network survives reboot.

## Hardware target

M5Stack Cardputer-Adv / ESP32-S3, 8 MB flash.
