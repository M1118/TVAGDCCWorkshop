#include <NmraDcc.h>

#include <SD.h>

#include <pcmConfig.h>
#include <pcmRF.h>
#include <TMRpcm.h>

#define SD_ChipSelectPin 4  //using digital pin 4 on arduino nano 328, can use other pins

#define DCCSOUND_VERSION_ID  2
/*
 * DCC Simple Sound Decoder
 *
 * Play sounds based on the function key pressed
 */
TMRpcm audio;
const int speakerPin = 9;
unsigned int percentage;

int curfile = -1;  // Current file number playing
char *filename[] = {
  "F0.wav", "F1.wav", "F2.wav", "F3.wav", "F4.wav",
  "F5.wav", "F6.wav", "F7.wav", "F8.wav", "F9.wav",
  "F10.wav", "F11.wav", "F12.wav"
};

unsigned int ChuffCount = 0;
char *ChuffFile = "SrtChuff.wav";
unsigned long maxchuffs = 250;
unsigned long minchuffs = 1000;
unsigned long chuffrange = minchuffs - maxchuffs;
unsigned long chuffgap = 0;
unsigned long nextchuff = 0;

NmraDcc  Dcc;
DCC_MSG  Packet;
const int DccAckPin = 3;
const int DccInPin = 2;

struct CVPair
{
  uint16_t  CV;
  uint8_t   Value;
};

#define  CV_VOLUME        10
#define  CV_MAXCHUFFS_LSB 11
#define  CV_MAXCHUFFS_MSB 12
#define  CV_MINCHUFFS_LSB 13
#define  CV_MINCHUFFS_MSB 14

CVPair FactoryDefaultCVs [] =
{
  {CV_ACCESSORY_DECODER_ADDRESS_LSB, 3},
  {CV_ACCESSORY_DECODER_ADDRESS_MSB, 0},
  {CV_MULTIFUNCTION_PRIMARY_ADDRESS, 3},
  {CV_VERSION_ID, DCCSOUND_VERSION_ID},
  {CV_MANUFACTURER_ID, MAN_ID_DIY},
  {CV_VOLUME, 5},
  {CV_MAXCHUFFS_LSB, maxchuffs & 0xff },
  {CV_MAXCHUFFS_MSB, maxchuffs >> 8},
  {CV_MINCHUFFS_LSB, minchuffs & 0xff },
  {CV_MINCHUFFS_MSB, minchuffs >> 8},
  {CV_29_CONFIG, 0},
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
  Dcc.setCV(CV, value);
  switch (CV)
  {
    case CV_VOLUME:
      audio.setVolume(value);
      break;
    case CV_MAXCHUFFS_LSB:
      maxchuffs = (maxchuffs & 0xff00) | value;
      chuffrange = minchuffs - maxchuffs;
      break;
    case CV_MAXCHUFFS_MSB:
      maxchuffs = (maxchuffs & 0xff) | (value << 8);
      chuffrange = minchuffs - maxchuffs;
      break;
    case CV_MINCHUFFS_LSB:
      minchuffs = (minchuffs & 0xff00) | value;
      chuffrange = minchuffs - maxchuffs;
      break;
    case CV_MINCHUFFS_MSB:
      minchuffs = (minchuffs & 0xff) | (value << 8);
      chuffrange = minchuffs - maxchuffs;
      break;
    case CV_MULTIFUNCTION_PRIMARY_ADDRESS:
      break;
  }
}


void notifyDccSpeed( uint16_t Addr, uint8_t Speed, uint8_t ForwardDir, uint8_t SpeedSteps )
{
  percentage = ((unsigned long)(Speed - 1) * 100) / SpeedSteps;
  if (percentage == 0)
    ChuffCount = 0;
  if (ChuffCount < 3)
     ChuffFile = "SrtChuff.wav";
  else if (percentage < 50)
    ChuffFile = "MidChuff.wav";
  else
    ChuffFile = "FstChuff.wav";
  if (percentage > 0)
  {
    chuffgap = maxchuffs + (((100 - percentage) * chuffrange) / 100);
    if (nextchuff < millis())
      nextchuff = millis() + chuffgap;
  }
}

void notifyDccFunc( uint16_t Addr, uint8_t FuncNum, uint8_t FuncState)
{
int newfile = -1;
static uint8_t  lastState = 0;
uint8_t diff;

  if (FuncNum == 1)
  {
    diff = FuncState & (~lastState);
    lastState = FuncState;
    if (diff & 0x10)
      newfile = 0;
    if (diff & 0x01)
      newfile = 1;
    if (diff & 0x2)
      newfile = 2;
    if (diff & 0x4)
      newfile = 3;
    if (diff & 0x8)
      newfile = 4;
  }
#if GRP2
  else if (FuncNum == 2)
  {
    if (FuncState & 0x11)
      newfile = 5;
    if (FuncState & 0x12)
      newfile = 6;
    if (FuncState & 0x14)
      newfile = 7;
    if (FuncState & 0x18)
      newfile = 8;
    if (FuncState & 0x01)
      newfile = 9;
    if (FuncState & 0x02)
      newfile = 10;
    if (FuncState & 0x04)
      newfile = 11;
    if (FuncState & 0x8)
      newfile = 12;
  }
#endif
  if (newfile != -1 && newfile != curfile && audio.isPlaying())
  {
    audio.stopPlayback();
    curfile = -1;
  }
  if ((newfile != curfile || audio.isPlaying() == false) && newfile != -1)
  {
    audio.play(filename[newfile]);
    curfile = newfile;
  }

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
  Dcc.init(MAN_ID_DIY, 10, FLAGS_MY_ADDRESS_ONLY|FLAGS_OUTPUT_ADDRESS_MODE, 0);
  
  // If the saved CV value for the decoder does not match this version of
  // the code force a factry reset of the CV values
  if (Dcc.getCV(CV_VERSION_ID) != DCCSOUND_VERSION_ID)
    FactoryDefaultCVIndex = sizeof(FactoryDefaultCVs)/sizeof(CVPair);
  
  if (!SD.begin(SD_ChipSelectPin)) {  // see if the card is present and can be initialized:
    Serial.println("SD fail");  
    return;   // don't do anything more if not
  }
  audio.speakerPin = speakerPin;
  audio.quality(1);
  audio.volume(1);
}

void loop()
{
  // You MUST call the NmraDcc.process() method frequently from the Arduino loop() function for correct library operation
  Dcc.process();
  audio.loop(0);
  if (percentage > 0 && millis() > nextchuff)
  {
    audio.play(ChuffFile);
    nextchuff = millis() + chuffgap;
    ChuffCount++;
  }
    
    
  /* Check to see if the default CV values are required */
  if( FactoryDefaultCVIndex && Dcc.isSetCVReady())
  {
    FactoryDefaultCVIndex--; // Decrement first as initially it is the size of the array 
    Dcc.setCV( FactoryDefaultCVs[FactoryDefaultCVIndex].CV,
          FactoryDefaultCVs[FactoryDefaultCVIndex].Value);
  }
}
