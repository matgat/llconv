#ifndef GUARD_basic_parser_hpp
#define GUARD_basic_parser_hpp
/*  ---------------------------------------------
    ©2022 matteo.gattanini@gmail.com

    OVERVIEW
    ---------------------------------------------
    Common parsing facilities

    CONSTRAINTS
    ---------------------------------------------
    UTF-8 files (yeah, drop other encodings)
    Unix line end '\n' (are you using a typewriter?)

    DEPENDENCIES:
    --------------------------------------------- */
#include <cctype> // std::isdigit, std::isblank, ...
#include <string_view>
#include <stdexcept> // std::exception, std::runtime_error, ...
#include <charconv> // std::from_chars
//#include <algorithm> // std::min
#include <fmt/core.h> // fmt::format

#include "string-utilities.hpp" // str::escape
#include "debug.hpp" // DBGLOG

using namespace std::literals; // "..."sv



/////////////////////////////////////////////////////////////////////////////
class parse_error final : public std::exception
{
 public:
    explicit parse_error(const std::string_view msg,
                         const std::string_view pth,
                         const std::size_t lin,
                         const std::size_t off) noexcept
       : i_msg(fmt::format("{} ({}:{})",msg,pth,lin))
       , i_filepath(pth)
       , i_line(lin)
       , i_pos(off) {}

    const char* what() const noexcept override { return i_msg.c_str(); } // Could rise a '-Wweak-vtables'

    const std::string& file_path() const noexcept { return i_filepath; }
    std::size_t line() const noexcept { return i_line; }
    std::size_t pos() const noexcept { return i_pos; }

 private:
    std::string i_msg;
    std::string i_filepath;
    std::size_t i_line, // Line
                i_pos; // Character offset
};



/////////////////////////////////////////////////////////////////////////////
class BasicParser
{
 protected:
    const std::string file_path;
    const char* const buf;
    const std::size_t siz; // buffer size
    const std::size_t i_last; // index of the last character
    std::size_t line; // Current line number
    std::size_t i; // Current character
    std::vector<std::string>& issues; // Problems found
    const bool fussy;

 public:
    BasicParser(const std::string& pth,
                const std::string_view dat,
                std::vector<std::string>& lst,
                const bool fus)
      : file_path(pth)
      , buf(dat.data())
      , siz(dat.size())
      , i_last(siz-1u) // siz>0
      , line(1)
      , i(0)
      , issues(lst)
      , fussy(fus)
       {
        // Check possible BOM    |  Encoding    |   Bytes     | Chars |
        //                       |--------------|-------------|-------|
        //                       | UTF-8        | EF BB BF    | ï»¿   |
        //                       | UTF-16 (BE)  | FE FF       | þÿ    |
        //                       | UTF-16 (LE)  | FF FE       | ÿþ    |
        //                       | UTF-32 (BE)  | 00 00 FE FF | ..þÿ  |
        //                       | UTF-32 (LE)  | FF FE 00 00 | ÿþ..  |
        if( siz < 1 )
           {
            throw std::runtime_error("Empty file");
           }
        // I'll accept just UTF-8 or any other 8-bit encodings
        if( buf[0]=='\xFF' || buf[0]=='\xFE' || buf[0]=='\x00' )
           {
            throw std::runtime_error("Bad encoding, not UTF-8");
           }

        // Doesn't play well with windows EOL "\r\n", since uses string_view extensively, cannot eat '\r'
        //if(buf.find('\r') != buf.npos) throw std::runtime_error("Use unix EOL, remove CR (\\r) characters");
       }

    BasicParser(const BasicParser&) = delete; // Prevent copy
    BasicParser(BasicParser&&) = delete; // Prevent move
    BasicParser& operator=(const BasicParser&) = delete; // Prevent copy assignment
    BasicParser& operator=(BasicParser&&) = delete; // Prevent move assignment
    ~BasicParser() = default; // Yes, destructor is not virtual


    //-----------------------------------------------------------------------
    [[nodiscard]] bool end_not_reached() const noexcept { return i<siz; }
    [[nodiscard]] std::size_t curr_line() const noexcept { return line; }
    [[nodiscard]] std::size_t curr_pos() const noexcept { return i; }


    //-----------------------------------------------------------------------
    parse_error create_parse_error(const std::string_view msg) const noexcept
       {
        return parse_error(msg, file_path, line, i<=i_last ? i : i_last);
       }

    //-----------------------------------------------------------------------
    parse_error create_parse_error(const std::string_view msg, const std::size_t lin, const std::size_t off) const noexcept
       {
        return parse_error(msg, file_path, lin, off<=i_last ? off : i_last);
       }

