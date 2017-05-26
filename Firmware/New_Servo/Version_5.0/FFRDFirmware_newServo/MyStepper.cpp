/*
 * MyStepper.cpp - Add the support of DFROBOT Stepper Driver Board
 *
 * The circuits can be found at
 *
 * http://www.dfrobot.com.cn/goods-833.html
 *
 * //Chong: for DFROBOT Stepper Driver Board
 * The sequence of controls signals for 1 control wires is as follows
 * (columns C0 from above):
 *
 * Step C0
 *    1  0
 *    2  1
 */

#include "Arduino.h"
#include "MyStepper.h"
#include <math.h>

/*
 * two-wire constructor.
 * Sets which wires should control the motor.
 */
MyStepper::MyStepper(int number_of_steps, int motor_pin, int direction_pin)
{
  this->step_number = 0;                    // which step the motor is on
  this->speed = 0;                          // the motor speed, in revolutions per minute
  this->direction = 0;                      // motor direction
  this->last_step_time = 0;                 // time stamp in us of the last step taken
  this->number_of_steps = number_of_steps;  // total number of steps for this motor

  // Arduino pins for the motor control connection:
  this->motor_pin = motor_pin;
  this->direction_pin = direction_pin;

  // setup the pins on the microcontroller:
  pinMode(this->motor_pin, OUTPUT);
  pinMode(this->direction_pin, OUTPUT);

  // pin_count is used by the stepMotor() method:
  this->pin_count = 1;
}

/*
 * Sets the speed in revs per minute
 */
void MyStepper::setSpeed(long whatSpeed)
{
  this->step_delay = 60L * 1000L * 1000L / this->number_of_steps / whatSpeed;
}

/*
 * Moves the motor steps_to_move steps.  If the number is negative,
 * the motor moves in the reverse direction.
 */
void MyStepper::step(int steps_to_move)
{
  int steps_left = abs(steps_to_move);  // how many steps to take

  // determine direction based on whether steps_to_mode is + or -:
  if (steps_to_move > 0)
  {
    this->direction = 1;
    digitalWrite(this->direction_pin, LOW);
  }
  if (steps_to_move < 0) 
  {
    this->direction = 0;
    digitalWrite(this->direction_pin, HIGH);
  }


  // decrement the number of steps, moving one step each time:
  while (steps_left > 0)
  {
    unsigned long now = micros();
    // move only if the appropriate delay has passed:
    if (now - this->last_step_time >= this->step_delay)
    {
      // get the timeStamp of when you stepped:
      this->last_step_time = now;
      // increment or decrement the step number,
      // depending on direction:
      if (this->direction == 1)
      {
        this->step_number++;
        if (this->step_number == this->number_of_steps) {
          this->step_number = 0;
        }
      }
      else
      {
        if (this->step_number == 0) {
          this->step_number = this->number_of_steps;
        }
        this->step_number--;
      }
      // decrement the steps left:
      steps_left--;
      // step the motor to step number 0, 1, ..., {3 or 10}
      if (this->pin_count == 1)
        stepMotor(this->step_number % 2);
      else
        stepMotor(this->step_number % 2);   // Chong: not used
    }
  }
}

/*
 * Moves the motor forward or backwards.
 */
void MyStepper::stepMotor(int thisStep)
{
  if (this->pin_count == 1) {
    switch (thisStep) {
      case 0:  // 0
        digitalWrite(motor_pin, LOW);
        break;
      case 1:  // 1
        digitalWrite(motor_pin, HIGH);
        break;
    }
  }
}

void MyStepper::clockwiseRotate(int degree)
{
    int calc_steps = lround(degree * double(this->number_of_steps) / double(360));
    step(calc_steps);
}

void MyStepper::anticlockwiseRotate(int degree)
{
    int calc_steps = lround(degree * double(this->number_of_steps) / double(360));
    step(-calc_steps);
}
