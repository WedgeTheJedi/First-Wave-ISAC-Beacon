/*********************SUMMARY AND ACKNOWLEDGEMENTS*********************
   The following code is designed for use with the Arduino
   setup of the First Wave ISAC Beacon by Cameron Goodwin.
   This code uses example code from the following sources
   and I wish to acknowledge and thank those authors for
   their examples and hard work.

   Guido666 - Initial Neopixel code from their version of
              the beacon and parts of their 3D models that
               I modified for my version.
              https://www.thingiverse.com/guido666/about
              https://www.thingiverse.com/thing:1411233

   MZD471 -   3D models and housing that I used for the
              beacon itself making a few slight modifications
              to the antenna assembly and adding holes for
              buttons on the side of the beacon.
              https://www.thingiverse.com/MZD471/about
              https://www.thingiverse.com/thing:1327053

   Angelo
   Qiao -     The example code for the DFMini Player
              library.  Legal and troubleshooting
              notices from their sample are below.
              Angelo.qiao@dfrobot.com

              /*********************************************************
                DFPlayer - A Mini MP3 Player For Arduino
                <https://www.dfrobot.com/index.php?route=product/product&product_id=1121>

               ***************************************************
                This example shows the basic function of library for DFPlayer.

                Created 2016-12-07
                By [Angelo qiao](Angelo.qiao@dfrobot.com)

                GNU Lesser General Public License.
                See <http://www.gnu.org/licenses/> for details.
                All above must be included in any redistribution
               ****************************************************

               ***********Notice and Trouble shooting***************
                1.Connection and Diagram can be found here
                <https://www.dfrobot.com/wiki/index.php/DFPlayer_Mini_SKU:DFR0299#Connection_Diagram>
                2.This code is tested on Arduino Uno, Leonardo, Mega boards.
               ****************************************************/
/*******************END SUMMARY AND ACKNOWLEDGEMENTS*******************/

///////////////////////////////////////
//  List of Improvements / Things to do
///////////////////////////////////////

/*  
    Although I put a lot of effort and work into creating a great software package to go with
    the First Wave ISAC Beacon, I reached a point where I needed to stop and make an initial
    release.  This meant that some other cool effects or ideas I had needed to be put off
    until another time.  I welcome any feedback and conributions to the codebase through
    the Github project.  Here is a list of features / improvements that I currently have:

    -   Make the flare fire off with a specific button press like 3 taps rather than just be part
        of the random chatter.
    -   Add in an effect mode where the Halo does some things so it isn't just a constant color
        for if you were walking around a convention or something so it is more 'flashy' and eye
        catching.  Something similar to the throbbing or effects in Guido666's code possibly.

    Update July 16, 2023
    I was building a few more beacon circuts and after downloading the latest Arduino IDE, found
    that improvements to the compiler and libraries appear to have changed some of the timing when
    run on the circuit.  That resulted in some bugs, especially where entering / exiting Rogue
    Mode would make the circuit stop responding.  I believe I have now fixed this and also fixed
    up the button input detection so that it is now cleaner.
*/

#include "Arduino.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"
#include <Adafruit_NeoPixel.h>
#include <math.h>

/////////////////////////////////////////////////////////
//  DFPlayer Mini Configuration
//      Used to play all audio and sound clips
/////////////////////////////////////////////////////////
DFRobotDFPlayerMini _DFPlayer;

//  This initializes the ~3 and ~5 digital pins for a serial connection
//  port we use to control the DFPlayer Mini
SoftwareSerial _DFPlayerSerial(DD3, DD5);  // RX, TX

//  Global variables and constants used for audio control
#define VOLUME_MAX       30   // DFPlayer Max volume is 30
#define VOLUME_STEP      2    // Value change of volume each step
#define VOLUME_STEP_WAIT 200  // Milliseconds between steps

uint8_t _AudioVolume      = 20;    // Start volume at ~70% of 30
uint32_t LastVolumeChange = 0;


//  Provided from the DFPlayer Mini example code, is useful for
//  debugging
void DFDebugPrintDetail(uint8_t type, int value);

//  The way the DFPlayer Mini works is that the global # of a track is
//  determined by listing the files as they were phsyically coppied onto
//  the SD card.  For example, re-copying the first file to the card will
//  change its order in global #'s and it will now be last
//  This enum should correspond to each sfx's global ID in order as they
//  should be copied to the SD in the setup instructions.
enum SoundEffects {
  // Startup Sequence
  Div2Startup = 1,
  Div2LoadingTics,
  BootUp,
  ISACReactivated,
  ISACReactivatedAgentAuthenticated,

  // Skills
  DZScanAudio,
  PulseAudio,
  BansheeAudio,
  JammerAudio,

  // DarkZone
  EnteringDarkZone,
  ExitingDarkZone,
  RogueProtocol,
  RogueProtocol_SFX,
  RogueStatusRemoved,
  RogueStatusRemoved_SFX,
  HeloGalExtractionCalled,
  HeloGalAlmostAtExtractionZone,
  HeloGalArrived,
  HeloGal30seconds,
  HeloGal15Seconds,
  HeloGalReturningToBase,

  // World Chatter
  VitalSignsCritical,
  AgentVitalSignsZero,
  FriendlyControlPointNearby,
  AgentRequestingBackup,
  CiviliansInDanger,
  DetectedNearbyLocationGuardedByHostiles,
  DrinkingWaterNearby,
  HostileControlPointIdentified,
  HostileRadioIntercepted,
  IncomingHostilesDetected,
  ObjectOfInterestDetected,
  RationsNearby,
  SuppliesDetected,

  // Dark Zone Chatter
  AgentOutOfAction,
  ImmediateMedicalAssistanceNeeded,
  DZLandmarkEngaged,
  DZContaminatedGearDetected,
  DZDropIncoming,
  DZDropStolen,
  NearbyAgentDissavowed,
  RogueAgentNearby,
  RogueAgentEliminated,

  //  Mode and other util sounds
  BackupNotificationBeeps,
  SkillCancel,
  Chirp,
  Div2SingleTic,

  // End enum - this can be used as a variable to know how many total clips we have
  ISACAudioCount
};

//  These will be used to randomly fill an array with the random
//  clips for world and DZ chatter.  This way each clip gets played
//  before one of them repeats, but it will still be in a random order
#define DZ_CLIPS_START  AgentOutOfAction
#define DZ_CLIPS_END    RogueAgentEliminated
#define DZ_CLIPS_COUNT  DZ_CLIPS_END - DZ_CLIPS_START + 2  // +2 because we need one more spot for the Extraction effect
                                                           // in the array so it can be fired off with the other effects
uint8_t DZClipOrderArray[DZ_CLIPS_COUNT] = {};
uint8_t CurrentDZClip = 0;

#define CHATTER_CLIPS_START VitalSignsCritical
#define CHATTER_CLIPS_END SuppliesDetected
#define CHATTER_CLIPS_COUNT CHATTER_CLIPS_END - CHATTER_CLIPS_START + 1

uint8_t ChatterClipOrderArray[CHATTER_CLIPS_COUNT] = {};
uint8_t CurrentChatterClip = 0;

//////////////////////////////////////////////////////
//  Side button (+) and (-) config
//////////////////////////////////////////////////////

//  The Pins that our + and - external buttons on the beacon are hooked up to
//  We are using analog pins here, but could also use digital pins.
#define MINUS_BUTTON_PIN A1
#define PLUS_BUTTON_PIN  A0