    //-----------------------------------------------------------------------
    //template<typename ...Args> void notify_error(const std::string_view msg, Args&&... args) const
    //   {
    //    // Unfortunately this generates error: "call to immediate function is not a constant expression"
    //    //if(fussy) throw create_parse_error( fmt::format(msg, args...) );
    //    //else issues.push_back( fmt::format("{} (line {}, offset {})"sv, fmt::format(msg, args...), line, i) );
    //    if(fussy) throw create_parse_error( fmt::format(fmt::runtime(msg), args...) );
    //    else issues.push_back( fmt::format("{} (line {}, offset {})", fmt::format(fmt::runtime(msg), args...), line, i) );
    //   }
    // consteval friendly:
    #define notify_error(...) \
       {\
        if(fussy) throw create_parse_error( fmt::format(__VA_ARGS__) );\
        else issues.push_back( fmt::format("{} (line {}, offset {})"sv, fmt::format(__VA_ARGS__), line, i) );\
       }


 protected:
    //-----------------------------------------------------------------------
    [[nodiscard]] static bool is_blank(const char c) noexcept
       {
        return std::isspace(c) && c!='\n';
       }


    //-----------------------------------------------------------------------
    // Skip space chars except new line
    void skip_blanks() noexcept
       {
        while( i<siz && is_blank(buf[i]) ) ++i;
       }


    //-----------------------------------------------------------------------
    [[maybe_unused]] bool eat_line_end() noexcept
       {
        assert(i<siz);
        if( buf[i]=='\n' )
           {
            ++i;
            ++line;
            return true;
           }
        return false;
       }


    //-----------------------------------------------------------------------
    // Skip empty lines
    void skip_empty_lines() noexcept
       {
        do{ skip_blanks(); }
        while( eat_line_end() );
       }


    //-----------------------------------------------------------------------
    void check_if_line_ended_after(const std::string_view fmt_str, const std::string_view fmt_arg)
       {
        skip_blanks();
        if( !eat_line_end() )
           {
            notify_error("Unexpected content after {}: {}", fmt::format(fmt::runtime(fmt_str), fmt_arg), str::escape(skip_line()));
           }
       }


    //-----------------------------------------------------------------------
    [[maybe_unused]] std::string_view skip_line() noexcept
       {
        // Intercept the edge case already at buffer end:
        if(i>i_last) return std::string_view(buf+i_last, 0);

        const std::size_t i_start = i;
        while( i<siz && !eat_line_end() ) ++i;
        return std::string_view(buf+i_start, i-i_start);
        // Note: If '\n' not found is i==siz and returns what remains in buf
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] bool eat(const std::string_view s) noexcept
       {
        const std::size_t i_end = i+s.length();
        if( i_end<=siz && s==std::string_view(buf+i,s.length()) )
           {
            i = i_end;
            return true;
           }
        return false;
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] bool eat_token(const std::string_view s) noexcept
       {
        const std::size_t i_end = i+s.length();
        if( ((i_end<siz && !std::isalnum(buf[i_end])) || i_end==siz) && s==std::string_view(buf+i,s.length()) )
           {
            i = i_end;
            return true;
           }
        return false;
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] std::string_view collect_token() noexcept
       {
        const std::size_t i_start = i;
        while( i<siz && !std::isspace(buf[i]) ) ++i;
        return std::string_view(buf+i_start, i-i_start);
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] std::string_view collect_identifier() noexcept
       {
        const std::size_t i_start = i;
        while( i<siz && (std::isalnum(buf[i]) || buf[i]=='_') ) ++i;
        return std::string_view(buf+i_start, i-i_start);
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] std::string_view collect_numeric_value() noexcept
       {
        const std::size_t i_start = i;
        while( i<siz && (std::isdigit(buf[i]) || buf[i]=='+' || buf[i]=='-' || buf[i]=='.' || buf[i]=='E') ) ++i;
        return std::string_view(buf+i_start, i-i_start);
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] std::string_view collect_digits() noexcept
       {
        const std::size_t i_start = i;
        while( i<siz && std::isdigit(buf[i]) ) ++i;
        return std::string_view(buf+i_start, i-i_start);
       }

