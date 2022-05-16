//
//  Equalizer.hpp
//  Equalizer
//
//  Created by Dale on 5/9/22.
//

#ifndef Equalizer_hpp
#define Equalizer_hpp

#include <stdio.h>
#include <string.h>
#include <iostream>

#include "Poly.h"

#ifdef GLEW_IN
#include <GL/glew.h>
#endif

#if __has_include(<rtaudio/RtAudio.h>)
#include <rtaudio/RtAudio.h>
#else
#include <RtAudio.h>
#endif

// include OpenGL
#ifdef __WXMAC__
#include "OpenGL/glu.h"
#include "OpenGL/gl.h"
#else
#include <GL/glu.h>
#include <GL/gl.h>
#endif

#include <wx/wx.h>
#include <wx/sizer.h>
#include <wx/glcanvas.h>

#include <liquid/liquid.h>
#include <sndfile.h>
#include <stdlib.h>

 enum {
    ID_INSERT = 10004,
    ID_PLAY,
    ID_PAUSE,
    ID_DELETE,
    ID_RENDER,
    TEXT_CLIPBOARD_COPY,
    TEXT_CLIPBOARD_PASTE,
    SCROLL_01,
    SCROLL_02,
    SCROLL_03,
    SCROLL_04,
    SCROLL_05,
    SCROLL_06,
    SCROLL_07,
    SCROLL_08,
    SCROLL_09,
    SCROLL_10,
    SCROLL_TIME,
    SCROLL_GAIN,
    PLAYBACK_MENU,
    AUDIO_MENU =20000,
    OPEN_RECENT=30000,
};


#endif /* Equalizer_hpp */
