#ifndef __config_h_
#define __config_h_

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     0 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32 0x7A

#define ENCODER_A 14 // Drehzahlencoder Pin A
#define ENCODER_B 27 // Drehzahlencoder Pin B
#define ENCODER_PULS_PER_REVOLUTION 600 // Drehzahlencoder Pin B

#define DIR_PIN 4
#define STEP_PIN 2
#define STEPPER_GEAR_RATIO 0.5f // example: 20 teath on stepper & 40 teeth on spindel is a 1/2 gear ratio
#define MICROSTEPS_PER_REVOLUTION 400
#define SPINDEL_THREAD_PITCH 1.5f // value in mm
#define MINIMUM_STEP_PULS_WIDTH 4 // in µs will result in speed -> less torque
#define THRESHOLD 0.001f
#define MAX_MOTOR_SPEED 1200 // Höchstgeschwindigkeit
#define SINGLE_STEP ((SPINDEL_THREAD_PITCH * STEPPER_GEAR_RATIO) / MICROSTEPS_PER_REVOLUTION)
#define MAX_SPEED_MULTIPLIER 100.0f
// Buttons
#define BUTTON_ADD_PIN 23
#define BUTTON_ADD_INDEX 0

#define BUTTON_REMOVE_PIN 18
#define BUTTON_REMOVE_INDEX 1

#define BUTTON_TARGET_PIN 13
#define BUTTON_TARGET_INDEX 2

#define BUTTON_POSITION_PIN 17
#define BUTTON_POSITION_INDEX 3

#define BUTTON_JOG_LEFT_PIN 33
#define BUTTON_JOG_RIGHT_PIN 35

#endif