uint8_t MinusButtonLastState = HIGH;
uint8_t PlusButtonLastState = HIGH;

//  These are used to help keep rack of the button press
//  functionality where we are watching for both sequential
//  tapping and holding of each button or both together.
//  As mentioned in the things to imrpove, this could be
//  simplified more than likely.
bool MinusButtonActionCleared     = true;   // Helps prevent double hits on buttons
bool PlusButtonActionCleared      = true;
bool MinusButtonIgnoreNextRelease = false;  //  Used when we are holding so we
bool PlusButtonIgnoreNextRelease  = false;  //  don't fire off an event on release
bool MinusButtonReleaseHandled    = true;   // Helps ensure we handle button releases properly
bool PlusButtonReleaseHandled     = true;   // and don't double hit our logic code
bool ComboButtonActionCleared     = true;
bool ComboButtonReleaseHandled    = true;
bool IgnoreSingleButtonHold       = false;
uint8_t MinusButtonTimesTapped    = 0;
uint8_t PlusButtonTimesTapped     = 0;
uint32_t LastInputCycle           = 0;
uint32_t MinusButtonHeldSince     = 0;      // when the button was pressed but not yet
uint32_t PlusButtonHeldSince      = 0;      // released
uint32_t MinusButtonReleased      = 0;      // These help to watch for sequential taps and time out
uint32_t PlusButtonReleased       = 0;      // when to fire off an effect after tapping has finished

/////////////////////////////////////////////////////////
//  NeoPixel 16 config (aka the beacon's halo)
/////////////////////////////////////////////////////////
#define LED_COUNT     16    // Number of LEDs on the NeoPixel
#define NEOPIXEL_PIN  6     // Data pin for neopixel
#define CHATTER_PIN   A4    // Analog pin used to detect talking volume for
                            // the chatter blinking effect

Adafruit_NeoPixel Pixels = Adafruit_NeoPixel(LED_COUNT, NEOPIXEL_PIN);

#define DEFAULT_BRIGHTNESS 85
#define CHATTER_BRIGHT_MAX 150
#define CHATTER_BRIGHTNESS 45  // Brightness to go to as the base
                               // when Isac is talking

uint16_t HighestChatterLevel = 0;

//  Helper functions with setting the pixel's colors or resetting them
void ResetPixels();
void ResetPixels(uint32_t Color);
void ResetPixels(uint32_t Color, uint8_t Brightness);
void SparklePixels(uint32_t EffectLength, uint32_t SparkleColor, uint8_t Brightness);
void SparklePixels(uint32_t EffectLength, uint32_t SparkleColor, uint8_t Brightness, uint32_t BaseColor);
void SparklePixels(uint32_t EffectLength, uint32_t SparkleColor, uint8_t Brightness, uint32_t BaseColor, uint8_t Delay);
void UpdatePixelsFromArray(uint32_t HaloArray[], size_t ArraySize);
void UpdatePixelsFromArray(uint32_t HaloArray[], size_t ArraySize, uint8_t Brightness);

////////////////////////////////////////////////////////////
//  Pre-defined Colors used for effects and such
////////////////////////////////////////////////////////////
#define COLOR_RED    0xFF0000
#define COLOR_GREEN  0x00FF00
#define COLOR_BLUE   0x0000FF

//  True SHD 'Division' Orange is (255, 109, 16), but on the
//  Neopixel 16 this looks too yellow and not orange ehough
#define COLOR_SHD    0xFF6D10

//  These are custom color shades that look better on the
//  Neopixel in terms of SHD orange when being displayed
#define COLOR_SHD_CUSTOM        0xFF4C00       //  This one is still more 'yellow' but is good for some effects
#define COLOR_SHD_CUSTOM_DARK   0XFF2A00       //  This seems to be the best represntation of orange on the Neopixel
                                               //  and is a darker version of SHDCustom, since this looks best this
                                               //  is the default halo color

//  Other colors used by skills
#define COLOR_DZ_SCAN           0xe500e5
#define COLOR_BANSHEE_WAVE      0x69aaff
#define COLOR_JAMMER_CHARGE     0x3030ff
#define COLOR_JAMMER_DISCHARGE  0x5055ff

//  These are used for convenience during debugging to change default color
//  scheme with changing one variable rather than all over the code base.
#define COLOR_DEFAULT      COLOR_SHD_CUSTOM_DARK
#define COLOR_DEFAULT_DARK COLOR_SHD_CUSTOM_DARK

/////////////////////////////////////////////////////////////////////
//  Global state variables that are used across loop function cycles
/////////////////////////////////////////////////////////////////////
uint32_t _CurrentPixelColor = COLOR_DEFAULT;  // Current color... start orange

bool _IsRogue            = false;
bool _IsInDarkZone       = false;
bool _IsExtractionCalled = false;
uint8_t _ExtractionPhase;
uint32_t _ExtractionLastEvent = 0;
uint32_t _ChatterThreshold;  // Value that if the analog pin is higher, does the
                             // the chatter effect halo blinking

//  This enum is used to set the order of the skills and
//  how the skill wheel will cycle between them in order
enum Skills {
  Pulse,    // Red
  Jammer,   // Blue
  Banshee,  // Orange
  DZScan,   // Purple
  SkillCount
};

uint8_t _CurrentSkill = Pulse;       // used to cycle the skill powers

enum EffectMode {
  Manual = 0,
  Auto,
  EffectModeCount  // used as variable to know when we have cycled all the
                   // modes and need to loop back to the first
};

#define AUTO_EFFECT_DELAY 60000  // 1 minutes between auto audio effects
uint8_t _CurrentEffectMode = Manual;  // used to cycle between functionality modes
uint32_t _LastEffectTime = 0;

///////////////////////////////////////////////////////////////////
//  Initial Start-up and continuous looping functions
///////////////////////////////////////////////////////////////////
void setup() 
{
  //  Serial port for debug output (you will need an FTDI cable if
  //  running on the Trinket Pro 5V)
  Serial.begin(115200);

  //  Initialize the Neopixel / halo
  Pixels.begin();
  Pixels.setBrightness(DEFAULT_BRIGHTNESS);
  _LastEffectTime = millis();

  // Initialize the button and chatter pins as input.  Since we are 
  // not using a drawresister these will be INPUT_PULLUP
  pinMode(MINUS_BUTTON_PIN, INPUT_PULLUP);
  pinMode(PLUS_BUTTON_PIN, INPUT_PULLUP);
  pinMode(CHATTER_PIN, INPUT_PULLUP);

  Serial.println();
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));

  //  Serial port to control the DFMini Player (must be 9600)
  _DFPlayerSerial.begin(9600);

  if (!_DFPlayer.begin(_DFPlayerSerial)) 
  {  
    //Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
    while (!_DFPlayer.begin(_DFPlayerSerial))
    {
      delay(250);
    };
  }
  Serial.println(F("DFPlayer Mini online."));

  //  Seed our random number generator
  int seed = millis();
  srand(seed);

  //  Initialize the world and DZ chatter arrays
  for (uint8_t i = 0; i < CHATTER_CLIPS_COUNT; i++) 
  {
    ChatterClipOrderArray[i] = CHATTER_CLIPS_START + i;
  }

  ShuffleArray(ChatterClipOrderArray, CHATTER_CLIPS_COUNT);

  //  this sets the Extraction event into the DZ chatter
  //  effects to be fired
  DZClipOrderArray[0] = HeloGalExtractionCalled;

  //  Fill the other spaces in the array with the other DZ Chatter
  //  effects.
  for (uint8_t i = 1; i < DZ_CLIPS_COUNT; i++) 
  {
    DZClipOrderArray[i] = DZ_CLIPS_START + i - 1;
  }

  ShuffleArray(DZClipOrderArray, DZ_CLIPS_COUNT);

  _DFPlayer.volume(_AudioVolume);  //Set volume value. From 0 to 30

  //  Initialize Boot-up effect
  BootUpEffect();
}

