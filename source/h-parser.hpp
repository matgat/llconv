#ifndef h_parser_hpp
#define h_parser_hpp
/*  ---------------------------------------------
    ©2022 matteo.gattanini@gmail.com

    OVERVIEW
    ---------------------------------------------
    Parses a Sipro 'h' file containing a list
    of c-like preprocessor defines

    DEPENDENCIES:
    --------------------------------------------- */
#include <cctype> // std::isdigit, std::isblank, ...
#include <string_view>
#include <stdexcept> // std::exception, std::runtime_error, ...
#include <charconv> // std::from_chars
#include <fmt/core.h> // fmt::format

#include "string-utilities.hpp" // str::escape
#include "plc-elements.hpp" // plc::*, plcb::*
#include "logging.hpp" // dlg::parse_error, DBGLOG
#include "sipro.hpp" // sipro::

using namespace std::literals; // Use "..."sv


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace h //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{


/////////////////////////////////////////////////////////////////////////////
// Descriptor of a '#define' entry in the file
class RawDefine
{
 public:

    std::string_view label() const noexcept { return i_Label; }
    void set_label(const std::string_view s)
       {
        if(s.empty()) throw std::runtime_error("Empty define label");
        i_Label = s;
       }

    std::string_view value() const noexcept { return i_Value; }
    void set_value(const std::string_view s)
       {
        if(s.empty()) throw std::runtime_error("Empty define value");
        i_Value = s;
       }
    bool value_is_number() const noexcept
       {
        double result;
        const auto i_end = i_Value.data() + i_Value.size();
        auto [i, ec] = std::from_chars(i_Value.data(), i_end, result);
        return ec==std::errc() && i==i_end;
       }

    std::string_view comment() const noexcept { return i_Comment; }
    void set_comment(const std::string_view s) noexcept { i_Comment = s; }
    bool has_comment() const noexcept { return !i_Comment.empty(); }

    std::string_view comment_predecl() const noexcept { return i_CommentPreDecl; }
    void set_comment_predecl(const std::string_view s) noexcept { i_CommentPreDecl = s; }
    bool has_comment_predecl() const noexcept { return !i_CommentPreDecl.empty(); }

 private:
    std::string_view i_Label;
    std::string_view i_Value;
    std::string_view i_Comment;
    std::string_view i_CommentPreDecl;
};


//---------------------------------------------------------------------------
// Parse a generic 'h' file containing a list of c-like preprocessor defines
std::vector<RawDefine> parse(const std::string_view buf, std::vector<std::string>& issues, const bool fussy)
{
    std::vector<RawDefine> defines;

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
            return defines;
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
    // Skip space chars except new line
    auto skip_blanks = [&]() noexcept -> void
       {
        while( i<siz && is_blank(buf[i]) ) ++i;
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
    auto eat_block_comment_start = [&]() noexcept -> bool
       {
        if( i<(siz-1) && buf[i]=='/' && buf[i+1]=='*' )
           {
            i += 2; // Skip "/*"
            return true;
           }
        return false;
       };

    //---------------------------------
    auto skip_block_comment = [&]() -> void
       {
        const std::size_t line_start = line; // Store current line
        const std::size_t i_start = i; // Store current position
        const std::size_t sizm1 = siz-1;
        while( i<sizm1 )
           {
            if( buf[i]=='*' && buf[i+1]=='/' )
               {
                i += 2; // Skip "*/"
                return;
               }
            else if( buf[i]=='\n' )
               {
                ++line;
               }
            ++i;
           }
        throw dlg::parse_error("Unclosed block comment", line_start, i_start);
       };

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
            const std::size_t i_start = i; // Start of overall comment string

            // Detect possible pre-declarator in square brackets like: // [xxx] comment
            std::size_t i_pre_start = i; // Start of possible pre-declarator in square brackets
            std::size_t i_pre_len = 0;
            if( buf[i]=='[' )
               {
                ++i; // Skip '['
                skip_blanks();
                i_pre_start = i;
                std::size_t i_last_not_blank = i;
                while( i<siz )
                   {
                    if( buf[i]==']' )
                       {
                        i_pre_len = i_last_not_blank - i_pre_start;
                        ++i; // Skip ']'
                        break;
                       }
                    else if( buf[i]=='\n' )
                       {
                        notify_error("Unclosed initial \'[\' in the comment of define {}", def.label());
                        def.set_comment( std::string_view(buf.data()+i_start, i_last_not_blank-i_start) );
                        break;
                       }
                    else
                       {
                        if( !is_blank(buf[i]) ) i_last_not_blank = i;
                        ++i;
                       }
                   }
                skip_blanks();
               }
            def.set_comment_predecl( std::string_view(buf.data()+i_pre_start, i_pre_len) );

            // Collect the actual comment text
            if( !def.has_comment() && buf[i]!='\n' )
               {
                const std::size_t i_txt_start = i;
                std::size_t i_txt_len = 0;
                std::size_t i_last_not_blank = i;
                while( i<siz )
                   {
                    if( buf[i]=='\n' )
                       {// Line finished
                        i_txt_len = i_last_not_blank - i_txt_start;
                        break;
                       }
                    else
                       {
                        if( !is_blank(buf[i]) ) i_last_not_blank = i;
                        ++i;
                       }
                   }
                def.set_comment( std::string_view(buf.data()+i_txt_start, i_txt_len) );
               }
           }
        else
           {
            notify_error("Define {} hasn't a comment", def.label());
           }

        // Expecting a line end here
        skip_blanks();
        if( !eat_line_end() ) notify_error("Unexpected content after define \"{}\": {}", def.label(), str::escape(skip_line()));
        //DBGLOG("    [*] Collected define: label=\"{}\" value=\"{}\" comment=\"{}\"\n", def.label(), def.value(), def.comment())
        return def;
       };


    try{
        while( i<siz )
           {
            skip_blanks();
            if(i>=siz) break;
            else if( eat_line_comment_start() ) skip_line(); // A line comment
            else if( eat_block_comment_start() ) skip_block_comment(); // If supporting block comments
            else if( eat_line_end() ) continue; // An empty line
            else if( eat_token("#define"sv) ) defines.push_back( collect_define() );
            else notify_error("Unexpected content: {}", str::escape(skip_line()));
           }
       }
    catch(std::exception& e)
       {
        throw dlg::parse_error(e.what(), line, i);
       }

    return defines;

    #undef notify_error
}