    //-----------------------------------------------------------------------
    // Read a (base10) positive integer literal
    [[nodiscard]] std::size_t extract_index()
       {
        if( i>=siz )
           {
            throw create_parse_error("Index not found");
           }

        if( buf[i]=='+' )
           {
            ++i;
            if( i>=siz )
               {
                throw create_parse_error("Invalid index \'+\'");
               }
           }
        else if( buf[i]=='-' )
           {
            throw create_parse_error("Negative index");
           }
        if( !std::isdigit(buf[i]) )
           {
            throw create_parse_error(fmt::format("Invalid char \'{}\' in index", buf[i]));
           }
        std::size_t result = (buf[i]-'0');
        const std::size_t base = 10u;
        while( ++i<siz && std::isdigit(buf[i]) )
           {
            result = (base*result) + (buf[i]-'0');
           }
        return result;
       }


    //-----------------------------------------------------------------------
    // Read a (base10) integer literal
    [[nodiscard]] int extract_integer()
       {
        if( i>=siz )
           {
            throw create_parse_error("No integer found");
           }
        int sign = 1;
        if( buf[i]=='+' )
           {
            //sign = 1;
            ++i;
            if( i>=siz )
               {
                throw create_parse_error("Invalid integer \'+\'");
               }
           }
        else if( buf[i]=='-' )
           {
            sign = -1;
            ++i;
            if( i>=siz )
               {
                throw create_parse_error("Invalid integer \'-\'");
               }
           }
        if( !std::isdigit(buf[i]) )
           {
            throw create_parse_error(fmt::format("Invalid char \'{}\' in integer", buf[i]));
           }
        int result = (buf[i]-'0');
        const int base = 10;
        while( ++i<siz && std::isdigit(buf[i]) )
           {
            result = (base*result) + (buf[i]-'0');
           }
        return sign * result;
       }


    //-----------------------------------------------------------------------
    // Read a (base10) floating point number literal
    //[[nodiscard]] double extract_double()
    //   {
    //    // [sign]
    //    int sgn = 1;
    //    if( buf[i]=='-' ) {sgn = -1; ++i;}
    //    else if( buf[i]=='+' ) ++i;
    //
    //    // [mantissa - integer part]
    //    double mantissa = 0;
    //    bool found_mantissa = false;
    //    if( std::isdigit(buf[i]) )
    //       {
    //        found_mantissa = true;
    //        do {
    //            mantissa = (10.0 * mantissa) + static_cast<double>(buf[i] - '0');
    //            //if( buf[++i] == '\'' ); // Skip thousand separator char
    //           }
    //        while( std::isdigit(buf[i]) );
    //       }
    //    // [mantissa - fractional part]
    //    if( buf[i] == '.' )
    //       {
    //        ++i;
    //        double k = 0.1; // shift of decimal part
    //        if( std::isdigit(buf[i]) )
    //           {
    //            found_mantissa = true;
    //            do {
    //                mantissa += k * static_cast<double>(buf[i] - '0');
    //                k *= 0.1;
    //                ++i;
    //               }
    //            while( std::isdigit(buf[i]) );
    //           }
    //       }
    //
    //    // [exponent]
    //    int exp=0, exp_sgn=1;
    //    bool found_expchar = false,
    //         found_expval = false;
    //    if( buf[i] == 'E' || buf[i] == 'e' )
    //       {
    //        found_expchar = true;
    //        ++i;
    //        // [exponent sign]
    //        if( buf[i] == '-' ) {exp_sgn = -1; ++i;}
    //        else if( buf[i] == '+' ) ++i;
    //        // [exponent value]
    //        if( std::isdigit(buf[i]) )
    //           {
    //            found_expval = true;
    //            do {
    //                exp = (10 * exp) + static_cast<int>(buf[i] - '0');
    //                ++i;
    //               }
    //            while( std::isdigit(buf[i]) );
    //           }
    //       }
    //    if( found_expchar && !found_expval )
    //         {
    //          throw create_parse_error("Invalid floating point number: No exponent value"); // ex "123E"
    //         }
    //
    //    // [calculate result]
    //    double result = 0.0;
    //    if( found_mantissa )
    //      {
    //       result = sgn * mantissa * std::pow(10.0, exp_sgn*exp);
    //      }
    //    else if( found_expval )
    //       {
    //        result = sgn * std::pow(10.0,exp_sgn*exp); // ex "E100"
    //       }
    //    else
    //       {
    //        throw create_parse_error("Invalid floating point number");
    //        //if(found_expchar) result = 1.0; // things like 'E,+E,-exp,exp+,E-,...'
    //        //else result = std::numeric_limits<double>::quiet_NaN(); // things like '+,-,,...'
    //       }
    //    return result;
    //   }