//  Continuous loop / main thread
void loop() 
{
  //  Check the analog value on the chatter pin, if it is above
  //  the threshold, then change the brightness acording to the
  //  value for the blinking chatter effect
  int chatterPinValue = analogRead(CHATTER_PIN);
  int chatterLevel = chatterPinValue - _ChatterThreshold;
  if (chatterLevel > 0) 
  {
    if (chatterLevel > HighestChatterLevel) 
    {
      HighestChatterLevel = chatterLevel;
    }

    //  Get the % brightness of the max brightness based on the chatter level vs
    //  the highest catter level we've seen
    int chatterBrightness = (double)CHATTER_BRIGHT_MAX * ((double)chatterLevel / (double)HighestChatterLevel);

    Pixels.setBrightness(chatterBrightness);
    for (uint8_t i = 0; i < LED_COUNT; i++) 
    {
      Pixels.setPixelColor(i, _CurrentPixelColor);
    }

    Pixels.show();
    delay(70);
  }
  else if (Pixels.getBrightness() != DEFAULT_BRIGHTNESS) 
  {
    //  If the chatter isn't happening, then reset to the standard brightness
    Pixels.setBrightness(DEFAULT_BRIGHTNESS);
    for (uint8_t i = 0; i < LED_COUNT; i++) {
      Pixels.setPixelColor(i, _CurrentPixelColor);
    }
    Pixels.show();
  }

  //  With recent performance updates to the compiler we need
  //  to wait for 50ms before reading input again or else we
  //  possibly get multiple button reads on a single push.
  if((millis() - LastInputCycle) > 50)
  {
    LastInputCycle = millis();

    // read the state of the pushbutton values for this cycle
    uint8_t MinusButtonState = digitalRead(MINUS_BUTTON_PIN);
    uint8_t PlusButtonState = digitalRead(PLUS_BUTTON_PIN);
    
    if(MinusButtonState != MinusButtonLastState)
    {
      MinusButtonLastState = MinusButtonState;
      
      if(MinusButtonState == LOW)
      {
        Serial.println(F("(-) button pressed)"));
        MinusButtonHeldSince = millis();
        MinusButtonReleaseHandled = false;
        MinusButtonActionCleared = false;
      }
      else
      {
        //Serial.println(F("(-) button released)"));
      }      
    }

    if (MinusButtonState == LOW &&      // Button is still pressed
        !MinusButtonReleaseHandled &&   // Button release has not been handled
         !IgnoreSingleButtonHold)       // Skip if this is a double press state
    {
      //  Button is being held, check how long and if
      //  more than 2 seconds, go into volume control
      if (millis() - MinusButtonHeldSince >= 2000) {
        Serial.println(F("(-) button held for 2 seconds"));
        Serial.println(F("Stepping volume down"));

        // Don't take action on the next button release
        // because the volume up or down is the action.
        MinusButtonIgnoreNextRelease = true;

        //  Go into a loop to change the volume of the device
        //  re-check to see if the button was let go &
        //  update the halo LEDs to indicate current volume
        do 
        {
          //  Step down the volume
          if (millis() - LastVolumeChange >= VOLUME_STEP_WAIT) 
          {
            LastVolumeChange = millis();
            if (_AudioVolume != 0) 
            {
              VolumeDecrease();
            }
            SetHaloToVolumeValue();
          }

          delay(50);
          MinusButtonState = digitalRead(MINUS_BUTTON_PIN);
        } while (MinusButtonState == LOW);

        //  Leave the volume array up for a moment
        //  for visual conformation after release
        delay(500);
        ResetPixels(_CurrentPixelColor);
      }
    } 
    else if (MinusButtonState == HIGH &&  // Button is not pressed
              !MinusButtonActionCleared)  // We have not taken action on the button release
    {
      if (MinusButtonIgnoreNextRelease)
      {
            MinusButtonIgnoreNextRelease = false; // we have now skipped this release
            MinusButtonActionCleared = true;
            MinusButtonReleaseHandled = true;
            MinusButtonReleased = 0;
            MinusButtonTimesTapped = 0;
      }
      else if (!MinusButtonReleaseHandled) 
      {
        //  This is a new button release, it could be the first
        //  or Nth in a sequence depending on the value of when it
        //  was last released
        if (MinusButtonReleased == 0) 
        {
          //  This is the first button release record the time
          //  and let our auto timer handle the result
          MinusButtonReleased = millis();
          MinusButtonTimesTapped = 1;
        }
        //  If the last tap was within 700 ms, this is a sequential tap (double)
        else if (millis() - MinusButtonReleased <= 700) 
        {
          MinusButtonTimesTapped++;
          Serial.print("(-) button tapped ");
          Serial.print(MinusButtonTimesTapped);
          Serial.println(" times");
          MinusButtonReleased = millis();  //  start the release watch timer again
        } 
        else 
        {
          //  ignore double read of release which happened in earlier versions of the code
        }

        MinusButtonReleaseHandled = true;  //  Mark that we handled this release
      } 
      else 
      {
        //  State clearing actions for single presses or end of a sequence of taps
        if (millis() - MinusButtonReleased > 700) 
        {
          Serial.println("(-) button action timeout");
          switch (MinusButtonTimesTapped) 
          {
            case 2:
              Serial.println("  Cycle chatter mode");
              //  Double Tap, cycle on/off auto chatter
              CycleAudioMode();
              break;

            default:
              Serial.println("  Play random chatter");
              if (_IsInDarkZone) 
              {
                DarkZoneEffects();
              } 
              else 
              {
                RandomChatter();
              }
              break;
          }

          //  We have now cleared the action from the input
          MinusButtonActionCleared = true;
          MinusButtonReleased = 0;  //  reset to watch for next tap sequence
          MinusButtonTimesTapped = 0;
          _LastEffectTime = millis();
        }
      }
    }

    if(PlusButtonState != PlusButtonLastState)
    {
      PlusButtonLastState = PlusButtonState;
      
      if(PlusButtonState == LOW)
      {
        Serial.println(F("(+) button pressed)"));
        PlusButtonHeldSince = millis();
        PlusButtonReleaseHandled = false;
        PlusButtonActionCleared = false;
      }
      else
      {
        //Serial.println(F("(+) button released)"));
      }      
    }

    if (PlusButtonState == LOW &&                                // Button is still pressed
          !PlusButtonReleaseHandled && !IgnoreSingleButtonHold)  //skip this if this is a double press state
    {
      //  Button is being held, check how long and if
      //  more than 2 seconds, go into volume control
      if (millis() - PlusButtonHeldSince >= 2000) 
      {
        Serial.println("(+) plus button held for 2 seconds");
        Serial.println("  Stepping volume up");
        PlusButtonIgnoreNextRelease = true;

        //  Go into a loop to change the volume of the device
        //  re-check to see if the button was let go to
        //  update the halo LEDs to indicate current volume
        do 
        {
          if (millis() - LastVolumeChange >= VOLUME_STEP_WAIT) 
          {
            LastVolumeChange = millis();
            if (_AudioVolume < VOLUME_MAX) 
            {
              VolumeIncrease();
            }
            SetHaloToVolumeValue();
          }

          delay(50);
          PlusButtonState = digitalRead(PLUS_BUTTON_PIN);
        } while (PlusButtonState == LOW);

        delay(500);
        ResetPixels(_CurrentPixelColor);
      }
    } 
    else if (PlusButtonState == HIGH &&
             !PlusButtonActionCleared) 
    {
        if (PlusButtonIgnoreNextRelease)
        {
            PlusButtonIgnoreNextRelease = false; // we have now skipped this release
            PlusButtonActionCleared = true;
            PlusButtonReleaseHandled = true;
            PlusButtonReleased = 0;
        }
        else if (!PlusButtonReleaseHandled) 
        {
          //Serial.println(F("(+) button released"));
          
          //  This is a new button release, it could be the first
          //  or nth in a sequence depending on the value of when it
          //  was last released
          if (PlusButtonReleased == 0) 
          {
            //  This is the first button release record the time
            //  and let our auto timer handle the result
            PlusButtonReleased = millis();
            PlusButtonTimesTapped = 1;
          }
          else if (millis() - PlusButtonReleased <= 700) 
          {
            //  If the last tap was within 250 ms, this is a sequential tap (double)
            PlusButtonTimesTapped++;
            Serial.print("(+) button tapped ");
            Serial.print(PlusButtonTimesTapped);
            Serial.println(" times");
            PlusButtonReleased = millis();  //  start the release watch timer again
            CycleSkills();
          } 
          else 
          {
            //  ignore double read of release which happened in earlier versions of the code
          }

          PlusButtonReleaseHandled = true;  //  Mark that we handled this release
      } 
      else
      {
        //  State clearing actions for single presses or end of a sequence of taps
        if (millis() - PlusButtonReleased > 700) 
        {
          Serial.println("(+) Button action timeout");
          ActivateSkill(_CurrentSkill);
          //  We have now cleared the action from the imput
          PlusButtonActionCleared = true;
          PlusButtonReleased = 0;  //  reset to watch for next tap sequence
        }
        else 
        {
          //  Show the Skill UI wheel
          ShowSkillsHalo();
        }
      }
    }

    //  Handle the case where both buttons are being held or tapped
    if (PlusButtonState == LOW && MinusButtonState == LOW) 
    {
      if (ComboButtonActionCleared && ComboButtonReleaseHandled) 
      {
        Serial.println("Button combo detected, ignoring individual button releases");
        ComboButtonActionCleared = false;
        IgnoreSingleButtonHold = true;
        ComboButtonReleaseHandled = false;
        PlusButtonIgnoreNextRelease = true;
        MinusButtonIgnoreNextRelease = true;
        PlusButtonHeldSince = millis();
        MinusButtonHeldSince = PlusButtonHeldSince;
      } 
      else if (!ComboButtonActionCleared) 
      {
        //  See if we have been held for 1.9 seconds (just before the volume controls would kick in
        if (millis() - PlusButtonHeldSince > 1900) 
        {
          Serial.println("Toggling Dark Zone");
          //Cycle Dark Zone
          ToggleDarkZone();
          ComboButtonActionCleared = true;
          _LastEffectTime = millis();
        }
      }
    } 
    else if (PlusButtonState == HIGH && MinusButtonState == HIGH
               && !ComboButtonReleaseHandled) 
    {
      //  This was a plus and minus button combo tap
      //  Only change rogue status if in Dark Zone
      if (!ComboButtonActionCleared) 
      {
        if (_IsInDarkZone) 
        {
          if (_IsRogue) 
          {
            ExitRogueEffect();
          } 
          else 
          {
            GoRogueEffect();
          }
        } 
        else 
        {
          //  Do nothing, we don't have functionality for a 
          //  2 button tap outside of the dark zone right now.
        }
        ComboButtonActionCleared = true;
      }

      ComboButtonReleaseHandled = true;
      IgnoreSingleButtonHold = false;
      _LastEffectTime = millis();
    }
  }

  //  Since the Extraction Effect self cycles a new clip every 15 seconds we
  //  call it every loop and let it decide if it needs to play the next clip
  ExtractionEffect(false);

  if (_CurrentEffectMode == Auto) 
  {
    //  Play an effect after waiting the defined time
    if ((millis() - _LastEffectTime) >= AUTO_EFFECT_DELAY)
    {
      _LastEffectTime = millis();
      Serial.println(F("Firing auto chatter effect"));
      if (_IsInDarkZone) 
      {
        DarkZoneEffects();
      } 
      else 
      {
        RandomChatter();
      }
    }
  }

  //  This is for debugging to help with issues related to the audo player
  if (_DFPlayer.available()) 
  {
    DFDebugPrintDetail(_DFPlayer.readType(), _DFPlayer.read());  //Print the detail message from DFPlayer to handle different errors and states.
  }
}

