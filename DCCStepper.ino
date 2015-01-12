#include <NmraDcc.h>
#include <DCCStepper.h>

/*
 * DCC Stepper Motor Example
 *
 * Control stepper motor with a combination
 * of the speed control knob, direction button and
 * fucntions F0 to F2.
 * The number of steps, gear ratio and maximum required
 * RPM's are set via CV values. Also the decoder address
 * is set via CV's
 */

DCCStepper  *stepper;
const int stepperPin = 4;

NmraDcc  Dcc;
DCC_MSG  Packet;
const int DccAckPin = 3;
const int DccInPin = 2;

struct CVPair
{
  uint16_t  CV;
  uint8_t   Value;
};

#define VERSION_ID      3

#define CV_STEPS       10
#define CV_RATIO       11
#define CV_MAXRPM      12

CVPair FactoryDefaultCVs [] =
{
  {CV_MULTIFUNCTION_EXTENDED_ADDRESS_LSB, 3},
  {CV_MULTIFUNCTION_EXTENDED_ADDRESS_MSB, 0},
  {CV_MULTIFUNCTION_PRIMARY_ADDRESS, 3},
  {CV_VERSION_ID, VERSION_ID},
  {CV_MANUFACTURER_ID, MAN_ID_DIY},
  {CV_STEPS, 8},
  {CV_RATIO, 64},
  {CV_MAXRPM, 60},
  {CV_29_CONFIG, 32},
};

static uint8_t FactoryDefaultCVIndex = 0;
// Set the above to sizeof(FactoryDefaultCVs) / sizeof(CVPair);
// In order to initialise CV values at startup


/*
 * A factory reset is required. Called on first run if the NVRAM does not contain
 * valid CV values
 */ 
void notifyCVResetFactoryDefault()
{
  // Make FactoryDefaultCVIndex non-zero and equal to num CV's to be reset 
  // to flag to the loop() function that a reset to Factory Defaults needs to be done
  FactoryDefaultCVIndex = sizeof(FactoryDefaultCVs)/sizeof(CVPair);
};

// This function is called by the NmraDcc library when a DCC ACK needs to be sent
// Calling this function should cause an increased 60ma current drain on the power supply for 6ms to ACK a CV Read 
void notifyCVAck(void)
{
  digitalWrite( DccAckPin, HIGH );
  delay( 6 );  
  digitalWrite( DccAckPin, LOW );
}

/*
 * Called to notify a CV value has been changed
 */
void notifyCVChange( uint16_t CV, uint8_t value)
{
  Serial.print("CV ");
  Serial.print(CV);
  Serial.print(" = ");
  Serial.println(value);
  Dcc.setCV(CV, value);
  switch (CV)
  {
    case CV_STEPS:
      break;
    case CV_MAXRPM:
      stepper->setRPM(value);
      break;
    case CV_MULTIFUNCTION_PRIMARY_ADDRESS:
      break;
  }
}


void notifyDccSpeed( uint16_t Addr, uint8_t Speed, uint8_t ForwardDir, uint8_t SpeedSteps )
{
  
  int percentage = ((Speed - 1) * 100) / SpeedSteps;
  stepper->setSpeed(percentage, ForwardDir != 0);
}

void notifyDccFunc( uint16_t Addr, uint8_t FuncNum, uint8_t FuncState)
{
  if (FuncNum != 1)
    return;
  if (FuncState & 0x10)
    stepper->setActive(true);
  else
    stepper->setActive(false);
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
  Dcc.init(MAN_ID_DIY, VERSION_ID, FLAGS_MY_ADDRESS_ONLY|FLAGS_OUTPUT_ADDRESS_MODE, 0);
Serial.println(Dcc.getCV(CV_VERSION_ID));
  if (Dcc.getCV(CV_VERSION_ID) != VERSION_ID)
    notifyCVResetFactoryDefault();
  
  stepper = new DCCStepper(Dcc.getCV(CV_RATIO) * Dcc.getCV(CV_STEPS),
                Dcc.getCV(CV_MAXRPM), stepperPin, stepperPin + 1, stepperPin + 2, stepperPin + 3);
}

void loop()
{
  // You MUST call the NmraDcc.process() method frequently from the Arduino loop() function for correct library operation
  Dcc.process();
  stepper->loop();
    
  /* Check to see if the default CV values are required */
  if( FactoryDefaultCVIndex && Dcc.isSetCVReady())
  {
    FactoryDefaultCVIndex--; // Decrement first as initially it is the size of the array 
    Dcc.setCV( FactoryDefaultCVs[FactoryDefaultCVIndex].CV,
          FactoryDefaultCVs[FactoryDefaultCVIndex].Value);
  }
}
