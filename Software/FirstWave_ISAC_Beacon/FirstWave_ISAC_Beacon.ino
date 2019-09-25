
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

    -   Clean up the code more / find a cleaner way to do all of the button tracking in the 
        loop() function.  That was some of the last code written and was not as clean or 
        elegant of a solution, it was pretty much 'hack it together to get it to work and
        get the initial release out'.  I think it is overly complicated but was struggling to
        simplify it with what little time I had at that moment.
    -   Make the flare fire off with a specific button press like 3 taps rather than just be part
        of the random chatter.
    -   Add in an effect mode where the Halo does some things so it isn't just a constant color
        for if you were walking around a convention or something so it is more 'flashy' and eye
        catching.  Something similar to the throbbing or effects in Guido666's code possibly.
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
DFRobotDFPlayerMini myDFPlayer;

//  This initializes the ~3 and ~5 digital pins for a serial connection
//  port we use to control the DFPlayer Mini
SoftwareSerial dfPlayerSerial(DD3, DD5); // RX, TX

//  Global variables and constants used for audio control
uint8_t         AudioVolume         = 20;   // Start volume at 70%
uint32_t        LastVolumeChange    = 0;
const uint8_t   AudioVolumeMax      = 30;   // DFPlayer Max volume is 30
const uint8_t   AudioVolumeStep     = 2;    // Value change of volume each step
const uint16_t  AudioVolumeStepWait = 110;  // Milliseconds between steps

//  Provided from the DFPlayer Mini example code, is useful for 
//  debugging
void DFDebugPrintDetail(uint8_t type, int value);