void DFDebugPrintDetail(uint8_t type, int value) 
{
  switch (type) 
  {
    case TimeOut:
      Serial.println(F("DFMini: Time Out!"));
      break;

    case WrongStack:
      Serial.println(F("DFMini: Stack Wrong!"));
      break;

    case DFPlayerCardInserted:
      Serial.println(F("DFMini: Card Inserted!"));
      break;

    case DFPlayerCardRemoved:
      Serial.println(F("DFMini: Card Removed!"));
      break;

    case DFPlayerCardOnline:
      Serial.println(F("DFMini: Card Online!"));
      break;

    case DFPlayerPlayFinished:
      Serial.print(F("DFMini: "));
      Serial.print(value);
      Serial.println(F(" finished"));
      _ChatterThreshold = CalculateChatThreshold(CHATTER_PIN);
      break;

    case DFPlayerError:
      Serial.print(F("DFMini ERROR: "));
      switch (value) 
      {
        case Busy:
          Serial.println(F("Card not found"));
          break;

        case Sleeping:
          Serial.println(F("Sleeping"));
          break;

        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;

        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;

        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;

        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;

        case Advertise:
          Serial.println(F("In Advertise"));
          break;

        default:
          Serial.println(F("Unknown"));
          break;
      }
      break;

    default:
      break;
  }
}

/********************* ISAC Beacon Effects ***********************
 *  These are the functions that will fire off and
 *  do the various audio and visual effects
 ****************************************************************/

