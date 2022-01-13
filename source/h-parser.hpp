#ifndef h_parser_hpp
#define h_parser_hpp
/*  ---------------------------------------------
    ©2022 matteo.gattanini@gmail.com

    OVERVIEW
    ---------------------------------------------
    Sipro 'h' file format
    (a list of c preprocessor defines)

    DEPENDENCIES:
    --------------------------------------------- */
#include <cctype> // std::isdigit, std::isblank, ...
#include <string_view>
#include <stdexcept> // std::exception, std::runtime_error, ...
#include <fmt/core.h> // fmt::format

#include "string-utilities.hpp" // str::escape
//#include "plc-elements.hpp" // plcb::*
#include "logging.hpp" // dlg::parse_error, dlg::error, DBGLOG

using namespace std::literals; // Use "..."sv


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace h //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{


/////////////////////////////////////////////////////////////////////////////
// Descriptor of a '#define' entry in the file
class RawDefine /////////////////////////////////////////////////////////////
{
 public:

    std::string_view label() const noexcept { return i_Label; }
    void set_label(const std::string_view s)
       {
        if(s.empty()) throw dlg::error("Empty define label");
        i_Label = s;
       }

    std::string_view value() const noexcept { return i_Value; }
    void set_value(const std::string_view s)
       {
        if(s.empty()) throw dlg::error("Empty define value");
        i_Value = s;
       }

    std::string_view comment() const noexcept { return i_Comment; }
    void set_comment(const std::string_view s) noexcept { i_Comment = s; }
    bool has_comment() const noexcept { return !i_Comment.empty(); }

 private:
    std::string_view i_Label;
    std::string_view i_Value;
    std::string_view i_Comment;
};


