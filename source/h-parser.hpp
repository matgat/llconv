#ifndef GUARD_h_parser_hpp
#define GUARD_h_parser_hpp
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
#include "format_string.hpp" // fmtstr::parse_error
#include "debug.hpp" // DBGLOG
#include "sipro.hpp" // sipro::

using namespace std::literals; // "..."sv


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace h //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{


/////////////////////////////////////////////////////////////////////////////
// Descriptor of a '#define' entry in a buffer
class DefineRef
{
 public:
    [[nodiscard]] operator bool() const noexcept { return !i_Value.empty(); }

    [[nodiscard]] std::string_view label() const noexcept { return i_Label; }
    void set_label(const std::string_view s)
       {
        if(s.empty()) throw std::runtime_error("Empty define label");
        i_Label = s;
       }

    [[nodiscard]] std::string_view value() const noexcept { return i_Value; }
    void set_value(const std::string_view s)
       {
        if(s.empty()) throw std::runtime_error("Empty define value");
        i_Value = s;
       }
    [[nodiscard]] bool value_is_number() const noexcept
       {
        double result;
        const auto i_end = i_Value.data() + i_Value.size();
        auto [i, ec] = std::from_chars(i_Value.data(), i_end, result);
        return ec==std::errc() && i==i_end;
       }

    [[nodiscard]] std::string_view comment() const noexcept { return i_Comment; }
    void set_comment(const std::string_view s) noexcept { i_Comment = s; }
    [[nodiscard]] bool has_comment() const noexcept { return !i_Comment.empty(); }

    [[nodiscard]] std::string_view comment_predecl() const noexcept { return i_CommentPreDecl; }
    void set_comment_predecl(const std::string_view s) noexcept { i_CommentPreDecl = s; }
    [[nodiscard]] bool has_comment_predecl() const noexcept { return !i_CommentPreDecl.empty(); }

 private:
    std::string_view i_Label;
    std::string_view i_Value;
    std::string_view i_Comment;
    std::string_view i_CommentPreDecl;
};


/////////////////////////////////////////////////////////////////////////////
// consteval friendly:
#define notify_error(...) \
   {\
    if(fussy) throw std::runtime_error( fmt::format(__VA_ARGS__) );\
    else issues.push_back( fmt::format("{} (line {}, offset {})"sv, fmt::format(__VA_ARGS__), line, i) );\
   }
/////////////////////////////////////////////////////////////////////////////
class Parser
{
 public:
    Parser(const std::string_view b, std::vector<std::string>& sl, const bool f)
      : buf(b.data())
      , siz(b.size())
      , line(1)
      , i(0)
      , issues(sl)
      , fussy(f)
       {
        // Check possible BOM    |  Encoding    |   Bytes     | Chars |
        //                       |--------------|-------------|-------|
        //                       | UTF-8        | EF BB BF    | ï»¿   |
        //                       | UTF-16 (BE)  | FE FF       | þÿ    |
        //                       | UTF-16 (LE)  | FF FE       | ÿþ    |
        //                       | UTF-32 (BE)  | 00 00 FE FF | ..þÿ  |
        //                       | UTF-32 (LE)  | FF FE 00 00 | ÿþ..  |
        if( siz < 2 )
           {
            notify_error("Empty file");
           }

        if( buf[0]=='\xFF' || buf[0]=='\xFE' || buf[0]=='\x00' )
           {
            throw std::runtime_error("Bad encoding, not UTF-8");
           }
       }

    [[nodiscard]] DefineRef next_define()
       {
        DefineRef def;

        try{
            while( i<siz )
               {
                skip_blanks();
                if( eat_line_comment_start() )
                   {
                    skip_line();
                   }
                else if( eat_block_comment_start() )
                   {
                    skip_block_comment();
                   }
                else if( eat_line_end() )
                   {// An empty line
                   }
                else if( eat_token("#define"sv) )
                   {
                    collect_define(def);
                    break;
                   }
                else notify_error("Unexpected content: {}", str::escape(skip_line()));
               }
           }
        catch(std::exception& e)
           {
            throw fmtstr::parse_error(e.what(), line, i);
           }

        return def;
       }

 private:
    const char* const buf;
    const std::size_t siz; // buffer size
    std::size_t line; // Current line number
    std::size_t i; // Current character
    std::vector<std::string>& issues; // Problems found
    const bool fussy;

    //-----------------------------------------------------------------------
    //template<typename ...Args> void notify_error(const std::string_view msg, Args&&... args) const
    //   {
    //    // Unfortunately this generates error: "call to immediate function is not a constant expression"
    //    //if(fussy) throw std::runtime_error( fmt::format(msg, args...) );
    //    //else issues.push_back( fmt::format("{} (line {}, offset {})"sv, fmt::format(msg, args...), line, i) );
    //    if(fussy) throw std::runtime_error( fmt::format(fmt::runtime(msg), args...) );
    //    else issues.push_back( fmt::format("{} (line {}, offset {})", fmt::format(fmt::runtime(msg), args...), line, i) );
    //   };

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
    bool eat_line_end() noexcept
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
    std::string_view skip_line() noexcept
       {
        const std::size_t i_start = i;
        while( i<siz && !eat_line_end() ) ++i;
        return std::string_view(buf+i_start, i-i_start);
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] bool eat_line_comment_start() noexcept
       {
        if( i<(siz-1) && buf[i]=='/' && buf[i+1]=='/' )
           {
            i += 2; // Skip "//"
            return true;
           }
        return false;
       }

