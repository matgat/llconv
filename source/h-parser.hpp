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

#include "basic-parser.hpp" // BasicParser
#include "plc-elements.hpp" // plc::*, plcb::*
#include "sipro.hpp" // sipro::

using namespace std::literals; // "..."sv


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace h //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{


/////////////////////////////////////////////////////////////////////////////
// Descriptor of a '#define' entry in a buffer
class DefineBuf final
{
 public:
    [[nodiscard]] operator bool() const noexcept
       {
        return !i_Value.empty();
       }

    [[nodiscard]] std::string_view label() const noexcept { return i_Label; }
    void set_label(const std::string_view s)
       {
        if(s.empty()) throw std::runtime_error("Empty define label");
        i_Label = s;
       }

    [[nodiscard]] std::string_view value() const noexcept
       {
        return i_Value;
       }

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
class Parser final : public BasicParser
{
 public:
    Parser(const std::string_view b, std::vector<std::string>& sl, const bool f)
      : BasicParser(b,sl,f) {}

    //-----------------------------------------------------------------------
    [[nodiscard]] DefineBuf next_define()
       {
        DefineBuf def;

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
                else
                   {
                    notify_error("Unexpected content: {}", str::escape(skip_line()));
                   }
               }
           }
        catch(fmtstr::parse_error)
           {
            throw;
           }
        catch(std::exception& e)
           {
            throw fmtstr::parse_error(e.what(), line, i);
           }

        return def;
       }


 private:

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
        while( i<i_last )
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
    void collect_define(DefineBuf& def)
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
            if( i<siz && buf[i]=='[' )
               {
                ++i; // Skip '['
                skip_blanks();
                const std::size_t i_pre_start = i; // Start of pre-declaration
                std::size_t i_pre_end = i; // One-past-end of pre-declaration
                while( true )
                   {
                    if( i>=siz || buf[i]=='\n' )
                       {
                        notify_error("Unclosed initial \'[\' in the comment of define {}", def.label());
                        def.set_comment( std::string_view(buf+i_start, i_pre_end-i_start) );
                        break;
                       }
                    else if( buf[i]==']' )
                       {
                        def.set_comment_predecl( std::string_view(buf+i_pre_start, i_pre_end-i_pre_start) );
                        ++i; // Skip ']'
                        break;
                       }
                    else
                       {
                        if( !is_blank(buf[i]) ) i_pre_end = ++i;
                        else ++i;
                       }
                   }
                skip_blanks();
               }

            // Collect the remaining comment text
            if( !def.has_comment() && i<siz && buf[i]!='\n' )
               {
                const std::size_t i_txt_start = i; // Start of comment text
                std::size_t i_txt_end = i; // One-past-end of comment text
                do {
                    if( buf[i]=='\n' )
                       {// Line finished
                        break;
                       }
                    else
                       {
                        if( !is_blank(buf[i]) ) i_txt_end = ++i;
                        else ++i;
                       }
                   }
                while( i<siz );

                def.set_comment( std::string_view(buf+i_txt_start, i_txt_end-i_txt_start) );
               }
           }
        //else if(fussy)
        //   {
        //    notify_error("Define {} hasn't a comment", def.label());
        //   }

        // Expecting a line end here
        if( !eat_line_end() )
           {
            notify_error("Unexpected content after define: {}", str::escape(skip_line()));
           }

        //DBGLOG("    [*] Collected define: label=\"{}\" value=\"{}\" comment=\"{}\"\n", def.label(), def.value(), def.comment())
       }
};



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
    while( const DefineBuf def = parser.next_define() )
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
