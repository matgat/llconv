#ifndef GUARD_debug_hpp
#define GUARD_debug_hpp
/*  ---------------------------------------------
    ©2022 matteo.gattanini@gmail.com

    OVERVIEW
    ---------------------------------------------
    Debug facilities

    DEPENDENCIES:
    --------------------------------------------- */
//#include <format> // c++20 formatting library
#include <fmt/core.h> // fmt::format
#include <fmt/color.h>
//#include <iostream>
#include <cassert> // assert


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace dbg
{

  #ifdef NDEBUG
    #define DBGLOG(...)
  #else
    // Occhio, fmt::print lancia std::runtime_error("invalid utf8") quindi meglio assicurarci di convertire
    #define DBGLOG(s,...) fmt::print(fg(fmt::color::aquamarine) | fmt::emphasis::italic, s, __VA_ARGS__);
  #endif

  #ifdef MS_WINDOWS
    #ifdef NDEBUG
      #define EVTLOG(...)
      //#define EVTLOG(s, ...) ::OutputDebugString(fmt::format(s,__VA_ARGS__).c_str());
    #else
      #define EVTLOG(s, ...) ::OutputDebugString(fmt::format(s,__VA_ARGS__).c_str());
    #endif
  #endif

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



//---- end unit -------------------------------------------------------------
#endif
