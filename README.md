# First-Wave-ISAC-Beacon
The First Wave ISAC Beacon is a recreation of the shoulder mounted radio worn by players in Tom Clancy's The Division video games. This version is modified from the 3D model work of 2 other authors (attributes in the guides).  It has fully functioning audio and visual effects from The Division 2. This version of the ISAC beacon also has multiple modes and features accessed by the buttons on the unit including:

- Startup boot sequence effect and audio
- 4 SHD skills from in-game with sound and visual effects
- World notifications as heard in-game that can be played manually, or automatically
- Dark Zone Mode that changes world notifications to ones specifically heard in the Dark Zone
- Rogue protocol - go rogue and turn the ISAC light ring RED while in the Dark Zone
- Volume control (especially to increase volume for cosplay at conventions)
- ISAC light ring 'chatter' effect when ISAC is talking

See the User Guide for information on how to access these features.

Here is a video showcasing the effects, however the camera doesn’t show the light ring effects as well as I had hoped (especially the light ring turning red during Rogue mode). 
https://vimeo.com/362583784

To achieve the effects of the First Wave ISAC Beacon, I designed and created a custom circuit board that simplifies assembly and soldering.  Also developed custom Arduino software to run the device and effects.  

See the Assembly Guide for a full list of parts and components as well as additional instructions and advice.

Latest updates:
2023-07-17 - Was working on building a few of these and found that the latest libraries and Arduino IDE compiler have improved performance.  This lead to timing issues with my older code which I believe I have fixed.  I also refactored a lot of the code including button state handling to be more precise and avoid the code treating a single click as multiple clicks.
             Known issue with current build: Rarely when you either enter or exit Rogue status the program will stop responding to further button clicks.  If this happens simply reboot the device, or add a but longer delay() at the end of the enter and exit Rogue functions.
			 