//---------------------------------------------------------------------------
// Parse h file
void parse(const std::string_view buf, plc::Library& lib, std::vector<std::string>& issues, const bool fussy =true)
{
    // Check possible BOM    |  Encoding    |   Bytes     | Chars |
    //                       |--------------|-------------|-------|
    //                       | UTF-8        | EF BB BF    | ï»¿   |
    //                       | UTF-16 (BE)  | FE FF       | þÿ    |
    //                       | UTF-16 (LE)  | FF FE       | ÿþ    |
    //                       | UTF-32 (BE)  | 00 00 FE FF | ..þÿ  |
    //                       | UTF-32 (LE)  | FF FE 00 00 | ÿþ..  |
    const std::size_t siz = buf.size();
    if( siz < 2 )
       {
        if(fussy)
           {
            throw std::runtime_error("Empty file");
           }
        else
           {
            issues.emplace_back("Empty file");
            return;
           }
       }
    // Mi accontento di intercettare UTF-16
    if( buf[0]=='\xFF' || buf[0]=='\xFE' ) throw std::runtime_error("Bad encoding, not UTF-8");

    std::size_t line = 1; // Current line number
    std::size_t i = 0; // Current character

    //---------------------------------
    //auto notify_error = [&](const std::string_view msg, auto... args)
    //   {
    //    if(fussy) throw std::runtime_error( fmt::format(fmt::runtime(msg), args...) );
    //    else issues.push_back( fmt::format("{} (line {}, offset {})", fmt::format(fmt::runtime(msg), args...), line, i) );
    //   };
    // Ehmm, with a lambda I cannot use consteval format
    #define notify_error(...) \
       {\
        if(fussy) throw std::runtime_error( fmt::format(__VA_ARGS__) );\
        else issues.push_back( fmt::format("{} (line {}, offset {})"sv, fmt::format(__VA_ARGS__), line, i) );\
       }

    //---------------------------------
    auto is_blank = [](const char c) noexcept -> bool
       {
        return std::isspace(c) && c!='\n';
       };

    //---------------------------------
    auto skip_blanks = [&]() noexcept -> void
       {
        while( i<siz && is_blank(buf[i]) ) ++i;
       };

    //---------------------------------
    auto last_not_blank = [&](std::size_t i_last) noexcept -> std::size_t
       {
        while( i_last>0 && is_blank(buf[i_last-1]) ) --i_last;
        return i_last;
       };

    //---------------------------------
    auto eat_line_end = [&]() noexcept -> bool
       {
        if( buf[i]=='\n' )
           {
            ++i;
            ++line;
            return true;
           }
        return false;
       };

    //---------------------------------
    auto skip_line = [&]() noexcept -> std::string_view
       {
        const std::size_t i_start = i;
        while( i<siz && !eat_line_end() ) ++i;
        return std::string_view(buf.data()+i_start, i-i_start);
       };

    //---------------------------------
    auto eat_line_comment_start = [&]() noexcept -> bool
       {
        if( i<(siz-1) && buf[i]=='/' && buf[i+1]=='/' )
           {
            i += 2; // Skip "//"
            return true;
           }
        return false;
       };

    //---------------------------------
    //auto eat_block_comment_start = [&]() noexcept -> bool
    //   {
    //    if( i<(siz-1) && buf[i]=='/' && buf[i+1]=='*' )
    //       {
    //        i += 2; // Skip "/*"
    //        return true;
    //       }
    //    return false;
    //   };

    //---------------------------------
    //auto skip_block_comment = [&]() -> void
    //   {
    //    const std::size_t line_start = line; // Store current line
    //    const std::size_t i_start = i; // Store current position
    //    const std::size_t sizm1 = siz-1;
    //    while( i<sizm1 )
    //       {
    //        if( buf[i]=='*' && buf[i+1]=='/' )
    //           {
    //            i += 2; // Skip "*/"
    //            return;
    //           }
    //        else if( buf[i]=='\n' )
    //           {
    //            ++line;
    //           }
    //        ++i;
    //       }
    //    throw dlg::parse_error("Unclosed block comment", line_start, i_start);
    //   };

    //---------------------------------
    auto eat_token = [&](const std::string_view s) noexcept -> bool
       {
        const std::size_t i_end = i+s.length();
        if( ((i_end<siz && !std::isalnum(buf[i_end])) || i_end==siz) && s==std::string_view(buf.data()+i,s.length()) )
           {
            i = i_end;
            return true;
           }
        return false;
       };

    //---------------------------------
    auto collect_token = [&]() noexcept -> std::string_view
       {
        const std::size_t i_start = i;
        while( i<siz && !std::isspace(buf[i]) ) ++i;
        return std::string_view(buf.data()+i_start, i-i_start);
       };

    //---------------------------------
    auto collect_identifier = [&]() noexcept -> std::string_view
       {
        const std::size_t i_start = i;
        while( i<siz && (std::isalnum(buf[i]) || buf[i]=='_') ) ++i;
        return std::string_view(buf.data()+i_start, i-i_start);
       };

    //---------------------------------
    auto collect_until_char_trimmed = [&](const char c) -> std::string_view
       {
        const std::size_t i_start = i;
        while( i<siz )
           {
            if( buf[i]==c ) break;
            else if( buf[i]=='\n' ) ++line;
            ++i;
           }
        const std::size_t i_end = last_not_blank(i);
        return std::string_view(buf.data()+i_start, i_end-i_start);
       };

    //---------------------------------
    //auto eat_directive_start = [&]() noexcept -> bool
    //   {//#define
    //    if( i<siz && buf[i]=='#' )
    //       {
    //        ++i;
    //        return true;
    //       }
    //    return false;
    //   };

    //---------------------------------
    auto collect_define = [&]() -> RawDefine
       {// LABEL       0  // [INT] Descr
        // vnName     vn1782  // descr [unit]
        // Contract: '#define' already eat
        RawDefine def;
        // [Label]
        skip_blanks();
        def.set_label( collect_identifier() );
        // [Value]
        skip_blanks();
        def.set_value( collect_token() );
        // [Comment]
        skip_blanks();
        if( eat_line_comment_start() )
           {
            skip_blanks();
            def.set_comment( collect_until_char_trimmed('\n') );
           }
        else
            {
             notify_error("Define {} hasn't a comment", def.label());
            }

        // Expecting a line end now
        skip_blanks();
        if( !eat_line_end() ) notify_error("Unexpected content after define \"{}\": {}", def.label(), str::escape(skip_line()));
        //DBGLOG("    [*] Collected define: label=\"{}\" value=\"{}\" comment=\"{}\"\n", def.label(), def.value(), def.comment())
        return def;
       };


    std::vector<RawDefine> defines;
    try{
        while( i<siz )
           {
            skip_blanks();
            if(i>=siz) break;
            else if( eat_line_comment_start() ) skip_line(); // A line comment
            //else if( eat_block_comment_start() ) skip_block_comment(); // If supporting block comments
            else if( eat_line_end() ) continue; // An empty line
            else if( eat_token("#define"sv) )
               {
                defines.push_back( collect_define() );
               }
            else
               {
                notify_error("Unexpected content: {}", str::escape(skip_line()));
               }
           } // while there's data
       }
    catch(std::exception& e)
       {
        throw dlg::parse_error(e.what(), line, i);
       }


    // Now I'll convert the suitable defines to pll::variable
    if( !defines.empty() )
       {
        //lib.global_variables().groups().emplace_back();
        //lib.global_variables().groups().back().set_name("Header_Variables");
        //lib.global_constants().groups().emplace_back();
        //lib.global_constants().groups().back().set_name("Header_Constants");
        for( const auto& def : defines )
           {
            // LABEL       0  // [INT] Descr
            // vnName     vn1782  // descr [unit]
            //DBGLOG(" - label=\"{}\" value=\"{}\" comment=\"{}\"\n", def.label(), def.value(), def.comment())
            
            //  vaProjName     AT %MB700.0    : STRING[ 80 ]; {DE:"va0    - Nome progetto caricato !HMI!"}
            //  vbHeartBeat    AT %MB300.2    : BOOL;         {DE:"vb2    - Battito di vita ogni secondo"}
            //  vnAlgn_Seq     AT %MW400.860  : INT;          {DE:"vn860  - Stato/risultato sequenze riscontri 'ID_ALGN'"}
            //  vqProd_X       AT %MD500.977  : DINT;         {DE:"vq977  - Posizione bordo avanti del prodotto [mm]"}
            //  vdPlcScanTime  AT %ML600.0    : LREAL;        {DE:"vd0    - Tempo di scansione del PLC [s]"}
            //  vdJobDate      AT %ML600.253  : LREAL;        {DE:"vd253  - Timestamp of last job start"}
            // RET_ABORTED        : INT := -1; { DE:"Return: Program not completed" }
            // NO_POS_UM          : DINT := 999999999; { DE:"Invalid quote [um]" }
            // BIT_CHS_CANTSTART  : UINT := 8192; { DE:"Stato Ch bit13: impossibile avviare il ciclo automatico per allarmi presenti o CMDA richiesto non attivo" }
            // SW_VER             : LREAL := 23.90; { DE:"Versione delle definizioni" }
           }
       }
    #undef notify_error
}


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



//---- end unit -------------------------------------------------------------
#endif
