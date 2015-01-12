#include <NmraDcc.h>
#include <Servo.h>
#include <DCCServo.h>

/*
 * DCC Example 3
 *
 * Control up to three servos with a combination
 * of the speed control knob, direction button and
 * fucntions F0 to F2.
 * Servo limits of travel and speed are fixed in the code
 * The decoder is hardwired to address 3
 */

DCCServo  *servo1, *servo2, *servo3;
const int servoPin = 9;

NmraDcc  Dcc;
DCC_MSG  Packet;
const int DccAckPin = 3;
const int DccInPin = 2;


/*
 * Periodically called with the speed, direction and number of speed steps
 *
 * Work out the current speed percentage and direction and update each of the
 * servos with this data
 */
void notifyDccSpeed( uint16_t Addr, uint8_t Speed, uint8_t ForwardDir, uint8_t SpeedSteps )
{
  /* Only respind to address 3 */
  if (Addr != 3)
    return;

  int percentage = ((Speed - 1) * 100) / SpeedSteps;
  servo1->setSpeed(percentage, ForwardDir != 0);
  servo2->setSpeed(percentage, ForwardDir != 0);
  servo3->setSpeed(percentage, ForwardDir != 0);
}

/*
 * Called regularly to report the state of the function keys
 * Update the active status of each of the servos based on the functions
 * that are enabled.
 */
void notifyDccFunc( uint16_t Addr, uint8_t FuncNum, uint8_t FuncState)
{
  /* Respond only to address 3 */
  if (Addr != 3)
    return;
    
  if (FuncNum != 1)
    return;
  if (FuncState & 0x10)
    servo1->setActive(true);
  else
    servo1->setActive(false);
  if (FuncState & 0x01)
    servo2->setActive(true);
  else
    servo2->setActive(false);
  if (FuncState & 0x02)
    servo3->setActive(true);
  else
    servo3->setActive(false);
}

void setup()
{
  Serial.begin(9600);
  
  // Configure the DCC CV Programing ACK pin for an output
  pinMode(DccAckPin, OUTPUT);
  digitalWrite(DccAckPin, LOW);
  
  // Setup which External Interrupt, the Pin it's associated with that we're using and enable the Pull-Up 
  Dcc.pin(0, DccInPin, 1);
  
  // Call the main DCC Init function to enable the DCC Receiver
  Dcc.init(MAN_ID_DIY, 10, FLAGS_OUTPUT_ADDRESS_MODE, 0);
  
  // Create the 3 instances of the DCCServo class that represents
  // each of the servos we control. The arguments are the pin number,
  // two limits of travel and the time in seconds to move between them
  // at 100% velocity
  servo1 = new DCCServo(servoPin, 10, 130, 30);
  servo2 = new DCCServo(servoPin + 1, 20, 80, 10);
  servo3 = new DCCServo(servoPin + 2, 0, 90, 60);
}

void loop()
{
  // You MUST call the NmraDcc.process() method frequently from the
  // Arduino loop() function for correct library operation
  Dcc.process();
  
  // Call the loop functions for each of the servos
  servo1->loop();
  servo2->loop();
  servo3->loop();
}