    //-----------------------------------------------------------------------
    //[[nodiscard]] std::string_view collect_until_char(const char c)
    //   {
    //    const std::size_t i_start = i;
    //    while( i<siz )
    //       {
    //        if( buf[i]==c )
    //           {
    //            //DBGLOG("    [*] Collected \"{}\" at line {}\n", std::string_view(buf+i_start, i-i_start), line)
    //            return std::string_view(buf+i_start, i-i_start);
    //           }
    //        else if( buf[i]=='\n' ) ++line;
    //        ++i;
    //       }
    //    throw create_parse_error(fmt::format("Unclosed content (\'{}\' expected)", str::escape(c)));
    //   }


    //-----------------------------------------------------------------------
    //[[nodiscard]] std::string_view collect_until_char_same_line(const char c)
    //   {
    //    const std::size_t i_start = i;
    //    while( i<siz )
    //       {
    //        if( buf[i]==c )
    //           {
    //            //DBGLOG("    [*] Collected \"{}\" at line {}\n", std::string_view(buf+i_start, i-i_start), line)
    //            return std::string_view(buf+i_start, i-i_start);
    //           }
    //        else if( buf[i]=='\n' ) break;
    //        ++i;
    //       }
    //    throw create_parse_error(fmt::format("Unclosed content (\'{}\' expected)", str::escape(c)));
    //   }


    //-----------------------------------------------------------------------
    [[nodiscard]] std::string_view collect_until_char_trimmed(const char c)
       {
        const std::size_t line_start = line; // Store current line
        const std::size_t i_start = i;       // and position
        std::size_t i_end = i_start; // Index past last char not blank
        while( i<siz )
           {
            if( buf[i]==c )
               {
                //++i; // Nah, do not eat c
                return std::string_view(buf+i_start, i_end-i_start);
               }
            else if( buf[i]=='\n' )
               {
                ++line;
                ++i;
               }
            else
               {
                if( !is_blank(buf[i]) ) i_end = ++i;
                else ++i;
               }
           }
        throw create_parse_error(fmt::format("Unclosed content (\'{}\' expected)", str::escape(c)), line_start, i_start);
       }


    //-----------------------------------------------------------------------
    //[[nodiscard]] std::string_view collect_until_token(const std::string_view tok)
    //   {
    //    const std::size_t i_start = i;
    //    const std::size_t max_siz = siz-tok.length();
    //    while( i<max_siz )
    //       {
    //        if( buf[i]==tok[0] && eat_token(tok) )
    //           {
    //            return std::string_view(buf+i_start, i-i_start-tok.length());
    //           }
    //        else if( buf[i]=='\n' ) ++line;
    //        ++i;
    //       }
    //    throw create_parse_error(fmt::format("Unclosed content (\"{}\" expected)",tok), line_start, i_start);
    //   }


    //-----------------------------------------------------------------------
    [[nodiscard]] std::string_view collect_until_newline_token(const std::string_view tok)
       {
        const std::size_t line_start = line;
        const std::size_t i_start = i;
        while( i<siz )
           {
            if( buf[i]=='\n' )
               {
                ++i;
                ++line;
                skip_blanks();
                if( eat_token(tok) )
                   {
                    return std::string_view(buf+i_start, i-i_start-tok.length());
                   }
               }
            else ++i;
           }
        throw create_parse_error(fmt::format("Unclosed content (\"{}\" expected)",tok), line_start, i_start);
       }


    //-----------------------------------------------------------------------
    //[[nodiscard]] bool eat_character(const char c) noexcept
    //   {
    //    if( i<siz && buf[i]==c )
    //       {
    //        ++i;
    //        return true;
    //       }
    //    return false;
    //   }


    //-----------------------------------------------------------------------
    //[[nodiscard]] bool eat_characters(const char c1, const char c2) noexcept
    //   {
    //    if( i<(siz-1) && buf[i]==c1 && buf[i+1]==c2 )
    //       {
    //        i += 2; // Skip them
    //        return true;
    //       }
    //    return false;
    //   }


    //-----------------------------------------------------------------------
    //void skip_until_characters(const char c1, const char c2)
    //   {
    //    const std::size_t line_start = line; // Store current line
    //    const std::size_t i_start = i; // Store current position
    //    while( i<i_last )
    //       {
    //        if( buf[i]==c1 && buf[i+1]==c2 )
    //           {
    //            i += 2; // Skip them
    //            return;
    //           }
    //        else if( buf[i]=='\n' )
    //           {
    //            ++line;
    //           }
    //        ++i;
    //       }
    //    throw create_parse_error(fmt::format("Unclosed block (\"{}{}\" expected)",c1,c2), line_start, i_start);
    //   }
};

//#undef notify_error


//---- end unit -------------------------------------------------------------
#endif
