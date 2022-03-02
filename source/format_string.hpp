#ifndef GUARD_format_string_hpp
#define GUARD_format_string_hpp
/*  ---------------------------------------------
    ©2021 matteo.gattanini@gmail.com

    OVERVIEW
    ---------------------------------------------
    Logging facilities

    DEPENDENCIES:
    --------------------------------------------- */
//#include <format> // c++20 formatting library
#include <fmt/core.h> // fmt::format
#include <fmt/color.h>
#include <string>
#include <string_view>
//#include <iostream>


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace fmtstr
{
    
//---------------------------------------------------------------------------
//template<typename ...Args> void print(Args&&... args)
//   {
//    fmt::print(fg(fmt::color::aquamarine) | fmt::emphasis::italic, std::forward<Args>(args)...);
//   }


//---------------------------------------------------------------------------
//template<typename ...Args> std::string format(Args&&... args)
//   {
//    return fmt::format(std::forward<Args>(args)...);
//   }


/////////////////////////////////////////////////////////////////////////////
class error : public std::exception
{
 public:
    template<class ... Args> explicit error(const std::string_view txt, Args&&... args)
       : i_msg(fmt::format(fmt::runtime(txt), std::forward<Args>(args)...)) {}

    const char* what() const noexcept override { return i_msg.c_str(); } // Will probably rise a '-Wweak-vtables'

 private:
    const std::string i_msg;
};


/////////////////////////////////////////////////////////////////////////////
class parse_error : public std::exception
{
 public:
    explicit parse_error(const std::string_view txt, const std::size_t l, const std::size_t i) noexcept
       : i_msg(fmt::format("{} (line {}, offset {})",txt,l,i)), i_line(l), i_pos(i) {}

    const char* what() const noexcept override { return i_msg.c_str(); } // Will probably rise a '-Wweak-vtables'

    std::size_t line() const noexcept { return i_line; }
    std::size_t pos() const noexcept { return i_pos; }

 private:
    const std::string i_msg;
    const std::size_t i_line, i_pos;
};


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



//---- end unit -------------------------------------------------------------
#endif
