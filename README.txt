COE718 MCB1700 MEDIA CENTER
===========================

TARGET
------
Keil uVision / ARM Compiler 6
MCB1700 board, NXP LPC1768
CMSIS-RTOS2 (RTX5)

FEATURES IN THIS STARTER IMPLEMENTATION
---------------------------------------
1. Joystick-driven main menu and module navigation (FSM).
2. Photo gallery navigation shell and edge-filter toggle.
3. Snake game: joystick direction, random food, growth, score, wall/self
   collision, game-over and restart.
4. Memory game: 4x3 board, 12 cards / 6 pairs, Fisher-Yates shuffle every
   entry, attempts counter, mismatch delay, matched-pair retention, victory.
5. Music player user interface: track selection, play/pause and volume
   control UI with explicit integration hooks for the USB Audio example.

FILES
-----
main.c             RTOS2 startup and application thread
media_center.c/h   Main FSM and menu
gallery.c/h        Photo gallery screen/navigation
audio_player.c/h   Music player UI and USB-audio integration hooks
games.c/h          Snake and memory matching games

ADD TO YOUR EXISTING UVISION PROJECT
------------------------------------
1. Make a backup of Final_project.uvprojx.
2. Use the Lab5_Part2 project as the starting project because it uses
   CMSIS-RTOS2 (cmsis_os2.h / osThreadNew).
3. Add the six .c files to the project and add this folder to Include Paths.
4. Keep the LPC1768 startup file, system_LPC17xx.c, RTX configuration,
   device pack, and board support files from the existing project.
5. Add/enable the MCB1700 GLCD and Joystick board support components.
   The code expects GLCD.h and Board_joystick.h, and the API names used in
   the supplied Part1 source: Joystick_Initialize(), Joystick_GetState(),
   JOYSTICK_UP/DOWN/LEFT/RIGHT/CENTER.
6. Remove the old Lab5_Part2 main.c from the build when adding this main.c.
   Do not compile multiple files that define main().
7. Build, then resolve any board-support include/API differences against
   the exact Keil pack installed in your lab.

IMPORTANT HARDWARE INTEGRATION NOTES
------------------------------------
A. USB MP3 streaming is NOT complete in this zip. The archive only includes
   a joinable-thread demonstration in Lab5_Part2 and RTOS priority-inversion
   demos in Lab5_Part1. It does not include the course USBAudio class stack.
   Copy the complete course examples/USBAudio source, descriptors, USB
   configuration and interrupt handlers into the project, then connect the
   AudioUSB_* hooks in audio_player.c. The MCB1700 must enumerate as a USB
   audio device and receive isochronous audio data from the PC.
B. Potentiometer-to-volume requires ADC sampling and mapping. Connect the
   board's potentiometer ADC channel to AudioUSB_SetVolume() (or the audio
   class volume control). The current volume percentage is a UI placeholder.
C. Photo bitmaps must be converted to RGB565 C arrays using the method in
   Media-Center.pdf. Add the generated source/header and call GLCD_Bitmap()
   with the exported width/height/data array. The gallery currently shows
   navigation and filter UI placeholders; no image assets were in the zip.
D. The edge-detection toggle is a UI hook only. Add a pixel-buffer processing
   routine (e.g., grayscale/edge magnitude) and apply it to RGB565 image data
   before drawing.
E. Joystick LEFT is reserved as the global return-to-menu action outside the
   menu. In Snake, LEFT is also a movement direction, so use joystick CENTER
   or change the back-button policy if you need all four directions during
   play. Recommended fix: add a dedicated exit gesture (hold CENTER) or use
   joystick select mapping; current global LEFT takes precedence.

MEMORY GAME RANDOMIZATION
-------------------------
The card values are initialized as pairs [0,0,1,1,...,5,5] then shuffled
using Fisher-Yates. The seed is mixed with the RTOS tick count on each entry.
For a production-grade random seed, mix in ADC noise or another entropy
source.

C COMPATIBILITY
---------------
Declarations are placed at the start of blocks where practical. The code
uses standard C and CMSIS-RTOS2. If your project is configured for an older
CMSIS-RTOS v1 RTX library, do not mix cmsis_os.h/osThreadCreate with this
CMSIS-RTOS2 main.c; switch the project to RTX5/CMSIS-RTOS2 or port the
application consistently to v1.