//  The way the DFPlayer Mini works is that the global # of a track is 
//  determined by listing the files as they were phsyically coppied onto
//  the SD card.  For example, re-copying the first file to the card will
//  change its order in global #'s and it will now be last
//  This enum should correspond to each sfx's global ID in order as they
//  should be copied to the SD in the setup instructions.
enum SoundEffects 
{
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
const uint8_t DZClipsStart = AgentOutOfAction;
const uint8_t DZClipsEnd = RogueAgentEliminated;
const uint8_t DZClipsCount = DZClipsEnd - DZClipsStart + 2;  // +2 because we need one more spot for the Extraction effect
                                                             // in the array so it can be fired off with the other effects
uint8_t DZClipOrderArray[DZClipsCount] = {};
uint8_t CurrentDZClip = 0;

const uint8_t ChatterClipsStart = VitalSignsCritical;
const uint8_t ChatterClipsEnd = SuppliesDetected;
const uint8_t ChatterClipsCount = ChatterClipsEnd - ChatterClipsStart + 1;
uint8_t ChatterClipOrderArray[ChatterClipsCount] = {};
uint8_t CurrentChatterClip = 0;

//////////////////////////////////////////////////////
//  Side button (+) and (-) config
//////////////////////////////////////////////////////

//  The Pins that our + and - external buttons on the beacon are hooked up to
//  We are using analog pins here, but could also use digital pins.
const uint8_t MinusButtonPin    = A0;
const uint8_t PlusButtonPin     = A1;

//  These are used to help keep rack of the button press
//  functionality where we are watching for both sequential
//  tapping and holding of each button or both together.
//  As mentioned in the things to imrpove, this could be
//  simplified more than likely.
bool MinusButtonActionCleared   = true; // Helps prevent double hits on buttons
bool PlusButtonActionCleared    = true;
bool MinusButtonIgnoreNextRelease= false;   //  Used when we are holding so we
bool PlusButtonIgnoreNextRelease = false;   //  don't fire off an event on release
bool MinusButtonReleaseHandled  = true; // Helps ensure we handle button releases properly
bool PlusButtonReleaseHandled   = true; // and don't double hit our logic code
bool ComboButtonActionCleared   = true;
bool ComboButtonReleaseHandled  = true;
bool IgnoreSingleButtonHold     = false;
uint8_t MinusButtonTimesTapped  = 0;
uint8_t PlusButtonTimesTapped   = 0;
uint32_t MinusButtonHeldSince   = 0; // when the button was pressed but not yet
uint32_t PlusButtonHeldSince    = 0; // released
uint32_t MinusButtonReleased    = 0; // These help to watch for sequential taps and time out
uint32_t PlusButtonReleased     = 0; // when to fire off an effect after tapping has finished

/////////////////////////////////////////////////////////
//  NeoPixel 16 config (aka the beacon's halo)
/////////////////////////////////////////////////////////
const uint8_t PixelCount = 16;
const uint8_t PixelPin   = 6;   // Data pin for neopixel
const uint8_t ChatterPin = A4;  // Analog pin used to detect talking volume for
                                // the chatter blinking effect

Adafruit_NeoPixel Pixels = Adafruit_NeoPixel(PixelCount, PixelPin);


const uint8_t PixelDefaultBrightness = 85;
const uint8_t PixelChatterBrightness = 45; // Brightness to go to as the base 
                                           // when Isac is talking
const uint8_t PixelChatterMaxBrightness = 150;
uint16_t HighestChatterLevel = 0;

//  Helper functions with setting the pixel's colors or resetting them
void ResetPixels(uint32_t Color = 0, uint8_t Brightness = PixelDefaultBrightness);
void SparklePixels(uint32_t EffectLength, uint32_t SparkleColor, uint8_t Brightness, uint32_t BaseColor = 0, uint8_t Delay = 10);
void UpdatePixelsFromArray(uint32_t HaloArray[], size_t ArraySize, uint8_t Brightness = PixelDefaultBrightness);

////////////////////////////////////////////////////////////
//  Pre-defined Colors used for effects and such
////////////////////////////////////////////////////////////
const uint32_t PixelColor_Red       = 0xFF0000;
const uint32_t PixelColor_Green     = 0x00FF00;
const uint32_t PixelColor_Blue      = 0x0000FF;

//  True SHD 'Division' Orange is (255, 109, 16), but on the
//  Neopixel 16 this looks too yellow and not orange ehough
const uint8_t PixelColor_SHD       = 0xFF6D10; 

//  These are custom color shades that look better on the 
//  Neopixel in terms of SHD orange when being displayed
const uint32_t PixelColor_SHDCustom     = 0xFF4C00; //  This one is still more 'yellow' but is good for some effects
const uint32_t PixelColor_SHDCustomDark = 0XFF2A00; //  This seems to be the best represntation of orange on the Neopixel
                                                    //  and is a darker version of SHDCustom, since this looks best this
                                                    //  is the default halo color

//  Other colors used by skills
const uint32_t PixelColor_DZScan          = 0xe500e5;
const uint32_t PixelColor_BansheeWave     = 0x69aaff;
const uint32_t PixelColor_JammerCharge    = 0x3030ff;
const uint32_t PixelColor_JammerDischarge = 0x5055ff;

//  These are used for convenience during debugging to change default color 
//  scheme with changing one variable rather than all over the code base.
const uint32_t PixelColor_Default     = PixelColor_SHDCustomDark;
const uint32_t PixelColor_DefaultDark = PixelColor_SHDCustomDark;

/////////////////////////////////////////////////////////////////////
//  Global state variables that are used across loop function cycles
/////////////////////////////////////////////////////////////////////
uint32_t PixelCurrentColor = PixelColor_Default;  // Current color... start orange

bool     IsRogue            = false;
bool     IsInDarkZone       = false;
bool     IsExtractionCalled = false;
uint8_t  ExtractionPhase;
uint32_t ExtractionLastEvent;

uint32_t ChatterAverageLastChecked;
uint32_t ChatterThreshold;  // Value that if the analog pin is higher, does the
                            // the chatter effect halo blinking

//  This enum is used to set the order of the skills and
//  how the skill wheel will cycle between them in order
enum Skills
{
    Pulse,      // Red
    Jammer,     // Blue
    Banshee,    // Orange
    DZScan,     // Purple
    SkillCount
};

enum AudioModes {
    Manual = 0,
    Auto,
    AudioModeCount  // used as variable to know when we have cycled all the 
                    // modes and need to loop back to the first
};

uint8_t  CurrentSkill       = Pulse;  // used to cycle the skill powers
uint8_t  CurrentAudioMode   = Manual;  // used to cycle between functionality modes

uint32_t AutoEffectWaitTime = 60000; // 1 minutes between auto audio effects
uint32_t LastEffectTime;

///////////////////////////////////////////////////////////////////
//  Initial Start-up and continuous looping functions
///////////////////////////////////////////////////////////////////
void setup()
{
    //  Initialize the Neopixel / halo
    Pixels.begin();
    Pixels.setBrightness(PixelDefaultBrightness);
    LastEffectTime = millis();

    //  Serial port to control the DFMini Player (must be 9600)
    dfPlayerSerial.begin(9600);

    //  Serial port for debug output (you will need an FTDI cable if
    //  running on the Trinket Pro 5V)
    Serial.begin(115200);

    // Initialize the button and chatter pins as input.  Since we are not using a draw
    // resister these will be INPUT_PULLUP
    pinMode(MinusButtonPin, INPUT_PULLUP);
    pinMode(PlusButtonPin,  INPUT_PULLUP);
    pinMode(ChatterPin,     INPUT_PULLUP);
    //  Todo: start final code scrub here tomorrow.
    Serial.println();
    Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));

    if (!myDFPlayer.begin(dfPlayerSerial)) {  //Use softwareSerial to communicate with mp3.
        Serial.println(F("Unable to begin:"));
        Serial.println(F("1.Please recheck the connection!"));
        Serial.println(F("2.Please insert the SD card!"));
        digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
        while (true);
    }
    Serial.println(F("DFPlayer Mini online."));

    //  Seed our random number generator
    int seed = millis();
    srand(seed);
    
    //  Initialize the world and DZ chatter arrays
    for (uint8_t i = 0; i < ChatterClipsCount; i++)
    {
        ChatterClipOrderArray[i] = ChatterClipsStart + i;
    }
    ShuffleArray(ChatterClipOrderArray, ChatterClipsCount);

    //  this sets the Extraction event into the DZ chatter
    //  effects to be fired
    DZClipOrderArray[0] = HeloGalExtractionCalled;

    //  Fill the other spaces in the array with the other DZ Chatter
    //  effects.
    for (uint8_t i = 1; i < DZClipsCount; i++)
    {
        DZClipOrderArray[i] = DZClipsStart + i - 1;
    }

    ShuffleArray(DZClipOrderArray, DZClipsCount);

    myDFPlayer.volume(AudioVolume);  //Set volume value. From 0 to 30

    //  Initialize Boot-up effect
    BootUpEffect();

    //  Tests code for debugging can go here

}