/////////////////////////
//  Boot-Up Effect
/////////////////////////
void BootUpEffect() 
{
  Serial.println("ISAC boot effect started...");
  //  For the initial audio boot up sound effects we want them to be max volume
  _DFPlayer.volume(VOLUME_MAX - 2);

  PlayAudioClip(Div2Startup);

  //Random Sparks then bright halo
  SparklePixels(1900, COLOR_DEFAULT_DARK, CHATTER_BRIGHTNESS, 0x000000, 20);

  ResetPixels(COLOR_DEFAULT);
  delay(1500);

  //  Reset halo ring to blank
  ResetPixels(0x00, DEFAULT_BRIGHTNESS);

  ////////////////////////////
  //  Circular Loading effect
  _DFPlayer.volume(VOLUME_MAX - 4);
  PlayAudioClip(Div2LoadingTics);

  //  Need to set the first pixel for how I placed the neoPixel
  Pixels.setPixelColor(0, COLOR_SHD_CUSTOM);
  Pixels.show();

  //  Now load the rest of the pixels
  for (int i = LED_COUNT; i > 0; i--) 
  {
    delay(200);
    Pixels.setPixelColor(i, COLOR_SHD_CUSTOM);
    Pixels.show();
  }

  PlayAudioClip(BootUp);

  SparklePixels(4200, 0xFFFFFF, DEFAULT_BRIGHTNESS, COLOR_SHD_CUSTOM, 20);

  //  This is a special sparkle where instead of dark background we have
  //  the colored ring, so we won't use the SparklePixel() function
  //    uint32_t effectLength = 4200;
  //    uint32_t start = millis();
  //    Pixels.setBrightness(DEFAULT_BRIGHTNESS); // Set to standard brightness
  //    while ((millis() - start) <= effectLength)
  //    {
  //        uint8_t i = random(16);
  //        Pixels.setPixelColor(i, 0xFFFFFF);
  //        Pixels.show();
  //        delay(20);
  //        Pixels.setPixelColor(i, COLOR_SHD_CUSTOM);
  //        Pixels.show();
  //    }
  delay(1200);

  TransitionFade(_CurrentPixelColor, 2);
  //ResetPixels(COLOR_DEFAULT);
  delay(1000);

  //  Before ISAC starts talking, reset the volume to the normal
  //  default level
  _DFPlayer.volume(_AudioVolume);
  PlayAudioClip(ISACReactivatedAgentAuthenticated);
  Serial.println("finished");
}

/////////////////////////
//  Go Rogue Effect
/////////////////////////
void GoRogueEffect() 
{
  uint32_t currentColor = 0;

  Serial.println("Going Rogue...");
  _IsRogue = true;
  PlayAudioClip(RogueProtocol_SFX);

  //  Sparkle to clear ring then glow to Rogue
  SparklePixels(700, COLOR_RED, DEFAULT_BRIGHTNESS);

  uint8_t cycleDelay = 15;
  for (currentColor = 0x000000; currentColor < COLOR_RED; currentColor += 0x010000) 
  {
    ResetPixels(currentColor);
    delay(cycleDelay);
  }

  _CurrentPixelColor = currentColor;
  delay(800);
  PlayAudioClip(RogueProtocol);
  Serial.println("Gone Rogue");
}

void ExitRogueEffect() 
{
  //  Todo: Add audio clips
  uint8_t currentBrightness;
  uint32_t currentColor = 0;

  Serial.println("Exiting Rogue...");
  PlayAudioClip(RogueStatusRemoved_SFX);

  //  Sparkle to clear ring
  SparklePixels(700, COLOR_DEFAULT_DARK, DEFAULT_BRIGHTNESS);

  uint8_t sectionCount = 5;
  uint16_t pixelsPerSection = LED_COUNT / sectionCount;
  uint8_t pixelsLeftOver = LED_COUNT % sectionCount;
  for (uint8_t section = 0; section < sectionCount; section++) 
  {
    uint8_t startPixel = section * pixelsPerSection;
    uint8_t endPixel = startPixel + pixelsPerSection;
    for (uint8_t i = startPixel; i < endPixel; i++) 
    {
      Pixels.setPixelColor(i, COLOR_DEFAULT_DARK);
    }
    Pixels.show();
    delay(200);
  }

  //  Set all pixels to the brighter color
  ResetPixels(COLOR_DEFAULT);
  _CurrentPixelColor = COLOR_DEFAULT;

  delay(500);
  _IsRogue = false;
  
  PlayAudioClip(RogueStatusRemoved);
  Serial.println(F("Rogue status removed"));
}

/////////////////////////
//  Scan Effect
/////////////////////////
void DZScanEffect() 
{
  Serial.println("DZ Scan");
  PlayAudioClip(DZScanAudio);

  delay(1000);

  ResetPixels(COLOR_DZ_SCAN, 20);

  delay(600);
  int effectLength = 1200;
  double effectBrightness = 100.0;
  uint32_t effectStart = millis();
  double brightnessPerCycle = 0.2;

  while ((millis() - effectStart) <= effectLength) 
  {
    Pixels.setBrightness(effectBrightness);
    for (uint8_t i = 0; i < LED_COUNT; i++) 
    {
      Pixels.setPixelColor(i, COLOR_DZ_SCAN);
    }
    Pixels.show();
    delay(2);

    effectBrightness = effectBrightness - brightnessPerCycle;
    if (effectBrightness < 0) {
      effectBrightness = 0;
    }
  }

  ResetPixels();
  SparklePixels(2000, COLOR_DZ_SCAN, DEFAULT_BRIGHTNESS, 0x000000, 35);
  delay(1600);

  ResetPixels(COLOR_DZ_SCAN);
  delay(700);

  TransitionFade(_CurrentPixelColor, 5);
}

/////////////////////////
//  Pulse Effect
/////////////////////////
void PulseEffect() 
{
  Serial.println("Pulse");
  PlayAudioClip(PulseAudio);

  delay(639);
  int effectLength = 1460;
  double effectBrightness = 255.0;
  uint32_t effectStart = millis();
  double brightnessPerCycle = 0.5;

  while ((millis() - effectStart) <= effectLength) 
  {
    Pixels.setBrightness(effectBrightness);

    for (uint8_t i = 0; i < LED_COUNT; i++) 
    {
      Pixels.setPixelColor(i, COLOR_SHD_CUSTOM);
    }

    Pixels.show();
    delay(2);

    effectBrightness = effectBrightness - brightnessPerCycle;
    if (effectBrightness < 0) 
    {
      effectBrightness = 0;
    }
  }

  //  Now randomly select 2-4 enemies to highlight on the 'radar'
  uint8_t enemies[4] = { 255, 255, 255, 255 };
  uint8_t numberOfEnemies = rand() % 4;

  //  Make sure that there is at least one
  numberOfEnemies++;
  for (uint8_t i = 0; i < numberOfEnemies; i++) 
  {
    enemies[i] = rand() % 15;  // select a pixel to put this enemy on
                               // leave pixel [15] open so that we can use
                               // it for the 2nd pixel if [14] is selected
  }

  Serial.print(numberOfEnemies);
  Serial.print(" enemies will be at the following pixels: ");
  DebugPrintArray(enemies, numberOfEnemies);

  //  Clear out pixels and re-set brightness
  ResetPixels();

  //  now do 2 sensory sweeps grabbing 2 enemies each time
  for (uint8_t enemiesPulsed = 0; enemiesPulsed < 4; enemiesPulsed++) 
  {
    int enemyPos1 = enemies[enemiesPulsed];
    enemiesPulsed++;
    int enemyPos2 = enemies[enemiesPulsed];

    for (uint8_t i = 0; i < LED_COUNT; i++) 
    {
      uint32_t color = Pixels.getPixelColor(i);
      Pixels.setPixelColor(i, COLOR_SHD_CUSTOM);
      Pixels.show();
      delay(20);

      //  If this is an enemy position mark it red
      //  otherwise just turn it off
      if (i == enemyPos1 || i == (enemyPos1 - 1) || i == enemyPos2 || i == (enemyPos2 - 1)) 
      {
        Pixels.setPixelColor(i, COLOR_RED);
      } 
      else 
      {
        Pixels.setPixelColor(i, color);
      }
      Pixels.show();
    }
  }

  //  now restore all the other pixels
  for (uint8_t i = 0; i < LED_COUNT; i++) 
  {
    uint32_t color = COLOR_SHD_CUSTOM;
    for (uint8_t enemy = 0; enemy < numberOfEnemies; enemy++) {
      if (i == enemies[enemy] || i == (enemies[enemy] - 1)) {
        color = COLOR_RED;
      }
    }

    Pixels.setPixelColor(i, color);
    Pixels.show();
  }
  delay(3000);

  TransitionFade(_CurrentPixelColor, 5);
}