    //-----------------------------------------------------------------------
    [[nodiscard]] bool eat_block_comment_start() noexcept
       {
        if( i<(siz-1) && buf[i]=='/' && buf[i+1]=='*' )
           {
            i += 2; // Skip "/*"
            return true;
           }
        return false;
       }

    //-----------------------------------------------------------------------
    void skip_block_comment()
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
        throw fmtstr::parse_error("Unclosed block comment", line_start, i_start);
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
    //[[nodiscard]] bool eat_directive_start() noexcept
    //   {
    //    if( i<siz && buf[i]=='#' )
    //       {
    //        ++i;
    //        return true;
    //       }
    //    return false;
    //   }

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
    void collect_define(DefineRef& def)
       {// LABEL       0  // [INT] Descr
        // vnName     vn1782  // descr [unit]
        // Contract: '#define' already eat
        // [Label]
        skip_blanks();
        def.set_label( collect_identifier() );
        // [Value]
        skip_blanks();
        def.set_value( collect_token() );
        // [Comment]
        skip_blanks();
        if( eat_line_comment_start() && i<siz )
           {
            skip_blanks();
            const std::size_t i_start = i; // Start of overall comment string

            // Detect possible pre-declarator in square brackets like: // [xxx] comment
            std::size_t i_pre_start = i; // Start of possible pre-declarator in square brackets
            std::size_t i_pre_len = 0;
            if( i<siz && buf[i]=='[' )
               {
                ++i; // Skip '['
                skip_blanks();
                i_pre_start = i;
                std::size_t i_last_not_blank = i;
                while( true )
                   {
                    if( i>=siz || buf[i]=='\n' )
                       {
                        notify_error("Unclosed initial \'[\' in the comment of define {}", def.label());
                        def.set_comment( std::string_view(buf+i_start, i_last_not_blank-i_start) );
                        break;
                       }
                    else if( buf[i]==']' )
                       {
                        i_pre_len = i_last_not_blank - i_pre_start + 1;
                        ++i; // Skip ']'
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
            if( i_pre_start<siz )
               {
                def.set_comment_predecl( std::string_view(buf+i_pre_start, i_pre_len) );
               }

            // Collect the actual comment text
            if( !def.has_comment() && i<siz && buf[i]!='\n' )
               {
                const std::size_t i_txt_start = i;
                std::size_t i_txt_len = 0;
                std::size_t i_last_not_blank = i;
                do {
                    if( buf[i]=='\n' )
                       {// Line finished
                        i_txt_len = i_last_not_blank - i_txt_start + 1;
                        break;
                       }
                    else
                       {
                        if( !is_blank(buf[i]) ) i_last_not_blank = i;
                        ++i;
                       }
                   }
                while( i<siz );

                def.set_comment( std::string_view(buf+i_txt_start, i_txt_len) );
               }
           }
        else
           {
            //notify_error("Define {} hasn't a comment", def.label());
            // Expecting a line end
            if( !eat_line_end() )
               {
                notify_error("Unexpected content after define: {}", str::escape(skip_line()));
               }
           }

        //DBGLOG("    [*] Collected define: label=\"{}\" value=\"{}\" comment=\"{}\"\n", def.label(), def.value(), def.comment())
       }
};

#undef notify_error



//---------------------------------------------------------------------------
// Parse a Sipro h file
void parse(const std::string_view buf, plcb::Library& lib, std::vector<std::string>& issues, const bool fussy)
{
    // Prepare the library containers for header data
    lib.global_variables().groups().emplace_back();
    lib.global_variables().groups().back().set_name("Header_Variables");
    lib.global_constants().groups().emplace_back();
    lib.global_constants().groups().back().set_name("Header_Constants");

    Parser parser(buf, issues, fussy);
    while( const DefineRef def = parser.next_define() )
       {
        //DBGLOG("Define - label=\"{}\" value=\"{}\" comment=\"{}\" predecl=\"{}\"\n", def.label(), def.value(), str::iso_latin1_to_utf8(def.comment()), def.comment_predecl())

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
        if( const sipro::Register reg(def.value());
            reg.is_valid() )
           {
            plcb::Variable var;

            var.set_name( def.label() );
            var.set_type( reg.iec_type() );
            if( reg.is_va() ) var.set_length( reg.get_va_length() );
            if( def.has_comment() ) var.set_descr( def.comment() );

            var.address().set_type( reg.iec_address_type() );
            var.address().set_typevar( reg.iec_address_vartype() );
            var.address().set_index( reg.iec_address_index() );
            var.address().set_subindex( reg.index() );

            lib.global_variables().groups().back().variables().push_back( var );
           }

        // Check if it's a numeric constant to be exported
        else if( def.value_is_number() )
           {
            // Must be exported to PLC?
            if( plc::is_num_type(def.comment_predecl()) )
               {
                plcb::Variable var;

                var.set_name( def.label() );
                var.set_type( def.comment_predecl() );

                var.set_value( def.value() );
                if( def.has_comment() ) var.set_descr( def.comment() );

                lib.global_constants().groups().back().variables().push_back( var );
               }
            //else
            //   {
            //    issues.push_back(fmt::format("{} value not exported: {}={} ({})", def.comment_predecl(), def.label(), def.value()));
            //   }
           }

        //else if( superfussy )
        //   {
        //    issues.push_back(fmt::format("Define not exported: {}={}"sv, def.label(), def.value()));
        //   }
       }

    if( lib.global_variables().groups().back().variables().empty() &&
        lib.global_constants().groups().back().variables().empty() )
       {
        if(fussy) throw std::runtime_error("No exportable defines found");
        else issues.push_back("No exportable defines found");
       }
}



}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::


//---- end unit -------------------------------------------------------------
#endif