//---------------------------------------------------------------------------
// Parse a Sipro h file
void parse(const std::string_view buf, plc::Library& lib, std::vector<std::string>& issues, const bool fussy =true)
{
    // Get the raw defines
    std::vector<RawDefine> defines = parse(buf, issues, fussy);
    if( defines.empty() ) throw std::runtime_error("No defines found");

    // Now I'll convert the suitable defines to plc variable descriptors
    lib.global_variables().groups().emplace_back();
    lib.global_variables().groups().back().set_name("Header_Variables");
    lib.global_constants().groups().emplace_back();
    lib.global_constants().groups().back().set_name("Header_Constants");
    for( auto& def : defines )
       {
        //DBGLOG(" - label=\"{}\" value=\"{}\" comment=\"{}\"\n", def.label(), def.value(), def.comment())

        // Must export these:
        //
        // Sipro registers
        // vnName     vn1782  // descr
        //             ↑ Sipro register
        //
        // Numeric constants
        // LABEL     123       // [INT] Descr
        //            ↑ Value       ↑ IEC61131-3 type

        // Check if it's a Sipro register
        if( sipro::Register reg = sipro::parse_register(def.value());
            reg.is_valid() )
           {
           }

        // Check if it's a numeric constant to be exported
        else if( def.value_is_number() && plc::is_num_type(def.comment_predecl()) )
           {
            plcb::Variable var;

            //var.set_name(s)
            //var.set_type(s)
            //var.set_length(n)
            //var.set_arraydim(n)
            //var.set_value(s)
            //var.set_descr(s)
            //
            //var.Address.set_type(s)
            //var.Address.set_typevar(s)
            //var.Address.set_index(s)
            //var.Address.set_subindex(s)

            lib.global_constants().variables().push_back( var );
           }


        //  vaProjName     AT %MB700.0    : STRING[ 80 ]; {DE:"va0    - Nome progetto caricato"}
        //  vbHeartBeat    AT %MB300.2    : BOOL;         {DE:"vb2    - Battito di vita ogni secondo"}
        //  vnAlgn_Seq     AT %MW400.860  : INT;          {DE:"vn860  - Stato/risultato sequenze riscontri 'ID_ALGN'"}
        //  vqProd_X       AT %MD500.977  : DINT;         {DE:"vq977  - Posizione bordo avanti del prodotto [mm]"}
        //  vdPlcScanTime  AT %ML600.0    : LREAL;        {DE:"vd0    - Tempo di scansione del PLC [s]"}
        //  vdJobDate      AT %ML600.253  : LREAL;        {DE:"vd253  - Timestamp of last job start"}

        // RET_ABORTED        : INT := -1; { DE:"Return: Program not completed" }
        // BIT_CHS_CANTSTART  : UINT := 8192; { DE:"Stato Ch bit13: impossibile avviare il ciclo automatico per allarmi presenti o CMDA richiesto non attivo" }
        // NO_POS_UM          : DINT := 999999999; { DE:"Invalid quote [um]" }
        // SW_VER             : LREAL := 23.90; { DE:"Versione delle definizioni" }
       }

}

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



//---- end unit -------------------------------------------------------------
#endif