/////////////////////////
//  Banshee Effect
/////////////////////////
void BansheeEffect() {
  Serial.println("Banshee");
  PlayAudioClip(BansheeAudio);

  delay(400);
  int effectLength = 4800;
  double effectBrightnessMax = 150;
  double effectBrightness = 0;
  uint32_t effectStart = millis();
  double brightnessPerCycle = 0.25;

  uint32_t currentRunTime;
  double chargeEffectPercent = 100.0;

  do 
  {
    //  Calculate how many pixels we need to light based on the effect time % done
    currentRunTime = (millis() - effectStart);
    chargeEffectPercent = (double)currentRunTime / (double)effectLength;
    effectBrightness = (effectBrightnessMax * chargeEffectPercent) + 50.0;
    int cycleBrightness = random(effectBrightness - 10, effectBrightness + 10);

    //  depending on how long the effect charge has been going
    //  we need to not light pixels from the lower end and circle up
    //  to make the arc effect of the Banshee.  But we need to
    //  make sure the 4 pixels at the top stay always lit.
    //  Because the top of the ring is between LED 0 and 15 we need
    //  to think of the % in terms of 6 pixels and then not light
    //  that number from each side, one going down from pixel 7
    //  and the other going up from pixel 8
    //  But the effect doesn't start in a 360 circle in game so
    //  we start with 1/3 of the pixels already not lit (about 3/4 on each side)
    //  and then go from there to make the effect look better.
    double pixelsToHide = ((((double)LED_COUNT - 4.0) / 1.333) * chargeEffectPercent) + (LED_COUNT / 3);
    Pixels.setBrightness(cycleBrightness);
    for (uint8_t i = 0; i < LED_COUNT; i++) 
    {

      uint32_t color = COLOR_DEFAULT;
      //  Check the right side pixels to light / hide
      if ((i <= LED_COUNT / 2) && (i > (LED_COUNT / 2) - (pixelsToHide / 2))) 
      {
        color = 0x000000;
      }
      //  Check the left side pixels to light / hide
      //if ((i < LED_COUNT - 2) && (i > ((LED_COUNT / 2) + (pixelsToHide / 2))))
      else if ((i >= LED_COUNT / 2) && (i < (LED_COUNT / 2) + (pixelsToHide / 2))) 
      {
        color = 0x000000;
      }

      Pixels.setPixelColor(i, color);
    }
    Pixels.show();
    delay(25);
  } while ((millis() - effectStart) < effectLength);

  ResetPixels();

  //  Sparks effect after discharge
  effectLength = 1500;
  effectStart = millis();
  Pixels.setBrightness(DEFAULT_BRIGHTNESS);  // Set to standard brightness

  while ((millis() - effectStart) <= effectLength) 
  {
    for (uint8_t i = 0; i < LED_COUNT; i++) 
    {
      if ((i > LED_COUNT - 2) || (i < 2)) 
      {
        // 50 / 50 chance this pixel gets lit
        if (random(3) == 0) 
        {
          Pixels.setPixelColor(i, COLOR_BANSHEE_WAVE);
        } 
        else 
        {
          Pixels.setPixelColor(i, 0);
        }
      } 
      else 
      {
        Pixels.setPixelColor(i, 0);
      }
    }
    Pixels.show();
    delay(30);
  }

  //  reset pixels
  ResetPixels();
  delay(2200);

  //  Reset halo ring
  TransitionFade(_CurrentPixelColor, 5);
}

/////////////////////////
//  EMP Jammer Effect
/////////////////////////
void JammerEffect() 
{
  Serial.println("Jammer");
  PlayAudioClip(JammerAudio);

  delay(400);
  int effectLength = 4500;
  double effectBrightness = 0;
  uint32_t effectStart = millis();
  double brightnessPerCycle = 0.25;

  while ((millis() - effectStart) <= effectLength) 
  {
    int cycleBrightness = random(effectBrightness - 10, effectBrightness + 15);
    
    if (cycleBrightness < 0) 
    {
      cycleBrightness = 0;
    } 
    else if (cycleBrightness > 255) 
    {
      cycleBrightness = 255;
    }

    Pixels.setBrightness(cycleBrightness);
    for (uint8_t i = 0; i < LED_COUNT; i++) 
    {
      Pixels.setPixelColor(i, COLOR_JAMMER_CHARGE);
    }
    Pixels.show();
    delay(25);

    effectBrightness += brightnessPerCycle;
    if (effectBrightness > 245) 
    {
      effectBrightness = 245;
    }
  }

  //  Sparks effect after discharge
  SparklePixels(1500, COLOR_JAMMER_DISCHARGE, DEFAULT_BRIGHTNESS);

  delay(400);

  TransitionFade(_CurrentPixelColor, 5);
}

/////////////////////////
//  Enter/Exit Dark Zone
/////////////////////////
void ToggleDarkZone() 
{
  if (_IsInDarkZone) 
  {
    //  Exit the Dark Zone
    _IsInDarkZone = false;
    if (_IsRogue) 
    {
      ExitRogueEffect();
    }
    PlayAudioClip(ExitingDarkZone);
  } 
  else 
  {
    //  Enter the Dark Zone
    _IsInDarkZone = true;
    PlayAudioClip(EnteringDarkZone);
  }
}

void DarkZoneEffects() 
{
  ++CurrentDZClip;
  if (CurrentDZClip >= DZ_CLIPS_COUNT) 
  {
    CurrentDZClip = 0;
    ShuffleArray(DZClipOrderArray, DZ_CLIPS_COUNT);
  }

  if (DZClipOrderArray[CurrentDZClip] == HeloGalExtractionCalled) 
  {
    ExtractionEffect(true);
  } 
  else 
  {
    PlayAudioClip(DZClipOrderArray[CurrentDZClip]);
  }
}

void ExtractionEffect(bool CallNewExtraction) {
  if (CallNewExtraction) {
    Serial.println("Calling for new extraction...");
    _IsExtractionCalled = true;
    _ExtractionLastEvent = millis();
    _ExtractionPhase = HeloGalExtractionCalled;
    PlayAudioClip(_ExtractionPhase);
  } 
  else if (_IsExtractionCalled && (millis() - _ExtractionLastEvent) >= 15000) 
  {
    //  Its been 15 seconds since the last sound clip, play
    //  the next extracton clip
    _ExtractionPhase++;
    PlayAudioClip(_ExtractionPhase);
    _ExtractionLastEvent = millis();
    if (_ExtractionPhase >= HeloGalReturningToBase) 
    {
      //  This is the end of the extraction clips
      //  extraction is now finished
      _IsExtractionCalled = false;
      _LastEffectTime = millis();
      Serial.println("Extraction complete");
    }
  } 
  else 
  {
    //  Do nothing this cycle and keep cycling till the next clip
    //  is supposed to be played
  }
}

/////////////////////////
//  Random Chatter
/////////////////////////
void RandomChatter() 
{
  ++CurrentChatterClip;
  if (CurrentChatterClip >= CHATTER_CLIPS_COUNT) 
  {
    CurrentChatterClip = 0;
    ShuffleArray(ChatterClipOrderArray, CHATTER_CLIPS_COUNT);
  }

  PlayAudioClip(ChatterClipOrderArray[CurrentChatterClip]);
}

