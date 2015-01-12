#include <NmraDcc.h>

/*
 * DCC Example 2
 *
 * Print all speed control and function packets packets for DCC address 3
 */
NmraDcc  Dcc;
DCC_MSG  Packet;
const int DccAckPin = 3;
const int DccInPin = 2;

/*
 * Periodically called with the speed, direction and number of speed steps
 */
void notifyDccSpeed( uint16_t Addr, uint8_t Speed, uint8_t ForwardDir, uint8_t SpeedSteps )
{

}

/*
 * Called regularly to report the state of the function keys
 */
void notifyDccFunc( uint16_t Addr, uint8_t FuncNum, uint8_t FuncState)
{
  // If the address is not 3 then simply return
  // Comment this out to have all addresses printed
  if (Addr != 3)
    return;
  Serial.print(" DCC Func Addr ");
  Serial.print(Addr, DEC);
  Serial.print(" FuncNum ");
  Serial.print(FuncNum, HEX);
  Serial.print(" FuncState ");
  Serial.println(FuncState, HEX);
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
}

void loop()
{
  // You MUST call the NmraDcc.process() method frequently from the Arduino loop() function for correct library operation
  Dcc.process();

}
