/*
 * MyStepper.h - Add the support of DFROBOT Stepper Driver Board
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

// ensure this library description is only included once
#ifndef MyStepper_h
#define MyStepper_h

// library interface description
class MyStepper {
  public:
    // constructors:
    MyStepper(int number_of_steps, int motor_pin, int direction_pin);

    // speed setter method:
    void setSpeed(long whatSpeed);

    // mover method:
    void step(int number_of_steps);
    void clockwiseRotate(int degree);
    void anticlockwiseRotate(int degree);

  private:
    void stepMotor(int this_step);

    int direction;            // Direction of rotation
    int speed;                // Speed in RPMs
    unsigned long step_delay; // delay between steps, in ms, based on speed
    int number_of_steps;      // total number of steps this motor can take
    int pin_count;            // how many pins are in use.
    int step_number;          // which step the motor is on

    // motor pin numbers:
    int motor_pin;
    int direction_pin;

    unsigned long last_step_time; // time stamp in us of when the last step was taken
};

#endif