void CycleAudioMode() 
{
  //  reset the effect timer for Auto
  _LastEffectTime = millis();

  _CurrentEffectMode++;
  if (_CurrentEffectMode >= EffectModeCount) 
  {
    _CurrentEffectMode = Manual;
  }

  switch (_CurrentEffectMode) 
  {
    case Manual:
      Serial.println("Auto chatter disabled");
      PlayAudioClip(SkillCancel);
      break;

    case Auto:
      Serial.println("Auto chatter enabled");
      PlayAudioClip(Chirp);
      break;
  }
}

void ActivateSkill(uint8_t Skill) 
{
  switch (Skill) 
  {
    case DZScan:
      DZScanEffect();
      break;

    case Pulse:
      PulseEffect();
      break;

    case Banshee:
      BansheeEffect();
      break;

    case Jammer:
      JammerEffect();
      break;
  }
}

void CycleSkills() 
{
  _CurrentSkill++;
  if (_CurrentSkill >= SkillCount) 
  {
    _CurrentSkill = 0;
  }
  ShowSkillsHalo();
}

void ShowSkillsHalo() 
{
  //  Create the array and reset all the pixels to off
  uint32_t haloUpdate[LED_COUNT] = {};
  SetHaloArray(haloUpdate, LED_COUNT, 0x000000);

  //  Pixels at 0, 4, 8, 12 will stay dark to represnet the Axis
  //  to split the halo into 4 sections, one for each skill
  //
  switch (_CurrentSkill) {
    case Pulse:
      haloUpdate[1] = COLOR_RED;
      haloUpdate[2] = COLOR_RED;
      haloUpdate[3] = COLOR_RED;
      break;

    case DZScan:
      haloUpdate[5] = COLOR_DZ_SCAN;
      haloUpdate[6] = COLOR_DZ_SCAN;
      haloUpdate[7] = COLOR_DZ_SCAN;
      break;

    case Banshee:
      haloUpdate[9] = COLOR_SHD_CUSTOM_DARK;
      haloUpdate[10] = COLOR_SHD_CUSTOM_DARK;
      haloUpdate[11] = COLOR_SHD_CUSTOM_DARK;
      break;

    case Jammer:
      haloUpdate[13] = COLOR_JAMMER_CHARGE;
      haloUpdate[14] = COLOR_JAMMER_CHARGE;
      haloUpdate[15] = COLOR_JAMMER_CHARGE;
      break;

    default:
      Serial.println("I got a rock!");
  }

  UpdatePixelsFromArray(haloUpdate, LED_COUNT);
}

//  Helper that will just set the default brightness for the pixel update
void UpdatePixelsFromArray(uint32_t HaloArray[], size_t ArraySize) 
{
  UpdatePixelsFromArray(HaloArray, ArraySize, DEFAULT_BRIGHTNESS);
}

//  Set the pixels of the halo to the colors specified in the passed in array
void UpdatePixelsFromArray(uint32_t HaloArray[], size_t ArraySize, uint8_t Brightness) 
{
  Pixels.setBrightness(Brightness);
  for (uint8_t i = 0; i < ArraySize && i < LED_COUNT; i++) 
  {
    Pixels.setPixelColor(i, HaloArray[i]);
  }
  Pixels.show();
}

void SetHaloArray(uint32_t HaloArray[], size_t ArraySize, uint32_t Color) 
{
  for (uint8_t i = 0; i < ArraySize; i++) 
  {
    HaloArray[i] = Color;
  }
}

//  This API will handle the pause and then play that
//  we use frequently and also handle skipping the audio
//  if we are in silent mode.
void PlayAudioClip(uint8_t file) 
{
  //  This used to be needed, but in the latest compiler
  //  it does not appear to be needed.
  //_DFPlayer.pause();

  _ChatterThreshold = CalculateChatThreshold(CHATTER_PIN);
  Serial.print(F("Playing "));
  Serial.print(file);
  Serial.println("... ");

  _DFPlayer.play(file);
}

//  Calculates the average and Max analog value coming from the
//  the pin watching the voltage going to the speaker for audio
//  This should be fired when nothing is playing so that it gets
//  an estimate of what the 'quiet' analog values are so that the
//  chatter blinking effect does not happen without an audio file
//  playing.  The threshold to ignore / take action on the analog
//  input is the calculated from this info.
//  The +8 to the max was determined by just what seemed to work
//  the best.
int CalculateChatThreshold(int AnalogPin) 
{
  uint8_t sampleCount = 50;
  uint32_t sum = 0;
  uint32_t currentSample = 0;
  uint32_t maxSample = 0;

  for (uint8_t i = 0; i < sampleCount; i++) 
  {
    currentSample = analogRead(AnalogPin);
    sum += currentSample;
    if (currentSample > maxSample) 
    {
      maxSample = currentSample;
    }
  }

  _ChatterThreshold = maxSample + _AudioVolume * 2;
  if (_AudioVolume < 6) 
  {
    _ChatterThreshold += 6;
  }

  return _ChatterThreshold;
}

//  Increases the volume by one step
void VolumeIncrease() 
{
  if (_AudioVolume >= (VOLUME_MAX - VOLUME_STEP)) 
  {
    //  We are already at or above the max
    _AudioVolume = VOLUME_MAX;
  } 
  else 
  {
    //  Go in increments of 5 for volume
    _AudioVolume += VOLUME_STEP;
  }

  _DFPlayer.volume(_AudioVolume);  //Set volume value. From 0 to 30
  PlayAudioClip(Div2SingleTic);

  // we are at a new volume level so reset the max chatter effect level
  HighestChatterLevel = 0;
}

//  Decreases the volume level by one step
void VolumeDecrease() 
{
  if (_AudioVolume <= VOLUME_STEP) {
    //  We are already at the low
    _AudioVolume = 0;
  } 
  else 
  {
    //  Go in increments of 5 for volume
    _AudioVolume -= VOLUME_STEP;
  }

  _DFPlayer.volume(_AudioVolume);  //Set volume value. From 0 to 30
  PlayAudioClip(Div2SingleTic);

  // we are at a new volume level so reset the max chatter effect level
  HighestChatterLevel = 0;
}


void SetHaloToVolumeValue() 
{
  uint8_t i;
  double volumePercent = (double)_AudioVolume / (double)VOLUME_MAX;
  uint8_t ledsToShow = ((double)(LED_COUNT - 1) * volumePercent) + 1;

  Pixels.setPixelColor(0, COLOR_GREEN);

  //  Start from the top right and then light
  //  how many pixels we need along the way.
  //  Once we hit the limit, just leave the rest
  //  dark.
  for (uint8_t i = LED_COUNT; i > 0; i--) 
  {
    if (ledsToShow > 0) {
      Pixels.setPixelColor(i, COLOR_GREEN);
      ledsToShow--;
    } else {
      Pixels.setPixelColor(i, 0);
    }
  }

  Pixels.show();
}

//  Resets all pixels to the specificed color and brightness
//  If called without parameters i.e. ResetPixels() it will reset them
//  to 'off'
void ResetPixels()
{
  ResetPixels(0, DEFAULT_BRIGHTNESS);
}

void ResetPixels(uint32_t Color)
{
  ResetPixels(Color, DEFAULT_BRIGHTNESS);
}

void ResetPixels(uint32_t Color, uint8_t Brightness) 
{
  Pixels.setBrightness(Brightness);
  for (uint8_t i = 0; i < LED_COUNT; i++) 
  {
    Pixels.setPixelColor(i, Color);
  }
  Pixels.show();
}

