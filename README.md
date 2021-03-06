# wxEqualizer

   wxEqualizer is a 10 channel audio equalizer created using wxWdigets. wxEqualizer has controls that permits the power of each channel to be adjusted +- 12 db. Gauges show the power of each channel. Gauges ,also, permit the time of playback and the overall volume level to be set. As files are opened they are saved in the "Opened Recent" menu. The "Opened Recent" is automatically save and restore as the program is started and stopped.  The file to open can be selected from the "Opened Recent" menu and turning on the "PlayBack->Options->Loop Files" flag tells the program to loop on playing the files in the "Opened Recent" list. It has makefiles for MacOS, Ubuntu and Raspbian (Raspberry Pi). RtAudio, LiquidSRD, wxWidigets, and Libsndfile are the required libaries.

  wxEqualizer was written to test the wxWidgets library to see how difficult it was to use and to try to make a usable audio equalizer. There was a problem with the Gauge widget - it would only display correctly inside a size widget. This unleashed a long list of problems with trying to get the size widgets to work - wxFlexGridSizer could not handle the number of lines needed - wxBoxSizer does not position the widgets correctly - etc.. The final layout was about what was wanted, but it required hours of fooling around to get it. On the other hand, the wxFileConfig routines were amazing - they made it simple to save and restore information as the application was started and stopped. The equalizer channels were tested and verified to be correct using the tone test routines. Most equalizer programs have presets for various types of music and that would be easy to add - just tweeking the controls a little can really improve the sound.
  
  Everything seems to be working and working well, but it needs some custom widgets for it to look pretty.
  
  If anyone wants to improve the wxEqualizer - go for it, fork the program - if you have any questions - just leave them in the "issues" list.

## Demo video

- [wxEqualizer a 10 channel audio equalizer using wxWidgets.](https://youtu.be/wcsD3ahMplg)