//  Continuous loop / main thread
void loop()
{
    //  Check the analog value on the chatter pin, if it is above
    //  the threshold, then change the brightness acording to the
    //  value for the blinking chatter effect
    int chatterPinValue = analogRead(ChatterPin);
    int chatterLevel = chatterPinValue - ChatterThreshold;
    if (chatterLevel > 0)
    {
        if( chatterLevel > HighestChatterLevel )
        {
          HighestChatterLevel = chatterLevel;
        }

        //  Get the % brightness of the max brightness based on the chatter level vs
        //  the highest catter level we've seen
        int chatterBrightness = (double)PixelChatterMaxBrightness * ((double)chatterLevel / (double)HighestChatterLevel);

        //  todo: put this in a util function
        //Serial.print("Talking ");
        //Serial.print(" ");
        //for (int i = 0; i < chatterLevel; i++)
        //{
        //    Serial.print("|");
        //}
        //Serial.println();
        
//        double chatterAdd = (((double)chatterLevel / (double)AudioVolume)) * 7;//;((double)AudioVolumeMax / (double)AudioVolume); //* (.((log10(AudioVolumeMax - AudioVolume))));
//        int chatterBrightness = PixelChatterBrightness + chatterAdd;
//        
        //Serial.print("Brightness level: ");
        //Serial.println(PixelChatterBrightness + chatterBrightness);

        Pixels.setBrightness(chatterBrightness);
        for (uint8_t i = 0; i < PixelCount; i++)
        {
            Pixels.setPixelColor(i, PixelCurrentColor);
        }

        Pixels.show();
        delay(70);
    }
    //  If the chatter isn't happening, then reset to the standard brightness
    else if (Pixels.getBrightness() != PixelDefaultBrightness)
    {
        Pixels.setBrightness(PixelDefaultBrightness);
        for (uint8_t i = 0; i < PixelCount; i++)
        {
            Pixels.setPixelColor(i, PixelCurrentColor);
        }
        Pixels.show();
    }

    //  todo: refactor this code after button layout changes
    // read the state of the pushbutton values for this cycle
    uint8_t MinusButtonState = digitalRead(MinusButtonPin);
    uint8_t PlusButtonState  = digitalRead(PlusButtonPin);

    if (MinusButtonState == LOW && MinusButtonReleaseHandled == true)
    {
        Serial.println(F("(-) button pushed"));
        MinusButtonReleaseHandled = false;
        MinusButtonActionCleared = false;
        MinusButtonHeldSince = millis();
    }
    else if (MinusButtonState == LOW &&      // Button is still pressed
            !MinusButtonReleaseHandled &&
            !IgnoreSingleButtonHold) //skip this if this is a double press state
    {
        //  Button is being held, check how long and if it is
        //  more than 2 seconds, go into volume control
        if (millis() - MinusButtonHeldSince >= 2000)
        {
            Serial.println(F("(-) button held for 2 seconds"));
            MinusButtonIgnoreNextRelease = true;

            Serial.println(F("Stepping volume down"));

            //  Go into a loop to change the volume of the device
            //  re-check to see if the button was let go every 50ms
            //  update the halo LEDs to indicate current volume
            do
            {
                //  Step down the volume 
                if (millis() - LastVolumeChange >= AudioVolumeStepWait)
                {
                    LastVolumeChange = millis();
                    if (AudioVolume != 0)
                    {
                        VolumeDecrease();
                    }
                    SetHaloToVolumeValue();
                }

                delay(50);
                MinusButtonState = digitalRead(MinusButtonPin);
            } while (MinusButtonState == LOW);

            //  Leave the volume array up for half a second
            //  for visual conformation after release
            delay(500);
            ResetPixels(PixelCurrentColor);
        }
    }
    else if (MinusButtonState == HIGH &&
            !MinusButtonActionCleared)
    {
        if (MinusButtonIgnoreNextRelease)
        {
            Serial.println("(-) button released - ignorning");
            MinusButtonIgnoreNextRelease = false; // we have now skipped this release
            MinusButtonActionCleared = true;
            MinusButtonReleaseHandled = true;
            MinusButtonReleased = 0;
            MinusButtonTimesTapped = 0;
        }
        else if (!MinusButtonReleaseHandled)
        {
            //  This is a new button release, it could be the first
            //  or nth in a sequence depending on the value of when it
            //  was last released
            Serial.println("(-) button released");
            if (MinusButtonReleased == 0)
            {
                //  This is the first button release record the time
                //  and let our auto timer handle the result
                MinusButtonReleased = millis();
                MinusButtonTimesTapped = 1;
            }
            //  If the last tap was within 250 ms, this is a sequential tap (double)
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
                Serial.println("(-) button - ignoring double read of release");
            }
            MinusButtonReleaseHandled = true;    //  Mark that we handled this release
        }
        else
        {
            //uint32_t timer = millis() - MinusButtonReleased;

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
                    if (IsInDarkZone)
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
                MinusButtonReleased    = 0;     //  reset to watch for next tap sequence
                MinusButtonTimesTapped = 0;
                LastEffectTime = millis();
            }
        }
    }

    if (PlusButtonState == LOW && PlusButtonReleaseHandled == true)
    {
        Serial.println("(+) button pushed");
        PlusButtonReleaseHandled = false;
        PlusButtonActionCleared = false;
        PlusButtonHeldSince = millis();
    }
    else if (PlusButtonState == LOW &&      // Button is still pressed
            !PlusButtonReleaseHandled &&
            !IgnoreSingleButtonHold) //skip this if this is a double press state
    {
        //  Button is being held, check how long and if it is
        //  more than 1 second, play a skill.  If it is
        //  more than 3 seconds togle the Auto / Manual mode
        if (millis() - PlusButtonHeldSince >= 2000)
        {
            Serial.println("(+) plus button held for 2 seconds");
            Serial.println("  Stepping volume up");
            PlusButtonIgnoreNextRelease = true;

            //  Go into a loop to change the volume of the device
            //  re-check to see if the button was let go every 50ms
            //  update the halo LEDs to indicate current volume
            do
            {
                //  Step down the volume every 1.5 seconds
                //Serial.println(F("MINUS BUTTON HELD FOR 4 SECONDS!"));
                if (millis() - LastVolumeChange >= AudioVolumeStepWait)
                {

                    LastVolumeChange = millis();
                    if (AudioVolume < AudioVolumeMax)
                    {
                        VolumeIncrease();
                    }
                    SetHaloToVolumeValue();
                }

                delay(50);
                PlusButtonState = digitalRead(PlusButtonPin);
            } while (PlusButtonState == LOW);

            delay(500);
            ResetPixels(PixelCurrentColor);
        }
    }
    else if (PlusButtonState == HIGH && 
             !PlusButtonActionCleared)
    {
        if (PlusButtonIgnoreNextRelease)
        {
            Serial.println("(+) button released - ignorning");
            PlusButtonIgnoreNextRelease = false; // we have now skipped this release
            PlusButtonActionCleared = true;
            PlusButtonReleaseHandled = true;
            PlusButtonReleased = 0;
        }
        else if( !PlusButtonReleaseHandled )
        {
            //  This is a new button release, it could be the first
            //  or nth in a sequence depending on the value of when it
            //  was last released
            Serial.println(F("(+) button released"));
            if (PlusButtonReleased == 0)
            {
                //  This is the first button release record the time
                //  and let our auto timer handle the result
                PlusButtonReleased = millis();
                PlusButtonTimesTapped = 1;
            }
            //  If the last tap was within 250 ms, this is a sequential tap (double)
            else if (millis() - PlusButtonReleased <= 700)
            {
                PlusButtonTimesTapped++;
                Serial.print("(+) button tapped ");
                Serial.print(PlusButtonTimesTapped);
                Serial.println(" times");
                PlusButtonReleased = millis();  //  start the release watch timer again
                CycleSkills();
                
            }
            else
            {
                Serial.println("(+) button - ignoring double read of release"); 
            }
            PlusButtonReleaseHandled = true;    //  Mark that we handled this release

        }
        else
        {
          uint32_t timer = millis() - PlusButtonReleased;
          
              //  State clearing actions for single presses or end of a sequence of taps
            if (millis() - PlusButtonReleased > 700)
            {
                Serial.println("(+) Button action timeout");
                ActivateSkill(CurrentSkill);
                //  We have now cleared the action from the imput
                PlusButtonActionCleared = true;
                PlusButtonReleased = 0;     //  reset to watch for next tap sequence
                
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
            Serial.println("(+) and (-) buttons pressed");
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
                LastEffectTime = millis();
            }
        }
    }
    else if (PlusButtonState == HIGH && MinusButtonState == HIGH 
             && !ComboButtonReleaseHandled)
    {
        Serial.println("(+) and (-) buttons released");
        //  This was a plus and minus button combo tap
        //  Only change rogue status if in Dark Zone
          if(!ComboButtonActionCleared)
          {
            if (IsInDarkZone)
            {
              if (IsRogue)
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
                //  Do nothing, we don't have functionality for a 2 button tap outside of the dark zone
                //  right now.
            }
            ComboButtonActionCleared = true;
          }
        
        ComboButtonReleaseHandled = true;
        IgnoreSingleButtonHold = false;
        LastEffectTime = millis();
    }

    //  Since the Extraction Effect self cycles a new clip every 15 seconds we
    //  call it every loop and let it decide if it needs to play the next clip
    ExtractionEffect(false);

    if (CurrentAudioMode == Auto)
    {
        //  Play an effect after waiting the defined time
        if (millis() - LastEffectTime >= AutoEffectWaitTime)
        {
            LastEffectTime = millis();
            Serial.println(F("Firing auto chatter effect"));
            if (IsInDarkZone)
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
    if (myDFPlayer.available()) {
        DFDebugPrintDetail(myDFPlayer.readType(), myDFPlayer.read()); //Print the detail message from DFPlayer to handle different errors and states.
    }
}

void DFDebugPrintDetail(uint8_t type, int value) {
    switch (type) {
    case TimeOut:
        Serial.println(F("Time Out!"));
        break;
    case WrongStack:
        Serial.println(F("Stack Wrong!"));
        break;
    case DFPlayerCardInserted:
        Serial.println(F("Card Inserted!"));
        break;
    case DFPlayerCardRemoved:
        Serial.println(F("Card Removed!"));
        break;
    case DFPlayerCardOnline:
        Serial.println(F("Card Online!"));
        break;
    case DFPlayerPlayFinished:
        Serial.print(F("Number:"));
        Serial.print(value);
        Serial.println(F(" Play Finished!"));
        ChatterThreshold = CalculateChatThreshold(ChatterPin);
        break;
    case DFPlayerError:
        Serial.print(F("DFPlayerError:"));
        switch (value) {
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
            break;
        }
        break;
    default:
        break;
    }
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
    myDFPlayer.volume(AudioVolumeMax-2);

    PlayAudioClip(Div2Startup);

    //Random Sparks then bright halo
    SparklePixels(1900, PixelColor_DefaultDark, PixelChatterBrightness, 0x000000, 20);

    ResetPixels(PixelColor_Default);
    delay(1500);

    //  Reset halo ring to blank
    ResetPixels(0x00, PixelDefaultBrightness);

    ////////////////////////////
    //  Circular Loading effect
    myDFPlayer.volume(AudioVolumeMax-4);
    PlayAudioClip(Div2LoadingTics);

    //  Need to set the first pixel for how I placed the neoPixel
    Pixels.setPixelColor(0, PixelColor_SHDCustom);
    Pixels.show();

    //  Now load the rest of the pixels
    for (int i = PixelCount; i > 0; i--)
    {
        delay(200);
        Pixels.setPixelColor(i, PixelColor_SHDCustom);
        Pixels.show();

    }

    PlayAudioClip(BootUp);

    SparklePixels(4200, 0xFFFFFF, PixelDefaultBrightness, PixelColor_SHDCustom, 20);

    //  This is a special sparkle where instead of dark background we have
    //  the colored ring, so we won't use the SparklePixel() function
//    uint32_t effectLength = 4200;
//    uint32_t start = millis();
//    Pixels.setBrightness(PixelDefaultBrightness); // Set to standard brightness
//    while ((millis() - start) <= effectLength)
//    {
//        uint8_t i = random(16);
//        Pixels.setPixelColor(i, 0xFFFFFF);
//        Pixels.show();
//        delay(20);
//        Pixels.setPixelColor(i, PixelColor_SHDCustom);
//        Pixels.show();
//    }
    delay(1200);

    TransitionFade(PixelCurrentColor, 2);
    //ResetPixels(PixelColor_Default);
    delay(1000);

    //  Before ISAC starts talking, reset the volume to the normal
    //  default level
    myDFPlayer.volume(AudioVolume);
    PlayAudioClip(ISACReactivatedAgentAuthenticated);
    Serial.println("finished");
}

/////////////////////////
//  Go Rogue Effect
/////////////////////////
void GoRogueEffect()
{
    uint32_t currentColor = 0;

    Serial.print("Going Rogue...");
    IsRogue = true;
    PlayAudioClip(RogueProtocol_SFX);

    //  Sparkle to clear ring then glow to Rogue
    SparklePixels(700, PixelColor_Red, PixelDefaultBrightness);

    uint32_t actualColor = 0;
    float cycleDelay = 15;
    for (currentColor = 0x000000; currentColor < PixelColor_Red; currentColor += 0x010000)
    {
        ResetPixels(currentColor);
        delay(cycleDelay);
    }
    PixelCurrentColor = currentColor;
    PlayAudioClip(RogueProtocol);
    Serial.println("Gone Rogue");
}

void ExitRogueEffect()
{
    //  Todo: Add audio clips
    uint8_t  currentBrightness;
    uint32_t currentColor = 0;

    Serial.print("Exiting Rogue...");
    PlayAudioClip(RogueStatusRemoved_SFX);

    //  Sparkle to clear ring
    SparklePixels(700, PixelColor_DefaultDark, PixelDefaultBrightness);

    uint8_t sectionCount = 5;
    uint16_t pixelsPerSection = PixelCount / sectionCount;
    uint8_t pixelsLeftOver = PixelCount % sectionCount;
    for (uint8_t section = 0; section < sectionCount; section++)
    {
        uint8_t startPixel = section * pixelsPerSection;
        uint8_t endPixel = startPixel + pixelsPerSection;
        for (uint8_t i = startPixel; i < endPixel; i++)
        {
            Pixels.setPixelColor(i, PixelColor_DefaultDark);
        }
        Pixels.show();
        delay(200);
    }

    //  Set all pixels to the brighter color
    ResetPixels(PixelColor_Default);
    PixelCurrentColor = PixelColor_Default;

    delay(200);
    IsRogue = false;
    Serial.println(F("Rouge status removed"));
    PlayAudioClip(RogueStatusRemoved);
}

/////////////////////////
//  Scan Effect
/////////////////////////
void DZScanEffect()
{
    Serial.println("DZ Scan");
    PlayAudioClip(DZScanAudio);

    delay(1000);

    ResetPixels(PixelColor_DZScan, 20);
    
    delay(600);
    int effectLength = 1200;
    double effectBrightness = 100.0;
    uint32_t effectStart = millis();
    double brightnessPerCycle = 0.2;

    while ((millis() - effectStart) <= effectLength)
    {
        Pixels.setBrightness(effectBrightness);
        for (uint8_t i = 0; i < PixelCount; i++)
        {
            Pixels.setPixelColor(i, PixelColor_DZScan);
        }
        Pixels.show();
        delay(2);

        effectBrightness = effectBrightness - brightnessPerCycle;
        if (effectBrightness < 0)
        {
            effectBrightness = 0;
        }
    }

    ResetPixels();
    SparklePixels(2000, PixelColor_DZScan, PixelDefaultBrightness, 0x000000, 35);
    delay(1600);

    ResetPixels(PixelColor_DZScan);
    delay(700);

    TransitionFade(PixelCurrentColor, 5);
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
        for (uint8_t i = 0; i < PixelCount; i++)
        {
            Pixels.setPixelColor(i, PixelColor_SHDCustom);
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
    uint8_t enemies[4] = { -1, -1, -1, -1 };
    uint8_t numberOfEnemies = rand() % 4;

    //  Make sure that there is at least one
    numberOfEnemies++;
    for (uint8_t i = 0; i < numberOfEnemies; i++)
    {
        enemies[i] = rand() % 15; // select a pixel to put this enemy on
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

        for (uint8_t i = 0; i < PixelCount; i++)
        {
            uint32_t color = Pixels.getPixelColor(i);
            Pixels.setPixelColor(i, PixelColor_SHDCustom);
            Pixels.show();
            delay(20);
            //  If this is an enemy position mark it red
            //  otherwise just turn it off
            if (i == enemyPos1 || i == (enemyPos1-1) ||
                i == enemyPos2 || i == (enemyPos2-1)) 
            {
                Pixels.setPixelColor(i, PixelColor_Red);
            }
            else
            {
                Pixels.setPixelColor(i, color);
            }
            Pixels.show();
        }
    }

    //  now restore all the other pixels
    for (uint8_t i = 0; i < PixelCount; i++)
    {
        uint32_t color = PixelColor_SHDCustom;
        for (uint8_t enemy = 0; enemy < numberOfEnemies; enemy++)
        {
            if (i == enemies[enemy] || i == (enemies[enemy]-1))
            {
                color = PixelColor_Red;
            }
        }
        Pixels.setPixelColor(i, color);
        Pixels.show();
    }
    delay(3000);

    TransitionFade(PixelCurrentColor, 5);
}

/////////////////////////
//  Banshee Effect
/////////////////////////
void BansheeEffect()
{
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
        double pixelsToHide = ((((double)PixelCount - 4.0) / 1.333) * chargeEffectPercent) + (PixelCount / 3);
        Pixels.setBrightness(cycleBrightness);
        for (uint8_t i = 0; i < PixelCount; i++)
        {

            uint32_t color = PixelColor_Default;
            //  Check the right side pixels to light / hide
            if ((i <= PixelCount / 2) && (i > (PixelCount / 2) - (pixelsToHide / 2)))
            {
                color = 0x000000;
            }
            //  Check the left side pixels to light / hide
            //if ((i < PixelCount - 2) && (i > ((PixelCount / 2) + (pixelsToHide / 2))))
            else if ((i >= PixelCount / 2) && (i < (PixelCount / 2) + (pixelsToHide / 2)))
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
    Pixels.setBrightness(PixelDefaultBrightness); // Set to standard brightness
    while ((millis() - effectStart) <= effectLength)
    {
        for (uint8_t i = 0; i < PixelCount; i++)
        {
            if ((i > PixelCount - 2) ||
                (i < 2))
            {
                // 50 / 50 chance this pixel gets lit
                if (random(3) == 0)
                {
                    Pixels.setPixelColor(i, PixelColor_BansheeWave);
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
    TransitionFade(PixelCurrentColor, 5);
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
        for (uint8_t i = 0; i < PixelCount; i++)
        {
            Pixels.setPixelColor(i, PixelColor_JammerCharge);
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
    SparklePixels(1500, PixelColor_JammerDischarge, PixelDefaultBrightness);

    delay(400);
    
    TransitionFade(PixelCurrentColor, 5);
}

/////////////////////////
//  Enter/Exit Dark Zone
/////////////////////////
void ToggleDarkZone()
{
    if (IsInDarkZone)
    {
        //  Exit the Dark Zone
        IsInDarkZone = false;
        if (IsRogue)
        {
            ExitRogueEffect();
        }
        PlayAudioClip(ExitingDarkZone);
    }
    else
    {
        //  Enter the Dark Zone
        IsInDarkZone = true;
        PlayAudioClip(EnteringDarkZone);
    }
}

void DarkZoneEffects()
{
    ++CurrentDZClip;
    if (CurrentDZClip >= DZClipsCount)
    {
        CurrentDZClip = 0;
        ShuffleArray(DZClipOrderArray, DZClipsCount);
    }

    if(DZClipOrderArray[CurrentDZClip] == HeloGalExtractionCalled)
    {
      ExtractionEffect(true);
    }
    else
    {
      PlayAudioClip(DZClipOrderArray[CurrentDZClip]);
    }

    
}

void ExtractionEffect(bool CallNewExtraction)
{
    if (CallNewExtraction)
    {
        Serial.println("Calling for new extraction...");
        IsExtractionCalled = true;
        ExtractionLastEvent = millis();
        ExtractionPhase = HeloGalExtractionCalled;
        PlayAudioClip(ExtractionPhase);
    }
    else if (IsExtractionCalled &&
        (millis() - ExtractionLastEvent) >= 15000)
    {
        //  Its been 15 seconds since the last sound clip, play
        //  the next extracton clip
        ExtractionPhase++;
        PlayAudioClip(ExtractionPhase);
        ExtractionLastEvent = millis();
        if (ExtractionPhase >= HeloGalReturningToBase)
        {
            //  This is the end of the extraction clips
            //  extraction is now finished
            IsExtractionCalled = false;
            LastEffectTime = millis();
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
    if (CurrentChatterClip >= ChatterClipsCount)
    {
        CurrentChatterClip = 0;
        ShuffleArray(ChatterClipOrderArray, ChatterClipsCount);
    }

    PlayAudioClip(ChatterClipOrderArray[CurrentChatterClip]);
}

void CycleAudioMode()
{
    //  reset the effect timer for Auto
    LastEffectTime = millis();

    CurrentAudioMode++;
    if (CurrentAudioMode >= AudioModeCount)
    {
        CurrentAudioMode = Manual;
    }

    switch (CurrentAudioMode)
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
    CurrentSkill++;
    if (CurrentSkill >= SkillCount)
    {
        CurrentSkill = 0;
    }
    ShowSkillsHalo();
}

void ShowSkillsHalo()
{
    //  Create the array and reset all the pixels to off
    uint32_t haloUpdate[PixelCount] = {};
    SetHaloArray(haloUpdate, PixelCount, 0x000000);

    //  Pixels at 0, 4, 8, 12 will stay dark to represnet the Axis
    //  to split the halo into 4 sections, one for each skill
    //  
    switch (CurrentSkill)
    {
    case Pulse:
        haloUpdate[1] = PixelColor_Red;
        haloUpdate[2] = PixelColor_Red;
        haloUpdate[3] = PixelColor_Red;
        break;

    case DZScan:
        haloUpdate[5] = PixelColor_DZScan;
        haloUpdate[6] = PixelColor_DZScan;
        haloUpdate[7] = PixelColor_DZScan;
        break;

    case Banshee:
        haloUpdate[9] = PixelColor_SHDCustomDark;
        haloUpdate[10] = PixelColor_SHDCustomDark;
        haloUpdate[11] = PixelColor_SHDCustomDark;
        break;

    case Jammer:
        haloUpdate[13] = PixelColor_JammerCharge;
        haloUpdate[14] = PixelColor_JammerCharge;
        haloUpdate[15] = PixelColor_JammerCharge;
        break;

    default:
        Serial.println("I got a rock!");
    }
  
    UpdatePixelsFromArray(haloUpdate, PixelCount);
    delete haloUpdate;
}

//  Set the pixels of the halo to the colors specified in the passed in array
void UpdatePixelsFromArray(uint32_t HaloArray[], size_t ArraySize, uint8_t Brightness = PixelDefaultBrightness)
{
    Pixels.setBrightness(Brightness);
    for (uint8_t i = 0; i < ArraySize && i < PixelCount; i++)
    {
        Pixels.setPixelColor(i, HaloArray[i]);
    }
    Pixels.show();
}

void SetHaloArray(uint32_t HaloArray[], size_t ArraySize, uint32_t Color )
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
    myDFPlayer.pause();
    ChatterThreshold = CalculateChatThreshold(ChatterPin);
    myDFPlayer.play(file);
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

    ChatterThreshold = maxSample + AudioVolume*2;
    if(AudioVolume < 6)
    {
      ChatterThreshold += 6;
    }
    
    ChatterAverageLastChecked = millis();
    return ChatterThreshold;
}

//  Increases the volume by one step
void VolumeIncrease()
{
    if (AudioVolume >= (AudioVolumeMax - AudioVolumeStep))
    {
        //  We are already at or above the max
        AudioVolume = AudioVolumeMax;
    }
    else
    {
        //  Go in increments of 5 for volume
        AudioVolume += AudioVolumeStep;
    }

    myDFPlayer.volume(AudioVolume);  //Set volume value. From 0 to 30
    PlayAudioClip(Div2SingleTic);
    
    // we are at a new volume level so reset the max chatter effect level
    HighestChatterLevel = 0;
}

//  Decreases the volume level by one step
void VolumeDecrease()
{
    if (AudioVolume <= AudioVolumeStep)
    {
        //  We are already at the low
        AudioVolume = 0;
    }
    else
    {
        //  Go in increments of 5 for volume
        AudioVolume -= AudioVolumeStep;
    }
    
    myDFPlayer.volume(AudioVolume);  //Set volume value. From 0 to 30
    PlayAudioClip(Div2SingleTic);

    // we are at a new volume level so reset the max chatter effect level
    HighestChatterLevel = 0;
}


void SetHaloToVolumeValue()
{
    uint8_t i;
    double volumePercent = (double)AudioVolume / (double)AudioVolumeMax;
    uint8_t ledsToShow = ((double)(PixelCount-1) * volumePercent) + 1;

    Pixels.setPixelColor(0, PixelColor_Green);

    //  Start from the top right and then light 
    //  how many pixels we need along the way.
    //  Once we hit the limit, just leave the rest 
    //  dark.
    for (uint8_t i = PixelCount; i > 0; i--)
    {
        if (ledsToShow > 0)
        {
            Pixels.setPixelColor(i, PixelColor_Green);
            ledsToShow--;
        }
        else
        {
            Pixels.setPixelColor(i, 0);
        }
    }

    Pixels.show();
}

//  Resets all pixels to the specificed color and brightness
//  If called without parameters i.e. ResetPixels() it will reset them
//  to 'off'
void ResetPixels(uint32_t Color = 0, uint8_t Brightness = PixelDefaultBrightness)
{
    Pixels.setBrightness(Brightness);
    for (uint8_t i = 0; i < PixelCount; i++)
    {
        Pixels.setPixelColor(i, Color);
    }
    Pixels.show();
}


//  Does the sparkle effect of flashing a random pixel shortly then turning it off
void SparklePixels(uint32_t EffectLength, uint32_t SparkleColor, uint8_t Brightness, uint32_t BaseColor = 0, uint8_t Delay = 10)
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

//////////////////////////////////
//  Prototype Functions
//////////////////////////////////

//  I started work on these as added effects, but did not
//  finish incorporating them before getting the initial
//  release done.

//  Fades all pixes from their current color to the one specified
//  by increments of 0x01 on each color
void TransitionFade(uint32_t TargetColor, uint16_t DelaySpeed)
{
    uint32_t fadeStart = millis();
    Serial.println("Fade effect started");
    
    uint8_t targetR = TargetColor >> 16;         // 0xRRGGBB -> bit shift 16 bits (8 bytes) -> 0x0000RR
    uint8_t targetG = (TargetColor >> 8) & 0xFF; // 0xRRGGBB -> bit shift 8 bits  (4 bytes) -> 0x00RRGG ->>
                                                 // & with 0xFF strips off all but the first 2 bytes -> 0x0000GG
    uint8_t targetB = TargetColor & 0xFF;        // 0xRRGGBB -> & with 0xFF strips off all but the first 2 bytes -> 0x0000BB

    //  If the brightness is less than 255 (full) then + or - by 0x01
    //  on a color won't neccesarily change the color, so the next time you check
    //  the pixel it will still be the 'old' value.
    //  Therefore, we need to do the increment and checking with our own array
    //  for simplicity of tracking and changing RGB values we create a 2 dimensional
    //  array.  Each pixel will have a row of 3 floats's that will represent R [i][0], 
    //  G [i][1], and B [i][2], [i][3] in the transition array is reserved for how many
    //  steps are needed to get to the target value.  This will be used as a counter
    //  and decremented each step we take till there are 0 left
    float currentPixelColorArray[PixelCount][3] = {};
    float transitionPixelArray[PixelCount][4] = {};
    
    //  Now set the current array to the pixel color values on the array at the moment
    for(uint8_t i = 0; i < PixelCount; i++)
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

      if(deltaR < 0)
      {
        deltaR = deltaR * (-1);
      }
      if(deltaG < 0)
      {
        deltaG = deltaG * (-1);
      }
      if(deltaB < 0)
      {
        deltaB = deltaB * (-1);
      }

      int highestDelta = deltaR;
      if(highestDelta < deltaG)
      {
        highestDelta = deltaG;
      }
      if(highestDelta < deltaB)
      {
        highestDelta = deltaB;
      }
      //  make sure this is not 0
      if(highestDelta == 0)
      {
        highestDelta = 1;
      }
//      Serial.print("highestdelta ");
//      Serial.println(highestDelta);

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

    //  todo: remove
    //  Show the transition matrix
//    for(uint8_t i = 0; i < PixelCount; i++)
//    {
//      Serial.print("Transition pixel ");
//      Serial.print(i);
//      Serial.print(" ");
//      Serial.print(transitionPixelArray[i][0],5);
//      Serial.print(",");
//      Serial.print(transitionPixelArray[i][1],5);
//      Serial.print(",");
//      Serial.print(transitionPixelArray[i][2],5);
//      Serial.println("");
//    }

    //  Create the tracking array
    bool changesMade = true;
    while (changesMade)
    {
        changesMade = false;    // set it to false and it will get set to true if
                                // we had to make any changes
        for (uint8_t i = 0; i < PixelCount; i++)
        {
            //  Check if we have done enough transition steps on this pixel
            //  from our previous calculations.  If not, then do the changes
            if ( transitionPixelArray[i][3] > 0 )
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
    Serial.print("Fade effect done in ");
    Serial.println(millis() - fadeStart);
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