//  Does the sparkle effect of flashing a random pixel shortly then turning it off
void SparklePixels(uint32_t EffectLength, uint32_t SparkleColor, uint8_t Brightness)
{
  SparklePixels(EffectLength, SparkleColor, Brightness, 0, 10);
}

void SparklePixels(uint32_t EffectLength, uint32_t SparkleColor, uint8_t Brightness, uint32_t BaseColor)
{
  SparklePixels(EffectLength, SparkleColor, Brightness, BaseColor, 10);
}

void SparklePixels(uint32_t EffectLength, uint32_t SparkleColor, uint8_t Brightness, uint32_t BaseColor, uint8_t Delay) 
{
  uint32_t effectStart = millis();
  Pixels.setBrightness(Brightness);
  while ((millis() - effectStart) <= EffectLength) 
  {
    uint8_t i = rand() % 16;
    Pixels.setPixelColor(i, SparkleColor);
    Pixels.show();
    delay(Delay);
    Pixels.setPixelColor(i, BaseColor);
    Pixels.show();
  }
}

//  Fades all pixes from their current color to the one specified
//  by increments of 0x01 on each color
void TransitionFade(uint32_t TargetColor, uint16_t DelaySpeed) 
{
  uint32_t fadeStart = millis();

  uint8_t targetR = TargetColor >> 16;          // 0xRRGGBB -> bit shift 16 bits (8 bytes) -> 0x0000RR
  uint8_t targetG = (TargetColor >> 8) & 0xFF;  // 0xRRGGBB -> bit shift 8 bits  (4 bytes) -> 0x00RRGG ->>
                                                // & with 0xFF strips off all but the first 2 bytes -> 0x0000GG
  uint8_t targetB = TargetColor & 0xFF;         // 0xRRGGBB -> & with 0xFF strips off all but the first 2 bytes -> 0x0000BB

  //  If the brightness is less than 255 (full) then + or - by 0x01
  //  on a color won't neccesarily change the color, so the next time you check
  //  the pixel it will still be the 'old' value.
  //  Therefore, we need to do the increment and checking with our own array
  //  for simplicity of tracking and changing RGB values we create a 2 dimensional
  //  array.  Each pixel will have a row of 3 floats's that will represent R [i][0],
  //  G [i][1], and B [i][2], [i][3] in the transition array is reserved for how many
  //  steps are needed to get to the target value.  This will be used as a counter
  //  and decremented each step we take till there are 0 left
  float currentPixelColorArray[LED_COUNT][3] = {};
  float transitionPixelArray[LED_COUNT][4] = {};

  //  Now set the current array to the pixel color values on the array at the moment
  for (uint8_t i = 0; i < LED_COUNT; i++) 
  {
    uint32_t currentPixelColor = Pixels.getPixelColor(i);

    // R 0xRRGGBB -> bit shift 16 bits (8 bytes) -> 0x0000RR
    currentPixelColorArray[i][0] = currentPixelColor >> 16;

    // G 0xRRGGBB -> bit shift 8 bits  (4 bytes) -> 0x00RRGG ->>
    //   & with 0xFF strips off all but the first 2 bytes -> 0x0000GG
    currentPixelColorArray[i][1] = (currentPixelColor >> 8) & 0xFF;

    // B 0xRRGGBB -> & with 0xFF strips off all but the first 2 bytes -> 0x0000BB
    currentPixelColorArray[i][2] = currentPixelColor & 0xFF;

    //  Now calculate the delta that each of these needs to be adjusted
    transitionPixelArray[i][0] = targetR - currentPixelColorArray[i][0];
    transitionPixelArray[i][1] = targetG - currentPixelColorArray[i][1];
    transitionPixelArray[i][2] = targetB - currentPixelColorArray[i][2];

    int deltaR = transitionPixelArray[i][0];
    int deltaG = transitionPixelArray[i][1];
    int deltaB = transitionPixelArray[i][2];

    if (deltaR < 0) 
    {
      deltaR = deltaR * (-1);
    }

    if (deltaG < 0) 
    {
      deltaG = deltaG * (-1);
    }

    if (deltaB < 0) {
      deltaB = deltaB * (-1);
    }

    int highestDelta = deltaR;
    if (highestDelta < deltaG) 
    {
      highestDelta = deltaG;
    }

    if (highestDelta < deltaB) 
    {
      highestDelta = deltaB;
    }

    //  make sure this is not 0
    if (highestDelta == 0) 
    {
      highestDelta = 1;
    }

    //  now that we know the highest delta, we can calculate
    //  by how much we need to step each individual color of the
    //  pixel so that we arrive at the target color at the same time
    //  this helps with the blending effect not to make odd colors
    //  especially when coming from 'off' to orange where it will make a more
    //  green ring before the red is fully added in

    transitionPixelArray[i][0] = transitionPixelArray[i][0] / (float)highestDelta;
    transitionPixelArray[i][1] = transitionPixelArray[i][1] / (float)highestDelta;
    transitionPixelArray[i][2] = transitionPixelArray[i][2] / (float)highestDelta;
    transitionPixelArray[i][3] = highestDelta;  //  this will be our steps remaining counter
  }

  //  Create the tracking array
  bool changesMade = true;
  while (changesMade) 
  {
    changesMade = false;  // set it to false and it will get set to true if
                          // we had to make any changes
    for (uint8_t i = 0; i < LED_COUNT; i++) 
    {
      //  Check if we have done enough transition steps on this pixel
      //  from our previous calculations.  If not, then do the changes
      if (transitionPixelArray[i][3] > 0) 
      {
        changesMade = true;
        currentPixelColorArray[i][0] += transitionPixelArray[i][0];
        currentPixelColorArray[i][1] += transitionPixelArray[i][1];
        currentPixelColorArray[i][2] += transitionPixelArray[i][2];
        //  reduce the remaining steps by 1
        transitionPixelArray[i][3]--;

        //  write the updated pixels
        Pixels.setPixelColor(i,
                             currentPixelColorArray[i][0],
                             currentPixelColorArray[i][1],
                             currentPixelColorArray[i][2]);
      }
    }
    Pixels.show();
    delay(DelaySpeed);
  }
}

void ShuffleArray(uint8_t Array[], size_t Size) 
{
  //  We will use a nifty method to shuffle a set of
  //  fixed numbers by just swapping them randomly
  // http://www.cplusplus.com/forum/beginner/91557/
  int seed = millis();
  srand(seed);
  for (uint8_t i = Size - 1; i > 0; --i) 
  {
    //  get the index we want to swap i with
    int j = rand() & i;
    //  now swap Array[i] with Array[j]
    uint8_t tmp = Array[i];
    Array[i] = Array[j];
    Array[j] = tmp;
  }
  DebugPrintArray(Array, Size);
}

void DebugPrintArray(uint8_t Array[], size_t Size) 
{
  Serial.print("{");
  for (uint8_t i = 0; i < Size; i++) 
  {
    if (i != 0) 
    {
      Serial.print(",");
    }
    Serial.print(Array[i]);
  }
  Serial.println("}");
}

void TestAudioFiles() 
{
  //  play the files 1 by 1 in the order of their global IDs (which is the
  //  order they were copied to the SD card, not their name)
  for (int i = 1; i < ISACAudioCount; i++) 
  {
    PlayAudioClip(i);
    Serial.print(F("Playing track "));
    Serial.println(i);
    delay(4000);
  }
}
