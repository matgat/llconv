#ifndef pll_parser_hpp
#define pll_parser_hpp
/*  ---------------------------------------------
    ©2021 matteo.gattanini@gmail.com

    OVERVIEW
    ---------------------------------------------
    LogicLab 'pll' file format

    DEPENDENCIES:
    --------------------------------------------- */
#include <cctype> // std::isdigit, std::isblank, ...
#include <cmath> // std::pow, ...
#include <string_view>
//#include <limits> // std::numeric_limits
#include <stdexcept> // std::exception, std::runtime_error, ...
#include <fmt/core.h> // fmt::format

#include "string-utilities.hpp" // str::escape
#include "plc-elements.hpp" // plcb::*
#include "logging.hpp" // dlg::parse_error, dlg::error, DBGLOG

using namespace std::literals; // Use "..."sv


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace pll //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{


//---------------------------------------------------------------------------
// Parse pll file
void parse(const std::string_view buf, plcb::Library& lib, std::vector<std::string>& issues, const bool fussy =true)
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

    // Doesn't play well with windows EOL "\r\n", since uses string_view extensively, cannot eat '\r'
    //if(buf.find('\r') != buf.npos) throw std::runtime_error("EOL is not unix, remove CR (\\r) character");

    std::size_t line = 1; // Current line number
    std::size_t i = 0; // Current character index

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
    auto skip_comment = [&]() -> void
       {
        const std::size_t line_start = line; // Store current line
        const std::size_t i_start = i;       //               position
        const std::size_t sizm1 = siz-1;
        while( i<sizm1 )
           {
            if( buf[i]=='*' && buf[i+1]==')' )
               {
                //DBGLOG("    [*] Comment skipped at line {}\n", line)
                i += 2; // Skip "*)"
                return;
               }
            else if( buf[i]=='\n' )
               {
                ++line;
               }
            ++i;
           }
        throw dlg::parse_error("Unclosed comment", line_start, i_start);
       };

    //---------------------------------
    auto eat_comment_start = [&]() noexcept -> bool
       {
        if( i<(siz-1) && buf[i]=='(' && buf[i+1]=='*' )
           {
            i += 2; // Skip "(*"
            return true;
           }
        return false;
       };

    //---------------------------------
    auto eat_string = [&](const std::string_view s) noexcept -> bool
       {
        const std::size_t i_end = i+s.length();
        if( i_end<=siz && s==std::string_view(buf.data()+i,s.length()) )
           {
            i = i_end;
            return true;
           }
        return false;
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
    //auto collect_token = [&]() noexcept -> std::string_view
    //   {
    //    const std::size_t i_start = i;
    //    while( i<siz && !std::isspace(buf[i]) ) ++i;
    //    return std::string_view(buf.data()+i_start, i-i_start);
    //   };

    //---------------------------------
    auto collect_identifier = [&]() noexcept -> std::string_view
       {
        const std::size_t i_start = i;
        while( i<siz && (std::isalnum(buf[i]) || buf[i]=='_') ) ++i;
        return std::string_view(buf.data()+i_start, i-i_start);
       };

    //---------------------------------
    auto collect_numeric_value = [&]() noexcept -> std::string_view
       {
        const std::size_t i_start = i;
        while( i<siz && (std::isdigit(buf[i]) || buf[i]=='+' || buf[i]=='-' || buf[i]=='.' || buf[i]=='E') ) ++i;
        return std::string_view(buf.data()+i_start, i-i_start);
       };

    //---------------------------------
    auto collect_digits = [&]() noexcept -> std::string_view
       {
        const std::size_t i_start = i;
        while( i<siz && std::isdigit(buf[i]) ) ++i;
        return std::string_view(buf.data()+i_start, i-i_start);
       };

    //---------------------------------
    // Read a (base10) positive integer literal
    auto extract_index = [&]() -> std::size_t
       {
        if( i>=siz ) throw dlg::error("Index not found");
        if( buf[i]=='+' ) ++i; // sign = 1;
        else if( buf[i]=='-' ) throw dlg::error("Negative index"); // {sign = -1; ++i;}
        if( !std::isdigit(buf[i]) ) throw dlg::error("Invalid index");
        std::size_t result = (buf[i]-'0');
        const std::size_t base = 10u;
        while( ++i<siz && std::isdigit(buf[i]) ) result = (base*result) + (buf[i]-'0');
        return result;
       };

    //---------------------------------
    // Read a (base10) floating point number literal
    auto extract_double = [&]() -> double
       {
        // [sign]
        int sgn = 1;
        if( buf[i]=='-' ) {sgn = -1; ++i;}
        else if( buf[i]=='+' ) ++i;

        // [mantissa - integer part]
        double mantissa = 0;
        bool found_mantissa = false;
        if( std::isdigit(buf[i]) )
           {
            found_mantissa = true;
            do {
                mantissa = (10.0 * mantissa) + static_cast<double>(buf[i] - '0');
                //if( buf[++i] == '\'' ); // Skip thousand separator char
               }
            while( std::isdigit(buf[i]) );
           }
        // [mantissa - fractional part]
        if( buf[i] == '.' )
           {
            ++i;
            double k = 0.1; // shift of decimal part
            if( std::isdigit(buf[i]) )
               {
                found_mantissa = true;
                do {
                    mantissa += k * static_cast<double>(buf[i] - '0');
                    k *= 0.1;
                    ++i;
                   }
                while( std::isdigit(buf[i]) );
               }
           }

        // [exponent]
        int exp=0, exp_sgn=1;
        bool found_expchar = false,
             found_expval = false;
        if( buf[i] == 'E' || buf[i] == 'e' )
           {
            found_expchar = true;
            ++i;
            // [exponent sign]
            if( buf[i] == '-' ) {exp_sgn = -1; ++i;}
            else if( buf[i] == '+' ) ++i;
            // [exponent value]
            if( std::isdigit(buf[i]) )
               {
                found_expval = true;
                do {
                    exp = (10 * exp) + static_cast<int>(buf[i] - '0');
                    ++i;
                   }
                while( std::isdigit(buf[i]) );
               }
           }
        if( found_expchar && !found_expval ) throw dlg::error("Invalid floating point number: No exponent value"); // ex "123E"

        // [calculate result]
        double result = 0.0;
        if( found_mantissa )
          {
           result = sgn * mantissa * std::pow(10.0, exp_sgn*exp);
          }
        else if( found_expval )
           {
            result = sgn * std::pow(10.0,exp_sgn*exp); // ex "E100"
           }
        else
           {
            throw dlg::error("Invalid floating point number");
            //if(found_expchar) result = 1.0; // things like 'E,+E,-exp,exp+,E-,...'
            //else result = std::numeric_limits<double>::quiet_NaN(); // things like '+,-,,...'
           }
        return result;
       };

    //---------------------------------
    //auto collect_until_char_same_line = [&](const char c) -> std::string_view
    //   {
    //    const std::size_t i_start = i;
    //    while( i<siz )
    //       {
    //        if( buf[i]==c )
    //           {
    //            //DBGLOG("    [*] Collected \"{}\" at line {}\n", std::string_view(buf.data()+i_start, i-i_start), line)
    //            return std::string_view(buf.data()+i_start, i-i_start);
    //           }
    //        else if( buf[i]=='\n' ) break;
    //        ++i;
    //       }
    //    throw dlg::error("Unclosed content (\'{}\' expected)", str::escape(c));
    //   };

    //---------------------------------
    //auto collect_until_char = [&](const char c) -> std::string_view
    //   {
    //    const std::size_t i_start = i;
    //    while( i<siz )
    //       {
    //        if( buf[i]==c )
    //           {
    //            //DBGLOG("    [*] Collected \"{}\" at line {}\n", std::string_view(buf.data()+i_start, i-i_start), line)
    //            return std::string_view(buf.data()+i_start, i-i_start);
    //           }
    //        else if( buf[i]=='\n' ) ++line;
    //        ++i;
    //       }
    //    throw dlg::error("Unclosed content (\'{}\' expected)", str::escape(c));
    //   };


    //---------------------------------
    auto collect_until_char_trimmed = [&](const char c) -> std::string_view
       {
        const std::size_t line_start = line; // Store current line
        const std::size_t i_start = i;       //               position
        while( i<siz )
           {
            if( buf[i]==c )
               {
                const std::size_t i_end = last_not_blank(i);
                return std::string_view(buf.data()+i_start, i_end-i_start);
               }
            else if( buf[i]=='\n' ) ++line;
            ++i;
           }
        throw dlg::parse_error(fmt::format("Unclosed content (\'{}\' expected)", str::escape(c)), line_start, i_start);
       };

    //---------------------------------
    //auto collect_until_token = [&](const std::string_view tok) -> std::string_view
    //   {
    //    const std::size_t i_start = i;
    //    const std::size_t max_siz = siz-tok.length();
    //    while( i<max_siz )
    //       {
    //        if( buf[i]==tok[0] && eat_token(tok) )
    //           {
    //            return std::string_view(buf.data()+i_start, i-i_start-tok.length());
    //           }
    //        else if( buf[i]=='\n' ) ++line;
    //        ++i;
    //       }
    //    throw dlg::error(");
    //    throw dlg::parse_error(fmt::format("Unclosed content (\"{}\" expected)",tok), line_start, i_start);
    //   };

    //---------------------------------
    auto collect_until_newline_token = [&](const std::string_view tok) -> std::string_view
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
                    return std::string_view(buf.data()+i_start, i-i_start-tok.length());
                   }
               }
            else ++i;
           }
        throw dlg::parse_error(fmt::format("Unclosed content (\"{}\" expected)",tok), line_start, i_start);
       };

    //---------------------------------
    auto is_directive = [&]() noexcept -> bool
       {//{ DE:"some string" }, { CODE:ST }, ...
        if( i<siz && buf[i]=='{' )
           {
            ++i;
            return true;
           }
        return false;
       };

    //---------------------------------
    auto collect_directive = [&]() -> plcb::Directive
       {//{ DE:"some string" }, { CODE:ST }, ...
        // Contract: already checked if( i>=siz || buf[i]!='{' ) { return; }
        //++i; // Skip '{'
        skip_blanks();
        plcb::Directive dir;
        dir.set_key( collect_identifier() );
        skip_blanks();
        if( i>=siz || buf[i]!=':' ) throw dlg::error("Missing \':\' after directive {}", dir.key());
        ++i; // Skip ':'
        skip_blanks();
        if( i>=siz ) throw dlg::error("Truncated directive {}", dir.key());
        if( buf[i]=='\"' )
           {
            ++i; // Skip the first '\"'
            //dir.set_value( collect_until_char_same_line('\"') );
            const std::size_t i_start = i;
            while( i<siz && buf[i]!='\"' )
               {
                if( buf[i]=='\n' ) throw dlg::error("Unclosed directive {} value (\'\"\' expected)", dir.key());
                else if( buf[i]=='<' || buf[i]=='>' || buf[i]=='\"' ) throw dlg::error("Invalid character \'{}\' in directive {} value", buf[i], dir.key());
                ++i;
               }
            dir.set_value( std::string_view(buf.data()+i_start, i-i_start) );
            ++i; // Skip the second '\"'
           }
        else
           {
            dir.set_value( collect_identifier() );
           }
        skip_blanks();
        if( i>=siz || buf[i]!='}' ) throw dlg::error("Unclosed directive {} after {}", dir.key(), dir.value());
        ++i; // Skip '}'
        //DBGLOG("    [*] Collected directive \"{}\" at line {}\n", dir.key(), line)
        return dir;
       };

    //---------------------------------
    auto collect_parameter = [&]() -> plcb::Macro::Parameter
       {//   WHAT; {DE:"Parameter description"}
        plcb::Macro::Parameter par;
        skip_blanks();
        par.set_name( collect_identifier() );
        skip_blanks();
        if( i>=siz || buf[i]!=';' ) throw dlg::error("Missing \';\' after macro parameter");
        ++i; // Skip ';'
        skip_blanks();
        if( is_directive() )
           {
            plcb::Directive dir = collect_directive();
            if( dir.key() == "DE"sv ) par.set_descr( dir.value() );
            else notify_error("Unexpected directive \"{}\" in macro parameter", dir.key());
           }
        // Expecting a line end now
        skip_blanks();
        if( !eat_line_end() ) notify_error("Unexpected content after macro parameter: {}", str::escape(skip_line()));
        return par;
       };

    //---------------------------------
    auto collect_rest_of_variable = [&](plcb::Variable& var) -> void
       {// ... STRING[ 80 ]; { DE:"descr" }
        // ... ARRAY[ 0..999 ] OF BOOL; { DE:"descr" }
        // ... ARRAY[1..2, 1..2] OF DINT := [1, 2, 3, 4]; { DE:"multidimensional array" }
        // [Array data]
        skip_blanks();
        if( eat_token("ARRAY"sv) )
           {// Specifying an array ex. ARRAY[ 0..999 ] OF BOOL;
            // Get array size
            skip_blanks();
            if( i>=siz || buf[i]!='[' ) throw dlg::error("Expected \'[\' in array variable \"{}\"", var.name());
            ++i; // Skip '['
            // TODO: collect array dimensions (multidimensional: dim0, dim1, dim2)
            skip_blanks();
            const std::size_t start_idx = extract_index();
            if( start_idx!=0u ) throw dlg::error("Invalid array start index ({}) of variable \"{}\"", start_idx, var.name());
            skip_blanks();
            if( !eat_string(".."sv) ) throw dlg::error("Expected \"..\" in array index of variable \"{}\"", var.name());
            skip_blanks();
            const std::size_t end_idx = extract_index();
            if( end_idx < start_idx ) throw dlg::error("Invalid array indexes {}..{} of variable \"{}\"", start_idx, end_idx, var.name());
            skip_blanks();
            if( i<siz && buf[i]==',' ) throw dlg::error("Multidimensional arrays not yet supported in variable \"{}\"", var.name());
            if( i>=siz || buf[i]!=']' ) throw dlg::error("Expected \']\' in array variable \"{}\"", var.name());
            ++i; // Skip ']'
            skip_blanks();
            if( !eat_token("OF"sv) ) throw dlg::error("Expected \"OF\" in array variable \"{}\"", var.name());
            var.set_arraydim( (end_idx+1u) - start_idx );
            skip_blanks();
           }

        // [Type]
        var.set_type( collect_identifier() );
        skip_blanks();
        // Now I expect an array decl '[', a value ':' or simply an end ';'
        if( buf[i] == '[' )
           {
            ++i; // Skip '['
            skip_blanks();
            const std::size_t len = extract_index();
            if( len<=1u ) throw dlg::error("Invalid length ({}) of variable \"{}\"", len, var.name());
            skip_blanks();
            if( i>=siz || buf[i]!=']' ) throw dlg::error("Expected \']\' in variable length \"{}\"", var.name());
            ++i; // Skip ']'
            skip_blanks();
            var.set_length( len );
           }

        // [Value]
        if( buf[i]==':' )
           {
            ++i; // Skip ':'
            if( i>=siz || buf[i]!='=' ) throw dlg::error("Unexpected colon in variable \"{}\" type", var.name());
            ++i; // Skip '='
            skip_blanks();
            if( i<siz && buf[i]=='[' ) throw dlg::error("Array initialization not yet supported in variable \"{}\"", var.name());

            //var.set_value( collect_numeric_value() );
            const std::size_t i_start = i;
            while( i<siz )
               {
                if( buf[i]==';' )
                   {
                    const std::size_t i_end = last_not_blank(i);
                    var.set_value( std::string_view(buf.data()+i_start, i_end-i_start) );
                    //DBGLOG("    [*] Collected var value \"{}\"\n", var.name())
                    ++i; // Skip ';'
                    break;
                   }
                else if( buf[i]=='\n' ) throw dlg::error("Unclosed variable \"{}\" value {} (\';\' expected)", var.name(), std::string_view(buf.data()+i_start, i-i_start));
                else if( buf[i]==':' || buf[i]=='=' || buf[i]=='<' || buf[i]=='>' || buf[i]=='\"' ) throw dlg::error("Invalid character \'{}\' in variable \"{}\" value {}", buf[i], var.name(), std::string_view(buf.data()+i_start, i-i_start));
                ++i;
               }
           }
        else if( buf[i] == ';' )
           {
            ++i; // Skip ';'
           }

        // [Description]
        skip_blanks();
        if( is_directive() )
           {
            plcb::Directive dir = collect_directive();
            if( dir.key() == "DE"sv ) var.set_descr( dir.value() );
            else notify_error("Unexpected directive \"{}\" in variable \"{}\" declaration", dir.key(), var.name());
           }

        // Expecting a line end now
        skip_blanks();
        if( !eat_line_end() ) notify_error("Unexpected content after variable \"{}\" declaration: {}", var.name(), str::escape(skip_line()));
        //DBGLOG("    [*] Collected var: name=\"{}\" type=\"{}\" val=\"{}\" descr=\"{}\"\n", var.name(), var.type(), var.value(), var.descr())
       };

    //---------------------------------
    auto collect_variable = [&]() -> plcb::Variable
       {//  VarName : Type := Val; { DE:"descr" }
        //  VarName AT %MB300.6000 : ARRAY[ 0..999 ] OF BOOL; { DE:"descr" }
        //  VarName AT %MB700.0 : STRING[ 80 ]; {DE:"descr"}
        plcb::Variable var;

        // [Name]
        skip_blanks();
        var.set_name( collect_identifier() );
        skip_blanks();
        if( i<siz && buf[i]==',' ) throw dlg::error("Multiple names not supported in declaration of variable \"{}\"", var.name());

        // [Location address]
        if( eat_token("AT"sv) )
           {// Specified a location address %<type><typevar><index>.<subindex>
            skip_blanks();
            if( i>=siz || buf[i]!='%' ) throw dlg::error("Expected \'%\' in variable \"{}\" address", var.name());
            ++i; // Skip '%'
            // Here expecting something like: MB300.6000
            //std::size_t i_start = i;
            var.address().set_type( std::string_view(buf.data()+i, 1) ); i+=1;
            var.address().set_typevar( std::string_view(buf.data()+i, 1) ); i+=1;
            var.address().set_index( collect_digits() );
            if( i>=siz || buf[i]!='.' ) throw dlg::error("Expected \'.\' in variable \"{}\" address", var.name());
            ++i; // Skip '.'
            var.address().set_subindex( collect_digits() );
            skip_blanks();
           }

        // [Name/Type separator]
        if( i>=siz || buf[i]!=':' ) throw dlg::error("Expected \':\' before variable \"{}\" type", var.name());
        ++i; // Skip ':'

        collect_rest_of_variable(var);
        return var;
       };

    //---------------------------------
    auto collect_rest_of_struct = [&](plcb::Struct& strct) -> void
       {// <name> : STRUCT { DE:"struct descr" }
        //    x : DINT; { DE:"member descr" }
        //    y : DINT; { DE:"member descr" }
        //END_STRUCT;
        // Name already collected, "STRUCT" already skipped
        // Possible description
        skip_blanks();
        if( is_directive() )
           {
            plcb::Directive dir = collect_directive();
            if( dir.key() == "DE"sv ) strct.set_descr( dir.value() );
            else notify_error("Unexpected directive \"{}\" in struct \"{}\"", dir.key(), strct.name());
           }

        while( i<siz )
           {
            do{ skip_blanks(); } while( eat_line_end() ); // Skip empty lines
            if( eat_string("END_STRUCT;"sv) ) break;
            strct.members().push_back( collect_variable() );
            // Some checks
            //if( strct.members().back().has_value() ) throw dlg::error("Struct member \"{}\" cannot have a value ({})", strct.members().back().name(), strct.members().back().value());
            if( strct.members().back().has_address() ) throw dlg::error("Struct member \"{}\" cannot have an address", strct.members().back().name());
           }

        // Expecting a line end now
        skip_blanks();
        if( !eat_line_end() ) notify_error("Unexpected content after struct \"{}\": {}", strct.name(), str::escape(skip_line()));
        //DBGLOG("    [*] Collected struct \"{}\", {} members\n", strct.name(), strct.members().size())
       };

    //---------------------------------
    auto collect_enum_element = [&](plcb::Enum::Element& elem) -> bool
       {// VAL1 := 0, { DE:"elem descr" }
        // [Name]
        do{ skip_blanks(); } while( eat_line_end() ); // Skip empty lines
        elem.set_name( collect_identifier() );

        // [Value]
        skip_blanks();
        if( !eat_string(":="sv) ) throw dlg::error("Value not found in enum element \"{}\"", elem.name());
        skip_blanks();
        elem.set_value( collect_numeric_value() );
        skip_blanks();

        // Qui c'è una virgola se ci sono altri valori successivi
        bool has_next = i<siz && buf[i]==',';
        if( has_next ) ++i; // Skip comma

        // [Description]
        skip_blanks();
        if( is_directive() )
           {
            plcb::Directive dir = collect_directive();
            if( dir.key() == "DE"sv ) elem.set_descr( dir.value() );
            else notify_error("Unexpected directive \"{}\" in enum element \"{}\"", dir.key(), elem.name());
           }

        // Expecting a line end now
        skip_blanks();
        if( !eat_line_end() ) notify_error("Unexpected content after enum element \"{}\": {}", elem.name(), str::escape(skip_line()));
        //DBGLOG("    [*] Collected enum element: name=\"{}\" value=\"{}\" descr=\"{}\"\n", elem.name(), elem.value(), elem.descr())
        return has_next;
       };

    //---------------------------------
    auto collect_rest_of_enum = [&](plcb::Enum& en) -> void
       {// <name>: ( { DE:"enum descr" }
        //     VAL1 := 0, { DE:"elem descr" }
        //     VAL2 := -1 { DE:"elem desc" }
        // );
        // Name already collected, ": (" already skipped
        // Possible description
        skip_blanks();
        if( is_directive() )
           {
            plcb::Directive dir = collect_directive();
            if( dir.key() == "DE"sv ) en.set_descr( dir.value() );
            else notify_error("Unexpected directive \"{}\" in enum \"{}\"", dir.key(), en.name());
           }

        // Elements
        bool has_next;
        do {
            plcb::Enum::Element elem;
            has_next = collect_enum_element(elem);
            en.elements().push_back(elem);
           }
        while( has_next );

        // End expected
        skip_blanks();
        if( !eat_string(");"sv) ) throw dlg::error("Expected termination \");\" after enum \"{}\"", en.name());

        // Expecting a line end now
        skip_blanks();
        if( !eat_line_end() ) notify_error("Unexpected content after enum \"{}\": {}", en.name(), str::escape(skip_line()));
        //DBGLOG("    [*] Collected enum \"{}\", {} elements\n", en.name(), en.elements().size())
       };

    //---------------------------------
    auto collect_rest_of_subrange = [&](plcb::Subrange& subr) -> void
    {// <name> : DINT (5..23); { DE:"descr" }
     // Name already collected, ':' already skipped

        // [Type]
        skip_blanks();
        subr.set_type(collect_identifier());

        // [Min and Max]
        skip_blanks();
        if (i >= siz || buf[i] != '(') throw dlg::error("Expected \"(min..max)\" in subrange \"{}\"", subr.name());
        ++i; // Skip '('
        skip_blanks();
        const double min_val = extract_double();
        skip_blanks();
        if (!eat_string(".."sv)) throw dlg::error("Expected \"..\" in subrange \"{}\"", subr.name());
        skip_blanks();
        const double max_val = extract_double();
        skip_blanks();
        if (i >= siz || buf[i] != ')') throw dlg::error("Expected \')\' in subrange \"{}\"", subr.name());
        ++i; // Skip ')'
        subr.set_range(min_val, max_val);

        // [Description]
        skip_blanks();
        if (is_directive())
        {
            plcb::Directive dir = collect_directive();
            if (dir.key() == "DE"sv) subr.set_descr(dir.value());
            else notify_error("Unexpected directive \"{}\" in subrange \"{}\" declaration", dir.key(), subr.name());
        }

        // Expecting a line end now
        skip_blanks();
        if (!eat_line_end()) notify_error("Unexpected content after subrange \"{}\" declaration: {}", subr.name(), str::escape(skip_line()));
        //DBGLOG("    [*] Collected subrange: name=\"{}\" type=\"{}\" min=\"{}\" max=\"{}\" descr=\"{}\"\n", subr.name(), subr.type(), subr.min(), subr.max(), subr.descr())
    };


    enum class STS
       {
        HEADER,
        SEE,
        POU,
        MACRO,
        GLOBALVARS,
        TYPE
       } status = STS::HEADER;

    enum class SUB
       {
        GET_HEADER,
        GET_VARS,
        GET_BODY
       } substatus = SUB::GET_HEADER;

    struct
       {
        std::string_view pou_start;
        std::string_view pou_end;
        std::vector<plcb::Pou>* pous = nullptr;
        std::vector<plcb::Variable>* vars = nullptr;
        plcb::Variables_Groups* gvars = nullptr;
        bool var_value_needed = false;
       } collecting;

    try{
        while( i<siz )
           {
            switch( status )
               {
                // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - --
                case STS::SEE : // See what we have
                    skip_blanks();
                    if(i>=siz) continue;
                    else if( eat_comment_start() ) skip_comment();
                    else if( eat_line_end() ) continue;
                    else if( eat_token("PROGRAM"sv) )
                       {
                        collecting.pou_start = "PROGRAM"sv;
                        collecting.pou_end = "END_PROGRAM"sv;
                        collecting.pous = &(lib.programs());

                        skip_blanks();
                        std::string_view name = collect_identifier();
                        //DBGLOG("Found {} {} in line {}\n", collecting.pou_start, name, line)
                        collecting.pous->emplace_back();
                        collecting.pous->back().set_name(name);
                        status = STS::POU;
                        substatus = SUB::GET_HEADER;
                       }
                    else if( eat_token("FUNCTION_BLOCK"sv) )
                       {
                        collecting.pou_start = "FUNCTION_BLOCK"sv;
                        collecting.pou_end = "END_FUNCTION_BLOCK"sv;
                        collecting.pous = &(lib.function_blocks());

                        skip_blanks();
                        std::string_view name = collect_identifier();
                        //DBGLOG("Found {} {} in line {}\n", collecting.pou_start, name, line)
                        collecting.pous->emplace_back();
                        collecting.pous->back().set_name(name);
                        status = STS::POU;
                        substatus = SUB::GET_HEADER;
                       }
                    else if( eat_token("FUNCTION"sv) )
                       {
                        collecting.pou_start = "FUNCTION"sv;
                        collecting.pou_end = "END_FUNCTION"sv;
                        collecting.pous = &(lib.functions());

                        skip_blanks();
                        std::string_view name = collect_identifier();
                        //DBGLOG("Found {} {} in line {}\n", collecting.pou_start, name, line)
                        collecting.pous->emplace_back();
                        collecting.pous->back().set_name(name);
                        // Dopo il nome dovrebbe esserci la dichiarazione del parametro ritornato
                        skip_blanks();
                        if( i>=siz || buf[i]!=':' ) throw dlg::error("Missing return type in function \"{}\"", name);
                        ++i; // Skip ':'
                        skip_blanks();
                        collecting.pous->back().set_return_type( collect_until_char_trimmed('\n') );
                        status = STS::POU;
                        substatus = SUB::GET_HEADER;
                       }
                    else if( eat_token("MACRO"sv) )
                       {
                        skip_blanks();
                        std::string_view name = collect_identifier();
                        //DBGLOG("Found macro {} in line {}\n", name, line)
                        lib.macros().emplace_back();
                        lib.macros().back().set_name(name);
                        status = STS::MACRO;
                        substatus = SUB::GET_HEADER;
                       }
                    else if( eat_token("TYPE"sv) )
                       {
                        skip_blanks();
                        //DBGLOG("Found type declaration section in line {}\n", line)
                        status = STS::TYPE;
                       }
                    else if( eat_token("VAR_GLOBAL"sv) )
                       {
                        //DBGLOG("Found global vars in line {}\n", line)
                        // Check if there's some additional attributes
                        skip_blanks();
                        if( eat_token("CONSTANT"sv) )
                           {
                            collecting.gvars = &( lib.global_constants() );
                            collecting.var_value_needed = true;
                            status = STS::GLOBALVARS;
                           }
                        else if( eat_line_end() )
                           {
                            collecting.gvars = &( lib.global_variables() );
                            collecting.var_value_needed = false;
                            status = STS::GLOBALVARS;
                           }
                        else
                           {
                            throw dlg::error("Unexpected content in global variables declaration: {}", str::escape(skip_line()));
                           }
                       }
                    else
                       {
                        notify_error("Unexpected content: {}", str::escape(skip_line()));
                       }
                    break; // STS::SEE

                // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - --
                case STS::POU :
                    //POU NAME : RETURN_VALUE
                    //{ DE:"Description" }
                    //    VAR_YYY
                    //    ...
                    //    END_VAR
                    //    { CODE:ST }
                    //(* Body *)
                    //END_POU
                    skip_blanks();
                    if(i>=siz) throw dlg::error("{} not closed by {}", collecting.pou_start, collecting.pou_end);
                    else if( eat_line_end() ) continue;
                    else
                       {
                        switch(substatus)
                           {
                            case SUB::GET_HEADER :
                                if( is_directive() )
                                   {
                                    plcb::Directive dir = collect_directive();
                                    if( dir.key() == "DE"sv )
                                       {// Is a description
                                        if( !collecting.pous->back().descr().empty() ) notify_error("{} has already a description: {}", collecting.pou_start, collecting.pous->back().descr());
                                        collecting.pous->back().set_descr( dir.value() );
                                        //DBGLOG("    {} description: {}\n", collecting.pou_start, dir.value())
                                       }
                                    else if( dir.key() == "CODE"sv )
                                       {
                                        //if( dir.value() != "ST" ) notify_error("Code type: {} for {} {}", dir.value(), collecting.pou_start, collecting.pous->back().name());
                                        collecting.pous->back().set_code_type( dir.value() );
                                        substatus = SUB::GET_BODY;
                                       }
                                    else
                                       {
                                        notify_error("Unexpected directive \"{}\" in {} {}", dir.key(), collecting.pou_start, collecting.pous->back().name());
                                       }
                                   }
                                else if( eat_token("VAR_INPUT"sv) )
                                   {
                                    collecting.vars = &( collecting.pous->back().input_vars() );
                                    collecting.var_value_needed = false;
                                    substatus = SUB::GET_VARS;
                                   }
                                else if( eat_token("VAR_OUTPUT"sv) )
                                   {
                                    collecting.vars = &( collecting.pous->back().output_vars() );
                                    collecting.var_value_needed = false;
                                    substatus = SUB::GET_VARS;
                                   }
                                else if( eat_token("VAR_IN_OUT"sv) )
                                   {
                                    collecting.vars = &( collecting.pous->back().inout_vars() );
                                    collecting.var_value_needed = false;
                                    substatus = SUB::GET_VARS;
                                   }
                                else if( eat_token("VAR_EXTERNAL"sv) )
                                   {
                                    collecting.vars = &( collecting.pous->back().external_vars() );
                                    collecting.var_value_needed = false;
                                    substatus = SUB::GET_VARS;
                                   }
                                else if( eat_token("VAR"sv) )
                                   {
                                    // Check if there's some additional attributes
                                    skip_blanks();
                                    if( eat_token("CONSTANT"sv) )
                                       {
                                        collecting.vars = &( collecting.pous->back().local_constants() );
                                        collecting.var_value_needed = true;
                                        substatus = SUB::GET_VARS;
                                       }
                                    else if( eat_line_end() )
                                       {
                                        collecting.vars = &( collecting.pous->back().local_vars() );
                                        collecting.var_value_needed = false;
                                        substatus = SUB::GET_VARS;
                                       }
                                    else
                                       {
                                        throw dlg::error("Unexpected content in header of {} {}: {}", collecting.pou_start, collecting.pous->back().name(), str::escape(skip_line()));
                                       }
                                   }
                                else if( eat_token(collecting.pou_end) )
                                   {
                                    notify_error("Truncated {} {}", collecting.pou_start, collecting.pous->back().name());
                                    status = STS::SEE;
                                   }
                                else
                                   {
                                    notify_error("Unexpected content in {} {} header: {}", collecting.pou_start, collecting.pous->back().name(), str::escape(skip_line()));
                                   }
                                break;

                            case SUB::GET_VARS :
                                if( eat_token("END_VAR"sv) )
                                   {
                                    substatus = SUB::GET_HEADER;
                                   }
                                else if( eat_comment_start() )
                                   {// Nella lista variabili sono ammesse righe di commento
                                    skip_comment();
                                   }
                                else if( eat_token(collecting.pou_end) )
                                   {
                                    notify_error("Truncated vars in {} {}", collecting.pou_start, collecting.pous->back().name());
                                    status = STS::SEE;
                                   }
                                else
                                   {
                                    collecting.vars->push_back( collect_variable() );
                                    //DBGLOG("    {} variable \"{}\": {}\n", collecting.pou_start, collecting.vars->back().name(), collecting.vars->back().descr())
                                    // Check if a value was needed
                                    if( collecting.var_value_needed && !collecting.vars->back().has_value() ) throw dlg::error("Value not specified for variable \"{}\"", collecting.vars->back().name());
                                   }
                                break;

                            case SUB::GET_BODY :
                                collecting.pous->back().set_body( collect_until_newline_token(collecting.pou_end) );
                                //DBGLOG("    {} {} fully collected at line {}\n", collecting.pou_start, collecting.pous->back().name(), line)
                                status = STS::SEE;
                                break;
                           } // 'substatus'
                       }
                    break; // STS::POU

                // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - --
                case STS::MACRO : // Collecting macro definition
                    //MACRO IS_MSG
                    //
                    //{ DE:"Macro description" }
                    //
                    //    PAR_MACRO
                    //    WHAT; {DE:"Parameter description"}
                    //    END_PAR
                    //
                    //    { CODE:ST }
                    //(* Macro body *)
                    //END_MACRO
                    skip_blanks();
                    if(i>=siz) throw dlg::error("MACRO not closed by END_MACRO");
                    else if( eat_line_end() ) continue;
                    else
                       {
                        switch(substatus)
                           {
                            case SUB::GET_HEADER :
                                if( is_directive() )
                                   {
                                    plcb::Directive dir = collect_directive();
                                    if( dir.key() == "DE"sv )
                                       {// Is a description
                                        if( !lib.macros().back().descr().empty() ) notify_error("Macro {} has already a description: {}", lib.macros().back().name(), lib.macros().back().descr());
                                        lib.macros().back().set_descr( dir.value() );
                                        //DBGLOG("    Macro description: {}\n", dir.value())
                                       }
                                    else if( dir.key() == "CODE"sv )
                                       {
                                        //if( dir.value() != "ST" ) notify_error("Macro {} has code type: {}", lib.macros().back().name(), dir.value());
                                        lib.macros().back().set_code_type( dir.value() );
                                        substatus = SUB::GET_BODY;
                                       }
                                    else
                                       {
                                        notify_error("Unexpected directive \"{}\" in macro {} header", dir.key(), lib.macros().back().name());
                                       }
                                   }
                                else if( eat_token("PAR_MACRO"sv) )
                                   {
                                    if( !lib.macros().back().parameters().empty() ) notify_error("Multiple groups of macro parameters");
                                    substatus = SUB::GET_VARS;
                                   }
                                else if( eat_token("END_MACRO"sv) )
                                   {
                                    notify_error("Truncated macro");
                                    status = STS::SEE;
                                   }
                                else
                                   {
                                    notify_error("Unexpected content in header of macro {}: {}", lib.macros().back().name(), str::escape(skip_line()));
                                   }
                                break;

                            case SUB::GET_VARS :
                                if( eat_token("END_PAR"sv) )
                                   {
                                    substatus = SUB::GET_HEADER;
                                   }
                                else if( eat_token("END_MACRO"sv) )
                                   {
                                    notify_error("Truncated params in macro {}", lib.macros().back().name());
                                    status = STS::SEE;
                                   }
                                else
                                   {
                                    lib.macros().back().parameters().push_back( collect_parameter() );
                                    //DBGLOG("    Macro param {}: {}\n", lib.macros().back().parameters().back().name(), lib.macros().back().parameters().back().descr())
                                   }
                                break;

                            case SUB::GET_BODY :
                                lib.macros().back().set_body( collect_until_newline_token("END_MACRO"sv) );
                                //DBGLOG("    Macro {} fully collected at line {}\n", lib.macros().back().name(), line)
                                status = STS::SEE;
                                break;
                           } // 'substatus'
                       }
                    break; // STS::MACRO

                // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - --
                case STS::GLOBALVARS : // Collecting global variables
                    //    VAR_GLOBAL
                    //    {G:"System"}
                    //    Cnc : fbCncM32; { DE:"Cnc device" }
                    //    {G:"Arrays"}
                    //    vbMsgs AT %MB300.6000 : ARRAY[ 0..999 ] OF BOOL; { DE:"ivbMsgs Array messaggi attivati !MAX_MESSAGES!" }
                    //    END_VAR
                    skip_blanks();
                    if(i>=siz) throw dlg::error("VAR_GLOBAL not closed by END_VAR");
                    else if( eat_line_end() ) continue;
                    else if( is_directive() )
                       {
                        plcb::Directive dir = collect_directive();
                        if( dir.key() == "G" )
                           {// È la descrizione di un gruppo di variabili
                            if( dir.value().find(' ')!=std::string::npos ) notify_error("Avoid spaces in group name \"{}\"", dir.value());
                            collecting.gvars->groups().emplace_back();
                            collecting.gvars->groups().back().set_name( dir.value() );
                           }
                        else
                           {
                            notify_error("Unexpected directive \"{}\" in global vars", dir.key());
                           }
                       }
                    else if( eat_token("END_VAR"sv) )
                       {
                        //DBGLOG("    Global vars end at line {}\n", line)
                        status = STS::SEE;
                       }
                    else
                       {
                        if( collecting.gvars->groups().empty() ) collecting.gvars->groups().emplace_back(); // Unnamed group
                        collecting.gvars->groups().back().variables().push_back( collect_variable() );
                        //DBGLOG("    Variable {}: {}\n", collecting.gvars->groups().back().variables().back().name(), collecting.gvars->groups().back().variables().back().descr())
                        // Check if a value was needed
                        if( collecting.var_value_needed && !collecting.gvars->groups().back().variables().back().has_value() ) throw dlg::error("Value not specified for variable \"{}\"", collecting.gvars->groups().back().variables().back().name());
                       }
                    break; // STS::GLOBALVARS

                // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - --
                case STS::TYPE : // Collecting struct/typdef/enum
                    //    TYPE
                    //    ST_RECT : STRUCT { DE:"descr" } member : DINT; { DE:"member descr" } ... END_STRUCT;
                    //    VASTR : STRING[ 80 ]; { DE:"descr" }
                    //    EN_NAME: ( { DE:"descr" } VAL1 := 0, { DE:"elem descr" } ... );
                    //    END_TYPE
                    skip_blanks();
                    if(i>=siz) throw dlg::error("TYPE not closed by END_TYPE");
                    else if( eat_line_end() ) continue;
                    else if( eat_token("END_TYPE"sv) )
                       {
                        status = STS::SEE;
                       }
                    else
                       {// Expected a type name token here
                        std::string_view type_name = collect_identifier();
                        //DBGLOG("Found typename {} in line {}\n", type_name, line)
                        if( type_name.empty() )
                           {
                            notify_error("type name not found");
                            skip_line();
                           }
                        else
                           {
                            // Expected a colon after the name
                            skip_blanks();
                            if( i>=siz || buf[i]!=':' ) throw dlg::error("Missing \':\' after type name \"{}\"", type_name);
                            ++i; // Skip ':'
                            // Check what it is (struct, typedef, enum, subrange)
                            skip_blanks();
                            if(i>=siz) continue;
                            if( eat_token("STRUCT"sv) )
                               {// Struct: <name> : STRUCT
                                plcb::Struct strct;
                                strct.set_name(type_name);
                                collect_rest_of_struct( strct );
                                //DBGLOG("    struct {}, {} members\n", strct.name(), strct.members().size())
                                lib.structs().push_back( std::move(strct) );
                               }
                            else if( buf[i]=='(' )
                               {// Enum: <name>: (...
                                ++i; // Skip '('
                                plcb::Enum en;
                                en.set_name(type_name);
                                collect_rest_of_enum( en );
                                //DBGLOG("    enum {}, {} constants\n", en.name(), en.elements().size())
                                lib.enums().push_back( std::move(en) );
                               }
                            else
                               {// Could be a typedef or a subrange
                                // Unfortunately I have to peek to see which is
                                std::size_t j = i;
                                while(j<siz && buf[j]!=';' && buf[j]!='(' && buf[j]!='{' && buf[j]!='\n' ) ++j;
                                if(j<siz && buf[j]=='(' )
                                   {// Subrange: <name> : <type> (<min>..<max>); { DE:"descr" }
                                    plcb::Subrange subr;
                                    subr.set_name(type_name);
                                    collect_rest_of_subrange(subr);
                                   }
                                else
                                   {// Typedef: <name> : <type>; { DE:"descr" }
                                    plcb::Variable var;
                                    var.set_name(type_name);
                                    collect_rest_of_variable( var );
                                    //DBGLOG("    typedef {}: {}\n", var.name(), var.type())
                                    lib.typedefs().push_back( plcb::TypeDef(var) );
                                   }
                               }
                           }
                       }
                    break; // STS::TYPE

                // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - --
                case STS::HEADER : // Check my custom header for the library
                    skip_blanks();
                    if(i>=siz) continue;
                    else if( eat_line_end() ) continue;
                    else if( eat_comment_start() )
                       {// Could be my custom header
                        //(*
                        //    descr: "Wrapper to Sipro facilities (Macotec M-series machines)"
                        //    version: "0.5.0"
                        //    author: "©2017-2021 Matteo Gattanini"
                        //    dependencies: "defvar.pll"
                        //*)
                        std::size_t j = i; // Where comment content starts
                        skip_comment();
                        const std::size_t j_end = i-2; // Where comment content ends
                        // Check line by line
                        while( j < j_end )
                           {
                            // Get key
                            while( j<j_end && is_blank(buf[j]) ) ++j; // Skip blanks
                            std::size_t j_start = j;
                            while( j<j_end && std::isalnum(buf[j]) ) ++j;
                            const std::string_view key(buf.data()+j_start, j-j_start);
                            if( !key.empty() )
                               {
                                // Get separator
                                while( j<j_end && is_blank(buf[j]) ) ++j; // Skip blanks
                                if( buf[j]==':' )
                                   {// Get value
                                    ++j; // Skip ':'
                                    while( j<j_end && is_blank(buf[j]) ) ++j; // Skip blanks
                                    j_start = j;
                                    while( j<j_end && buf[j]!='\r' && buf[j]!='\n' ) ++j;
                                    const std::string_view val(buf.data()+j_start, j-j_start);
                                    if( !val.empty() )
                                       {
                                        //DBGLOG("    Found key/value {}:{}\n", key, val)
                                        if(key.starts_with("descr"sv)) lib.set_descr(val);
                                        else if(key=="version"sv) lib.set_version(val);
                                       }
                                   }
                               }
                            while( j<j_end && buf[j]!='\n' ) ++j; // Ensure to skip this line
                            ++j; // Skip '\n'
                           }
                       }
                    else
                       {
                        status = STS::SEE;
                       }
                    break; // STS::HEADER

               } // 'switch(status)'
           } // while there's data
       }
    catch(std::exception& e)
       {
        throw dlg::parse_error(e.what(), line, i);
       }
    #undef notify_error
}


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



//---- end unit -------------------------------------------------------------
#endif
