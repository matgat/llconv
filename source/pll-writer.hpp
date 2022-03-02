#ifndef GUARD_pll_writer_hpp
#define GUARD_pll_writer_hpp
/*  ---------------------------------------------
    ©2022 matteo.gattanini@gmail.com

    OVERVIEW
    ---------------------------------------------
    LogicLab5 'pll' format

    DEPENDENCIES:
    --------------------------------------------- */
#include <string>
#include <string_view>
#include <cstdint> // uint16_t, uint32_t
#include <limits> // std::numeric_limits
#include <cctype> // std::isdigit
//#include <type_traits> // std::enable_if, std::is_unsigned
#include <stdexcept> // std::exception, std::runtime_error, ...
#include <fmt/core.h> // fmt::format

//#include "string-utilities.hpp" // str::to_num
#include "plc-elements.hpp" // plcb::*
#include "system.hpp" // sys::*, fs::*

using namespace std::string_view_literals; // Use "..."sv


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace pll
{


//---------------------------------------------------------------------------
// Properly quote a description
//std::string& quoted(std::string& s) noexcept
//   {
//    std::string::size_type i = 0;
//    while( i<s.size() )
//       {
//        switch( s[i] )
//           {
//            case '\n': s.replace(i, 2, "\\n"); i+=2; break;
//            case '\r': s.replace(i, 2, "\\r"); i+=2; break;
//            case '\t': s.replace(i, 2, "\\t"); i+=2; break;
//            //case '\f': s.replace(i, 2, "\\f"); i+=2; break;
//            //case '\v': s.replace(i, 2, "\\v"); i+=2; break;
//            //case '\0': s.replace(i, 2, "\\0"); i+=2; break;
//            default : ++i;
//           }
//       }
//    return s;
//   }


//---------------------------------------------------------------------------
// Write variable to pll file
inline void write(const sys::file_write& f, const plcb::Variable& var)
{
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

    assert( !var.name().empty() );

    f<< "\t"sv << var.name(); // Exception!!

    if( var.has_address() )
       {
        f<< " AT %"sv << var.address().type()
                      << var.address().typevar()
                      << std::to_string(var.address().index())
                      << "."sv
                      << std::to_string(var.address().subindex());
       }

    f<< " : "sv;

    if( var.has_length() )
       {// STRING[ 80 ]
        f<< var.type() << "[ "sv << std::to_string(var.length()) << " ]"sv;
       }
    else if( var.is_array() )
       {// ARRAY[ 0..999 ] OF BOOL
        f<< "ARRAY[ 0.."sv << std::to_string(var.arraydim()-1u) << " ] OF "sv << var.type();
       }
    else
       {// DINT
        f<< var.type();
       }

    if( var.has_value() )
       {
        f<< " := "sv << var.value();
       }

    f<< ";"sv;

    if( var.has_descr() )
       {
        f<< " { DE:\""sv << var.descr() << "\" }\n"sv;
       }
}


//---------------------------------------------------------------------------
// Write library to pll file
void write(const sys::file_write& f, const plcb::Library& lib, [[maybe_unused]] const str::keyvals& options)
{
    // [Options]
    //auto xxx = options.value_of("xxx");

    // [Heading]
    f<< "(*\n"sv
     << "    name: "sv << lib.name() << "\n"sv
     << "    descr: "sv << lib.descr() << "\n"sv
     << "    version: "sv << lib.version() << "\n"sv
     << "    author: pll::write()\n"sv
     << "    date: "sv << sys::human_readable_time_stamp() << "\n"sv
     << "*)\n\n\n"sv;

    // [Content summary]
    //f<< "(*\n"sv
    // << "    global constants: "sv << std::to_string(lib.global_constants().size())  << "\n"sv
    // << "    global retain vars: "sv << std::to_string(lib.global_retainvars().size())  << "\n"sv
    // << "    global variables: "sv << std::to_string(lib.global_variables().size())  << "\n"sv
    // << "    function blocks: "sv << std::to_string(lib.function_blocks().size())  << "\n"sv
    // << "    functions: "sv << std::to_string(lib.functions().size())  << "\n"sv
    // << "    programs: "sv << std::to_string(lib.programs().size())  << "\n"sv
    // << "    macros: "sv << std::to_string(lib.macros().size())  << "\n"sv
    // << "    structs: "sv << std::to_string(lib.structs().size())  << "\n"sv
    // << "    typedefs: "sv << std::to_string(lib.typedefs().size())  << "\n"sv
    // << "    enums: "sv << std::to_string(lib.enums().size())  << "\n"sv
    // << "    subranges: "sv << std::to_string(lib.subranges().size())  << "\n"sv
    // //<< "    interfaces: "sv << std::to_string(lib.interfaces().size())  << "\n"sv
    // << "*)\n"sv;

    // [Global variables]
    if( !lib.global_variables().is_empty() )
       {
        f<< "\t(****************************)\n"
            "\t(*                          *)\n"
            "\t(*     GLOBAL VARIABLES     *)\n"
            "\t(*                          *)\n"
            "\t(****************************)\n"
            "\n"
            "\tVAR_GLOBAL\n"sv;
        for( const auto& group : lib.global_variables().groups() )
           {
            if( !group.name().empty() ) f << "\t{G:\""sv << group.name() << "\"}\n"sv;
            for( const auto& var : group.variables() ) write(f, var);
           }
        for( const auto& group : lib.global_retainvars().groups() )
           {
            if( !group.name().empty() ) f << "\t{G:\""sv << group.name() << "\"}\n"sv;
            for( const auto& var : group.variables() ) write(f, var);
           }
        f<< "\tEND_VAR\n\n\n"sv;
       }

    // [Global constants]
    if( !lib.global_constants().is_empty() )
       {
        f<< "\t(****************************)\n"
            "\t(*                          *)\n"
            "\t(*     GLOBAL CONSTANTS     *)\n"
            "\t(*                          *)\n"
            "\t(****************************)\n"
            "\n"
            "\tVAR_GLOBAL CONSTANT\n"sv;
        for( const auto& group : lib.global_constants().groups() )
           {
            if( !group.name().empty() ) f << "\t{G:\""sv << group.name() << "\"}\n"sv;
            for( const auto& var : group.variables() ) write(f, var);
           }
        f<< "\tEND_VAR\n\n\n"sv;
       }

    // [Functions]
    if( !lib.functions().empty() )
       {
        throw std::runtime_error("pll::write(): functions not yet supported");
        //for( const auto& pou : lib.functions() ) write(f, pou, "function"sv);
       }

    // [FunctionBlocks]
    if( !lib.function_blocks().empty() )
       {
        throw std::runtime_error("pll::write(): function blocks not yet supported");
        //for( const auto& pou : lib.function_blocks() ) write(f, pou, "functionBlock"sv);
       }

    // [Programs]
    if( !lib.programs().empty() )
       {
        throw std::runtime_error("pll::write(): programs not yet supported");
        //for( const auto& pou : lib.programs() ) write(f, pou, "program"sv,);
       }

    // [Macros]
    if( !lib.macros().empty() )
       {
        throw std::runtime_error("pll::write(): macros not yet supported");
        //for( const auto& macro : lib.macros() ) write(f, macro,);
       }

    // [Structs]
    if( !lib.structs().empty() )
       {
        throw std::runtime_error("pll::write(): structs not yet supported");
        //for( const auto& strct : lib.structs() )
        //   {
        //    f<< "\t"sv << strct.name() << " "sv << strct.descr() << "\n"sv;
        //    for( const auto& var : strct.members() )
        //       {
        //        //f<< "\t"sv << var.name() << " "sv << var.type() << " "sv << var.descr() << "\n"sv
        //       }
        //   }
       }

    // [Typedefs]
    if( !lib.typedefs().empty() )
       {
        throw std::runtime_error("pll::write(): structs not yet supported");
        //for( const auto& tdef : lib.typedefs() )
        //   {
        //    f<< "\t"sv << tdef.name() << " "sv << tdef.type() << "\""sv;
        //    if( tdef.has_length() ) f<< " "sv << std::to_string(tdef.length()) << "\n"sv;
        //    if( tdef.is_array() ) f<< " "sv << std::to_string(tdef.arraydim()) << "\n"sv;
        //    f<< " "sv << tdef.descr() << "\n"sv;
        //   }
       }

    // [Enums]
    if( !lib.enums().empty() )
       {
        throw std::runtime_error("pll::write(): enums not yet supported");
        //for( const auto& en : lib.enums() )
        //   {
        //    //f<< "\t"sv << en.name() << ""sv << en.descr() << "\n"sv;
        //    for( const auto& elem : en.elements() )
        //       {
        //        f<< "\t"sv << elem.name() << " "sv << elem.descr() << " "sv << elem.value() << "\n"sv
        //       }
        //   }
       }

    // [Subranges]
    if( !lib.subranges().empty() )
       {
        throw std::runtime_error("pll::write(): subranges not yet supported");
        //for( const auto& subr : lib.subranges() )
        //   {
        //    f<< "\t"sv << subr.name() << " "sv << subr.type() << " "sv << subr.descr() << "\n"sv;
        //     << " "sv << std::to_string(subr.min_value()) << " "sv << std::to_string(subr.max_value()) << "\n"sv;
        //   }
       }

    // [Interfaces]
    //if( !lib.interfaces().empty() )
    //   {
    //    throw std::runtime_error("pll::write(): interfaces not yet supported");
    //    //for( const auto& intfc : lib.interfaces() ) write(f, intfc);
    //   }
}


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



//---- end unit -------------------------------------------------------------
#endif
