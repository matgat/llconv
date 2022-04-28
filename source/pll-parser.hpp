#ifndef GUARD_pll_parser_hpp
#define GUARD_pll_parser_hpp
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

#include "basic-parser.hpp" // BasicParser
#include "plc-elements.hpp" // plcb::*

using namespace std::literals; // "..."sv


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace pll //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{


/////////////////////////////////////////////////////////////////////////////
class Parser final : public BasicParser
{
 public:
    Parser(const std::string_view b, std::vector<std::string>& sl, const bool f)
      : BasicParser(b,sl,f) {}


    //-----------------------------------------------------------------------
    void check_heading_comment(plcb::Library& lib)
       {
        while( i<siz )
           {
            skip_blanks();
            if( i>=siz )
               {// No more data!
                break;
               }
            else if( eat_line_end() )
               {// Skip empty line
                continue;
               }
            else if( eat_block_comment_start() )
               {// Could be my custom header
                //(*
                //    name: test
                //    descr: Libraries for strato machines (Macotec M-series machines)
                //    version: 0.5.0
                //    author: MG
                //    dependencies: Common.pll, defvar.pll, iomap.pll, messages.pll
                //*)
                std::size_t j = i; // Where comment content starts
                skip_block_comment();
                const std::size_t j_end = i-2; // Where comment content ends

                // Parse heading comment
                while( j < j_end )
                   {
                    // Get key
                    while( j<j_end && is_blank(buf[j]) ) ++j; // Skip blanks
                    std::size_t j_start = j;
                    while( j<j_end && std::isalnum(buf[j]) ) ++j;
                    const std::string_view key(buf+j_start, j-j_start);
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
                            const std::string_view val(buf+j_start, j-j_start);
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
               {// Heading comment not found
                break;
               }
           }
       }


    //-----------------------------------------------------------------------
    void collect_next(plcb::Library& lib)
       {
        skip_blanks();
        if( eat_line_end() )
           {
            return;
           }
        else if( eat_block_comment_start() )
           {
            skip_block_comment();
           }
        else if( eat_token("PROGRAM"sv) )
           {
            //DBGLOG("Found PROGRAM in line {}\n", line)
            auto& prg = lib.programs().emplace_back();
            collect_pou(prg, "PROGRAM"sv, "END_PROGRAM"sv);
           }
        else if( eat_token("FUNCTION_BLOCK"sv) )
           {
            //DBGLOG("Found FUNCTION_BLOCK in line {}\n", line)
            auto& fb = lib.function_blocks().emplace_back();
            collect_pou(fb, "FUNCTION_BLOCK"sv, "END_FUNCTION_BLOCK"sv);
           }
        else if( eat_token("FUNCTION"sv) )
           {
            //DBGLOG("Found FUNCTION in line {}\n", line)
            auto& fn = lib.functions().emplace_back();
            collect_pou(fn, "FUNCTION"sv, "END_FUNCTION"sv, true);
           }
        else if( eat_token("MACRO"sv) )
           {
            //DBGLOG("Found MACRO in line {}\n", line)
            auto& macro = lib.macros().emplace_back();
            collect_macro(macro);
           }
        else if( eat_token("TYPE"sv) )
           {// struct/typdef/enum/subrange
            //DBGLOG("Found TYPE in line {}\n", line)
            collect_type(lib);
           }
        else if( eat_token("VAR_GLOBAL"sv) )
           {
            //DBGLOG("Found VAR_GLOBAL in line {}\n", line)
            // Check if there's some additional attributes
            skip_blanks();
            if( eat_token("CONSTANT"sv) )
               {
                collect_global_vars( lib.global_constants().groups(), true );
               }
            else if( eat_token("RETAIN"sv) )
               {
                notify_error("RETAIN variables not supported");
               }
            else if( eat_line_end() )
               {
                collect_global_vars( lib.global_variables().groups() );
               }
            else
               {
                throw fmtstr::error("Unexpected content in VAR_GLOBAL declaration: {}", str::escape(skip_line()));
               }
           }
        else
           {
            notify_error("Unexpected content: {}", str::escape(skip_line()));
           }
       }


 private:

    //-----------------------------------------------------------------------
    [[nodiscard]] bool eat_block_comment_start() noexcept
       {
        if( i<(siz-1) && buf[i]=='(' && buf[i+1]=='*' )
           {
            i += 2; // Skip "(*"
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
            if( buf[i]=='*' && buf[i+1]==')' )
               {
                i += 2; // Skip "*)"
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
    [[nodiscard]] bool eat_directive_start() noexcept
       {// { DE:"some string" }
        if( i<siz && buf[i]=='{' )
           {
            ++i;
            return true;
           }
        return false;
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] plcb::Directive collect_directive()
       {// { DE:"some string" }
        // { CODE:ST }

        // Contract: already checked if( i>=siz || buf[i]!='{' ) { return; }

        //++i; // Skip '{'
        skip_blanks();
        plcb::Directive dir;
        dir.set_key( collect_identifier() );
        skip_blanks();
        if( i>=siz || buf[i]!=':' )
           {
            throw fmtstr::error("Missing \':\' after directive {}", dir.key());
           }
        ++i; // Skip ':'
        skip_blanks();
        if( i>=siz )
           {
            throw fmtstr::error("Truncated directive {}", dir.key());
           }

        if( buf[i]=='\"' )
           {
            ++i; // Skip the first '\"'
            //dir.set_value( collect_until_char_same_line('\"') );
            const std::size_t i_start = i;
            while( i<siz && buf[i]!='\"' )
               {
                if( buf[i]=='\n' )
                   {
                    throw fmtstr::error("Unclosed directive {} value (\'\"\' expected)", dir.key());
                   }
                else if( buf[i]=='<' || buf[i]=='>' || buf[i]=='\"' )
                   {
                    throw fmtstr::error("Invalid character \'{}\' in directive {} value", buf[i], dir.key());
                   }
                ++i;
               }
            dir.set_value( std::string_view(buf+i_start, i-i_start) );
            ++i; // Skip the second '\"'
           }
        else
           {
            dir.set_value( collect_identifier() );
           }
        skip_blanks();
        if( i>=siz || buf[i]!='}' )
           {
            throw fmtstr::error("Unclosed directive {} after {}", dir.key(), dir.value());
           }
        ++i; // Skip '}'
        //DBGLOG("    [*] Collected directive \"{}\" at line {}\n", dir.key(), line)
        return dir;
       }


    //-----------------------------------------------------------------------
    void collect_rest_of_struct(plcb::Struct& strct)
       {// <name> : STRUCT { DE:"struct descr" }
        //    x : DINT; { DE:"member descr" }
        //    y : DINT; { DE:"member descr" }
        //END_STRUCT;
        // Name already collected, "STRUCT" already skipped
        // Possible description
        skip_blanks();
        if( eat_directive_start() )
           {
            const plcb::Directive dir = collect_directive();
            if( dir.key() == "DE"sv ) strct.set_descr( dir.value() );
            else notify_error("Unexpected directive \"{}\" in struct \"{}\"", dir.key(), strct.name());
           }

        while( i<siz )
           {
            skip_empty_lines();
            if( eat("END_STRUCT;"sv) )
               {
                break;
               }
            else if( eat_line_end() )
               {// Nella lista membri ammetto righe vuote
                continue;
               }
            else if( eat_block_comment_start() )
               {// Nella lista membri ammetto righe di commento
                skip_block_comment();
                continue;
               }

            strct.members().push_back( collect_variable() );
            // Some checks
            //if( strct.members().back().has_value() )
            //   {
            //    throw fmtstr::error("Struct member \"{}\" cannot have a value ({})", strct.members().back().name(), strct.members().back().value());
            //   }
            if( strct.members().back().has_address() )
               {
                throw fmtstr::error("Struct member \"{}\" cannot have an address", strct.members().back().name());
               }
           }

        // Expecting a line end now
        check_if_line_ended_after("struct {}"sv, strct.name());
        //DBGLOG("    [*] Collected struct \"{}\", {} members\n", strct.name(), strct.members().size())
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] bool collect_enum_element(plcb::Enum::Element& elem)
       {// VAL1 := 0, { DE:"elem descr" }
        // [Name]
        skip_empty_lines();
        elem.set_name( collect_identifier() );

        // [Value]
        skip_blanks();
        if( !eat(":="sv) )
           {
            throw fmtstr::error("Value not found in enum element \"{}\"", elem.name());
           }
        skip_blanks();
        elem.set_value( collect_numeric_value() );
        skip_blanks();

        // Qui c'è una virgola se ci sono altri valori successivi
        bool has_next = i<siz && buf[i]==',';
        if( has_next ) ++i; // Skip comma

        // [Description]
        skip_blanks();
        if( eat_directive_start() )
           {
            const plcb::Directive dir = collect_directive();
            if( dir.key() == "DE"sv ) elem.set_descr( dir.value() );
            else notify_error("Unexpected directive \"{}\" in enum element \"{}\"", dir.key(), elem.name());
           }

        // Expecting a line end now
        check_if_line_ended_after("enum element {}"sv, elem.name());
        //DBGLOG("    [*] Collected enum element: name=\"{}\" value=\"{}\" descr=\"{}\"\n", elem.name(), elem.value(), elem.descr())
        return has_next;
       }


    //-----------------------------------------------------------------------
    void collect_rest_of_enum(plcb::Enum& en)
       {// <name>: ( { DE:"enum descr" }
        //     VAL1 := 0, { DE:"elem descr" }
        //     VAL2 := -1 { DE:"elem desc" }
        // );
        // Name already collected, ": (" already skipped
        // Possible description
        skip_blanks();
        eat_line_end(); // Possibile interruzione di linea
        skip_blanks();
        if( eat_directive_start() )
           {
            const plcb::Directive dir = collect_directive();
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
        if( !eat(");"sv) )
           {
            throw fmtstr::error("Expected termination \");\" after enum \"{}\"", en.name());
           }

        // Expecting a line end now
        check_if_line_ended_after("enum {}"sv, en.name());
        //DBGLOG("    [*] Collected enum \"{}\", {} elements\n", en.name(), en.elements().size())
       }


    //-----------------------------------------------------------------------
    void collect_rest_of_subrange(plcb::Subrange& subr)
       {// <name> : DINT (5..23); { DE:"descr" }
        // Name already collected, ':' already skipped

        // [Type]
        skip_blanks();
        subr.set_type(collect_identifier());

        // [Min and Max]
        skip_blanks();
        if( i>=siz || buf[i]!='(' )
           {
            throw fmtstr::error("Expected \"(min..max)\" in subrange \"{}\"", subr.name());
           }
        ++i; // Skip '('
        skip_blanks();
        const auto min_val = extract_integer(); // extract_double(); // Nah, Floating point numbers seems not supported
        skip_blanks();
        if( !eat(".."sv) )
           {
            throw fmtstr::error("Expected \"..\" in subrange \"{}\"", subr.name());
           }
        skip_blanks();
        const auto max_val = extract_integer();
        skip_blanks();
        if( i>=siz || buf[i]!=')' )
           {
            throw fmtstr::error("Expected \')\' in subrange \"{}\"", subr.name());
           }
        ++i; // Skip ')'
        skip_blanks();
        if( i>=siz || buf[i]!=';' )
           {
            throw fmtstr::error("Expected \';\' in subrange \"{}\"", subr.name());
           }
        ++i; // Skip ';'
        subr.set_range(min_val, max_val);

        // [Description]
        skip_blanks();
        if( eat_directive_start() )
           {
            const plcb::Directive dir = collect_directive();
            if(dir.key() == "DE"sv) subr.set_descr(dir.value());
            else notify_error("Unexpected directive \"{}\" in subrange \"{}\" declaration", dir.key(), subr.name());
           }

        // Expecting a line end now
        check_if_line_ended_after("subrange {}"sv, subr.name());
        //DBGLOG("    [*] Collected subrange: name=\"{}\" type=\"{}\" min=\"{}\" max=\"{}\" descr=\"{}\"\n", subr.name(), subr.type(), subr.min(), subr.max(), subr.descr())
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] plcb::Variable collect_variable()
       {//  VarName : Type := Val; { DE:"descr" }
        //  VarName AT %MB300.6000 : ARRAY[ 0..999 ] OF BOOL; { DE:"descr" }
        //  VarName AT %MB700.0 : STRING[ 80 ]; {DE:"descr"}
        plcb::Variable var;

        // [Name]
        skip_blanks();
        var.set_name( collect_identifier() );
        skip_blanks();
        if( i<siz && buf[i]==',' ) throw fmtstr::error("Multiple names not supported in declaration of variable \"{}\"", var.name());

        // [Location address]
        if( eat_token("AT"sv) )
           {// Specified a location address %<type><typevar><index>.<subindex>
            skip_blanks();
            if( i>=siz || buf[i]!='%' )
               {
                throw fmtstr::error("Expected \'%\' in variable \"{}\" address", var.name());
               }
            ++i; // Skip '%'
            // Here expecting something like: MB300.6000
            var.address().set_type( buf[i] ); i+=1; // In the Sipro/LogicLab world the address type is always 'M'
            var.address().set_typevar( buf[i] ); i+=1;
            var.address().set_index( collect_digits() );
            if( i>=siz || buf[i]!='.' )
               {
                throw fmtstr::error("Expected \'.\' in variable \"{}\" address", var.name());
               }
            ++i; // Skip '.'
            var.address().set_subindex( collect_digits() );
            skip_blanks();
           }

        // [Name/Type separator]
        if( i>=siz || buf[i]!=':' )
           {
            throw fmtstr::error("Expected \':\' before variable \"{}\" type", var.name());
           }
        ++i; // Skip ':'

        collect_rest_of_variable(var);
        return var;
       }


    //-----------------------------------------------------------------------
    void collect_rest_of_variable(plcb::Variable& var)
       {// ... STRING[ 80 ]; { DE:"descr" }
        // ... ARRAY[ 0..999 ] OF BOOL; { DE:"descr" }
        // ... ARRAY[1..2, 1..2] OF DINT := [1, 2, 3, 4]; { DE:"multidimensional array" }
        // [Array data]
        skip_blanks();
        if( eat_token("ARRAY"sv) )
           {// Specifying an array ex. ARRAY[ 0..999 ] OF BOOL;
            // Get array size
            skip_blanks();
            if( i>=siz || buf[i]!='[' )
               {
                throw fmtstr::error("Expected \'[\' in array variable \"{}\"", var.name());
               }
            ++i; // Skip '['
            skip_blanks();
            const std::size_t idx_start = extract_index();
            skip_blanks();
            if( !eat(".."sv) )
               {
                throw fmtstr::error("Expected \"..\" in array index of variable \"{}\"", var.name());
               }
            skip_blanks();
            const std::size_t idx_last = extract_index();
            skip_blanks();
            if( i<siz && buf[i]==',' )
               {
                throw fmtstr::error("Multidimensional arrays not yet supported in variable \"{}\"", var.name());
               }
            // TODO: Collect array dimensions (multidimensional: dim0, dim1, dim2)
            if( i>=siz || buf[i]!=']' )
               {
                throw fmtstr::error("Expected \']\' in array variable \"{}\"", var.name());
               }
            ++i; // Skip ']'
            skip_blanks();
            if( !eat_token("OF"sv) )
               {
                throw fmtstr::error("Expected \"OF\" in array variable \"{}\"", var.name());
               }
            var.set_array_range(idx_start, idx_last);
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
            if( len<=1u )
               {
                throw fmtstr::error("Invalid length ({}) of variable \"{}\"", len, var.name());
               }
            skip_blanks();
            if( i>=siz || buf[i]!=']' )
               {
                throw fmtstr::error("Expected \']\' in variable length \"{}\"", var.name());
               }
            ++i; // Skip ']'
            skip_blanks();
            var.set_length( len );
           }

        // [Value]
        if( buf[i]==':' )
           {
            ++i; // Skip ':'
            if( i>=siz || buf[i]!='=' )
               {
                throw fmtstr::error("Unexpected colon in variable \"{}\" type", var.name());
               }
            ++i; // Skip '='
            skip_blanks();
            if( i<siz && buf[i]=='[' )
               {
                throw fmtstr::error("Array initialization not yet supported in variable \"{}\"", var.name());
               }

            //var.set_value( collect_numeric_value() );
            const std::size_t i_start = i;
            std::size_t i_end = i_start; // Index past last char not blank
            while( i<siz )
               {
                if( buf[i]==';' )
                   {
                    var.set_value( std::string_view(buf+i_start, i_end-i_start) );
                    //DBGLOG("    [*] Collected var value \"{}\"\n", var.name())
                    ++i; // Skip ';'
                    break;
                   }
                else if( buf[i]=='\n' )
                   {
                    throw fmtstr::error("Unclosed variable \"{}\" value {} (\';\' expected)", var.name(), std::string_view(buf+i_start, i-i_start));
                   }
                else if( buf[i]==':' || buf[i]=='=' || buf[i]=='<' || buf[i]=='>' || buf[i]=='\"' )
                   {
                    throw fmtstr::error("Invalid character \'{}\' in variable \"{}\" value {}", buf[i], var.name(), std::string_view(buf+i_start, i-i_start));
                   }
                else
                   {// Collecting value
                    if( !is_blank(buf[i]) ) i_end = ++i;
                    else ++i;
                   }
               }
           }
        else if( buf[i] == ';' )
           {
            ++i; // Skip ';'
           }

        // [Description]
        skip_blanks();
        if( eat_directive_start() )
           {
            const plcb::Directive dir = collect_directive();
            if( dir.key() == "DE"sv )
               {
                var.set_descr( dir.value() );
               }
            else
               {
                notify_error("Unexpected directive \"{}\" in variable \"{}\" declaration", dir.key(), var.name());
               }
           }

        // Expecting a line end now
        check_if_line_ended_after("variable {} declaration"sv, var.name());
        //DBGLOG("    [*] Collected var: name=\"{}\" type=\"{}\" val=\"{}\" descr=\"{}\"\n", var.name(), var.type(), var.value(), var.descr())
       }


    //-----------------------------------------------------------------------
    void collect_var_block(std::vector<plcb::Variable>& vars, const bool value_needed =false)
       {
        while( i<siz )
           {
            skip_blanks();
            if( eat_token("END_VAR"sv) )
               {
                break;
               }
            else if( eat_line_end() )
               {// Sono ammesse righe vuote
                continue;
               }
            else if( eat_block_comment_start() )
               {// Nella lista variabili sono ammesse righe di commento
                skip_block_comment();
               }
            else
               {// Expected a variable entry
                vars.push_back( collect_variable() );
                //DBGLOG("  variable \"{}\": {}\n", vars.back().name(), vars.back().descr())
                // Check if a value was needed
                if( value_needed && !vars.back().has_value() )
                   {
                    throw fmtstr::error("Value not specified for var \"{}\"", vars.back().name());
                   }
               }
           }
       }


    //-----------------------------------------------------------------------
    void collect_pou_header(plcb::Pou& pou, const std::string_view start_tag, const std::string_view end_tag)
       {
        while( i<siz )
           {
            skip_blanks();
            if(i>=siz)
               {
                throw fmtstr::error("{} not closed by {}", start_tag, end_tag);
               }
            else if( eat_line_end() )
               {// Sono ammesse righe vuote
                continue;
               }
            else
               {
                if( eat_directive_start() )
                   {
                    const plcb::Directive dir = collect_directive();
                    if( dir.key() == "DE"sv )
                       {// Is a description
                        if( !pou.descr().empty() )
                           {
                            notify_error("{} has already a description: {}", start_tag, pou.descr());
                           }
                        pou.set_descr( dir.value() );
                        //DBGLOG("    {} description: {}\n", start_tag, dir.value())
                       }
                    else if( dir.key() == "CODE"sv )
                       {// Header finished
                        pou.set_code_type( dir.value() );
                        break;
                       }
                    else
                       {
                        notify_error("Unexpected directive \"{}\" in {} {}", dir.key(), start_tag, pou.name());
                       }
                   }
                else if( eat_token("VAR_INPUT"sv) )
                   {
                    check_if_line_ended_after("VAR_INPUT of {}"sv, pou.name());
                    collect_var_block( pou.input_vars() );
                   }
                else if( eat_token("VAR_OUTPUT"sv) )
                   {
                    check_if_line_ended_after("VAR_OUTPUT of {}"sv, pou.name());
                    collect_var_block( pou.output_vars() );
                   }
                else if( eat_token("VAR_IN_OUT"sv) )
                   {
                    check_if_line_ended_after("VAR_IN_OUT of {}"sv, pou.name());
                    collect_var_block( pou.inout_vars() );
                   }
                else if( eat_token("VAR_EXTERNAL"sv) )
                   {
                    check_if_line_ended_after("VAR_EXTERNAL of {}"sv, pou.name());
                    collect_var_block( pou.external_vars() );
                   }
                else if( eat_token("VAR"sv) )
                   {
                    // Check if there's some additional attributes
                    skip_blanks();
                    if( eat_token("CONSTANT"sv) )
                       {
                        check_if_line_ended_after("VAR CONSTANT of {}"sv, pou.name());
                        collect_var_block( pou.local_constants(), true );
                       }
                    //else if( eat_token("RETAIN"sv) )
                    //   {
                    //    notify_error("RETAIN variables not supported");
                    //   }
                    else if( eat_line_end() )
                       {
                        collect_var_block( pou.local_vars() );
                       }
                    else
                       {
                        throw fmtstr::error("Unexpected content after VAR of {} {}: {}", start_tag, pou.name(), str::escape(skip_line()));
                       }
                   }
                else if( eat_token(end_tag) )
                   {
                    notify_error("Truncated {} {}", start_tag, pou.name());
                    break;
                   }
                else
                   {
                    notify_error("Unexpected content in {} {} header: {}", start_tag, pou.name(), str::escape(skip_line()));
                   }
               }
           }
       }


    //-----------------------------------------------------------------------
    void collect_pou(plcb::Pou& pou, const std::string_view start_tag, const std::string_view end_tag, const bool needs_ret_type =false)
       {
        //POU NAME : RETURN_VALUE
        //{ DE:"Description" }
        //    VAR_YYY
        //    ...
        //    END_VAR
        //    { CODE:ST }
        //(* Body *)
        //END_POU

        //DBGLOG("Collecting {} in line {}\n", start_tag, line)

        // Get name
        skip_blanks();
        pou.set_name( collect_identifier() );
        if( pou.name().empty() )
           {
            throw fmtstr::error("No name found for {}", start_tag);
           }

        // Get possible return type
        // Dopo il nome dovrebbe esserci la dichiarazione del parametro ritornato
        skip_blanks();
        if( i<siz && buf[i]==':' )
           {// Got a return type!
            ++i; // Skip ':'
            skip_blanks();
            pou.set_return_type( collect_until_char_trimmed('\n') );
            if( pou.return_type().empty() )
               {
                throw fmtstr::error("Empty return type in {} {}", start_tag, pou.name());
               }
            if( !needs_ret_type )
               {
                throw fmtstr::error("Return type specified in {} {}", start_tag, pou.name());
               }
           }
        else
           {// No return type
            if( needs_ret_type )
               {
                throw fmtstr::error("Return type not specified in {} {}", start_tag, pou.name());
               }
           }

        // Collect description and variables
        collect_pou_header(pou, start_tag, end_tag);

        // Collect the code body
        if( pou.code_type().empty() )
           {
            throw fmtstr::error("CODE not found in {} {}", start_tag, pou.name());
           }
        //else if( pou.code_type() != "ST"sv )
        //   {
        //    issues.push_back(fmt::format("Code type: {} for {} {}", dir.value(), start_tag, pou.name()));
        //   }
        pou.set_body( collect_until_newline_token(end_tag) );
        //DBGLOG("    {} {} fully collected at line {}\n", start_tag, pou.name(), line)
       }


    //-----------------------------------------------------------------------
    [[nodiscard]] plcb::Macro::Parameter collect_macro_parameter()
       {//   WHAT; {DE:"Parameter description"}
        plcb::Macro::Parameter par;
        skip_blanks();
        par.set_name( collect_identifier() );
        skip_blanks();
        if( i>=siz || buf[i]!=';' )
           {
            throw fmtstr::error("Missing \';\' after macro parameter");
           }
        ++i; // Skip ';'
        skip_blanks();
        if( eat_directive_start() )
           {
            const plcb::Directive dir = collect_directive();
            if( dir.key() == "DE"sv ) par.set_descr( dir.value() );
            else notify_error("Unexpected directive \"{}\" in macro parameter", dir.key());
           }

        // Expecting a line end now
        check_if_line_ended_after("macro parameter {}"sv, par.name());

        return par;
       }


    //-----------------------------------------------------------------------
    void collect_macro_parameters(std::vector<plcb::Macro::Parameter>& pars)
       {
        while( i<siz )
           {
            skip_blanks();
            if( eat_token("END_PAR"sv) )
               {
                break;
               }
            else if( eat_line_end() )
               {// Sono ammesse righe vuote
                continue;
               }
            else if( eat_block_comment_start() )
               {// Ammetto righe di commento?
                skip_block_comment();
               }
            else if( eat_token("END_MACRO"sv) )
               {
                notify_error("Truncated params in macro");
                break;
               }
            else
               {
                pars.push_back( collect_macro_parameter() );
                //DBGLOG("    Macro param {}: {}\n", pars.back().name(), pars.back().descr())
               }
           }
       }


    //-----------------------------------------------------------------------
    void collect_macro_header(plcb::Macro& macro)
       {
        while( i<siz )
           {
            skip_blanks();
            if(i>=siz)
               {
                throw fmtstr::error("MACRO not closed by END_MACRO");
               }
            else if( eat_line_end() )
               {// Sono ammesse righe vuote
                continue;
               }
            else
               {
                if( eat_directive_start() )
                   {
                    const plcb::Directive dir = collect_directive();
                    if( dir.key() == "DE"sv )
                       {// Is a description
                        if( !macro.descr().empty() ) notify_error("Macro {} has already a description: {}", macro.name(), macro.descr());
                        macro.set_descr( dir.value() );
                        //DBGLOG("    Macro description: {}\n", dir.value())
                       }
                    else if( dir.key() == "CODE"sv )
                       {// Header finished
                        macro.set_code_type( dir.value() );
                        break;
                       }
                    else
                       {
                        notify_error("Unexpected directive \"{}\" in macro {} header", dir.key(), macro.name());
                       }
                   }
                else if( eat_token("PAR_MACRO"sv) )
                   {
                    if( !macro.parameters().empty() )
                       {
                        notify_error("Multiple groups of macro parameters");
                       }
                    check_if_line_ended_after("PAR_MACRO of {}"sv, macro.name());
                    collect_macro_parameters( macro.parameters() );
                   }
                else if( eat_token("END_MACRO"sv) )
                   {
                    notify_error("Truncated macro");
                    break;
                   }
                else
                   {
                    notify_error("Unexpected content in header of macro {}: {}", macro.name(), str::escape(skip_line()));
                   }
               }
           }
       }


    //-----------------------------------------------------------------------
    void collect_macro(plcb::Macro& macro)
       {
        //MACRO IS_MSG
        //{ DE:"Macro description" }
        //    PAR_MACRO
        //    WHAT; { DE:"Parameter description" }
        //    END_PAR
        //    { CODE:ST }
        //(* Macro body *)
        //END_MACRO

        // Get name
        skip_blanks();
        macro.set_name( collect_identifier() );
        if( macro.name().empty() )
           {
            throw fmtstr::error("No name found for MACRO");
           }

        collect_macro_header(macro);

        // Collect the code body
        if( macro.code_type().empty() )
           {
            throw fmtstr::error("CODE not found in MACRO {}", macro.name());
           }
        //else if( macro.code_type() != "ST"sv )
        //   {
        //    issues.push_back(fmt::format("Code type: {} for MACRO {}", dir.value(), macro.name()));
        //   }
        macro.set_body( collect_until_newline_token("END_MACRO"sv) );
       }


    //-----------------------------------------------------------------------
    void collect_global_vars(std::vector<plcb::Variables_Group>& vgroups, const bool value_needed =false)
       {
        //    VAR_GLOBAL
        //    {G:"System"}
        //    Cnc : fbCncM32; { DE:"Cnc device" }
        //    {G:"Arrays"}
        //    vbMsgs AT %MB300.6000 : ARRAY[ 0..999 ] OF BOOL; { DE:"ivbMsgs Array messaggi attivati !MAX_MESSAGES!" }
        //    END_VAR
        while( i<siz )
           {
            skip_blanks();
            if( i>=siz )
               {
                throw fmtstr::error("VAR_GLOBAL not closed by END_VAR");
               }
            else if( eat_line_end() )
               {// Nella lista variabili sono ammesse righe vuote
               }
            else if( eat_block_comment_start() )
               {// Nella lista variabili sono ammesse righe di commento
                skip_block_comment();
               }
            else if( eat_directive_start() )
               {
                const plcb::Directive dir = collect_directive();
                if( dir.key() == "G" )
                   {// È la descrizione di un gruppo di variabili
                    if( dir.value().find(' ')!=std::string::npos )
                       {
                        notify_error("Avoid spaces in var group name \"{}\"", dir.value());
                       }
                    vgroups.emplace_back();
                    vgroups.back().set_name( dir.value() );
                   }
                else
                   {
                    notify_error("Unexpected directive \"{}\" in global vars", dir.key());
                   }
               }
            else if( eat_token("END_VAR"sv) )
               {
                //DBGLOG("    Global vars end at line {}\n", line)
                break;
               }
            else
               {
                if( vgroups.empty() ) vgroups.emplace_back(); // Unnamed group
                vgroups.back().variables().push_back( collect_variable() );
                //DBGLOG("    Variable {}: {}\n", vgroups.back().variables().back().name(), vgroups.back().variables().back().descr())
                // Check if a value was needed
                if( value_needed && !vgroups.back().variables().back().has_value() )
                   {
                    throw fmtstr::error("Value not specified for variable \"{}\"", vgroups.back().variables().back().name());
                   }
               }
           }
       }


    //-----------------------------------------------------------------------
    void collect_type(plcb::Library& lib)
       {
        //    TYPE
        //    str_name : STRUCT { DE:"descr" } member : DINT; { DE:"member descr" } ... END_STRUCT;
        //    typ_name : STRING[ 80 ]; { DE:"descr" }
        //    en_name: ( { DE:"descr" } VAL1 := 0, { DE:"elem descr" } ... );
        //    subr_name : DINT (30..100);
        //    END_TYPE
        while( i<siz )
           {
            skip_blanks();
            if( i>=siz )
               {
                throw fmtstr::error("TYPE not closed by END_TYPE");
               }
            else if( eat_line_end() )
               {
                continue;
               }
            else if( eat_token("END_TYPE"sv) )
               {
                break;
               }
            else
               {// Expected a type name token here
                const std::string_view type_name = collect_identifier();
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
                    if( i>=siz || buf[i]!=':' )
                       {
                        throw fmtstr::error("Missing \':\' after type name \"{}\"", type_name);
                       }
                    ++i; // Skip ':'
                    // Check what it is (struct, typedef, enum, subrange)
                    skip_blanks();
                    if(i>=siz) continue;
                    if( eat_token("STRUCT"sv) )
                       {// <name> : STRUCT
                        plcb::Struct strct;
                        strct.set_name(type_name);
                        collect_rest_of_struct( strct );
                        //DBGLOG("    struct {}, {} members\n", strct.name(), strct.members().size())
                        lib.structs().push_back( std::move(strct) );
                       }
                    else if( buf[i]=='(' )
                       {// <name>: ( { DE:"an enum" }
                        ++i; // Skip '('
                        plcb::Enum en;
                        en.set_name(type_name);
                        collect_rest_of_enum( en );
                        //DBGLOG("    enum {}, {} constants\n", en.name(), en.elements().size())
                        lib.enums().push_back( std::move(en) );
                       }
                    else
                       {// Could be a typedef or a subrange
                        // I have to peek to see which one
                        std::size_t j = i;
                        while(j<siz && buf[j]!=';' && buf[j]!='(' && buf[j]!='{' && buf[j]!='\n' ) ++j;
                        if(j<siz && buf[j]=='(' )
                           {// <name> : <type> (<min>..<max>); { DE:"a subrange" }
                            plcb::Subrange subr;
                            subr.set_name(type_name);
                            collect_rest_of_subrange(subr);
                            //DBGLOG("    subrange {}: {}\n", var.name(), var.type())
                            lib.subranges().push_back( std::move(subr) );
                           }
                        else
                           {// <name> : <type>; { DE:"a typedef" }
                            plcb::Variable var;
                            var.set_name(type_name);
                            collect_rest_of_variable( var );
                            //DBGLOG("    typedef {}: {}\n", var.name(), var.type())
                            lib.typedefs().push_back( plcb::TypeDef(var) );
                           }
                       }
                   }
               }
           }
       }
};



//---------------------------------------------------------------------------
// Parse pll file
void parse(const std::string_view buf, plcb::Library& lib, std::vector<std::string>& issues, const bool fussy)
{
    Parser parser(buf, issues, fussy);

    try{
        parser.check_heading_comment(lib);
        while( parser.end_not_reached() )
           {
            //EVTLOG("main loop: offset:{} char:{}", i, buf[i])
            parser.collect_next(lib);
           }
       }
    catch(fmtstr::parse_error&)
       {
        throw;
       }
    catch(std::exception& e)
       {
        throw fmtstr::parse_error(e.what(), parser.curr_line(), parser.curr_pos());
       }
}


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

//---- end unit -------------------------------------------------------------
#endif
