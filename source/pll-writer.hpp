#ifndef pll_writer_hpp
#define pll_writer_hpp
/*  ---------------------------------------------
    ©2022 matteo.gattanini@gmail.com

    OVERVIEW
    ---------------------------------------------
    LogicLab5 'pll' xml format

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
// Write variable to pll file
inline void write(const sys::file_write& f, const plcb::Variable& var, const std::string_view tag, const std::string_view ind)
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

    //f<< ind << "<"sv << tag << " name=\""sv << var.name() << "\" type=\""sv << var.type() << "\""sv;

    //if( var.has_length() ) f<< " length=\""sv << std::to_string(var.length()) << "\""sv;
    //if( var.is_array() ) f<< " dim0=\""sv << std::to_string(var.arraydim()) << "\""sv;
    //
    //if( var.has_descr() || var.has_value() || var.has_address() )
    //   {// Tag contains something
    //    f<< ">\n"sv;
    //
    //    if( var.has_descr() )
    //       {
    //        f<< ind << "\t<descr>"sv << var.descr() << "</descr>\n"sv;
    //       }
    //
    //    if( var.has_value() )
    //       {
    //        f<< ind << "\t<initValue>"sv << var.value() << "</initValue>\n"sv;
    //       }
    //
    //    if( var.has_address() )
    //       {
    //        f<< ind << "\t<address type=\""sv << var.address().type()
    //         <<                "\" typeVar=\""sv << var.address().typevar()
    //         <<                "\" index=\""sv << var.address().index()
    //         <<                "\" subIndex=\""sv << var.address().subindex()
    //         <<                "\"/>\n"sv;
    //       }
    //
    //    f<< ind << "</"sv << tag << ">\n"sv;
    //   }
    //else
    //   {// Empty tag
    //    f<< "/>\n"sv;
    //   }
}


//---------------------------------------------------------------------------
// Write library to pll file
void write(const sys::file_write& f, const plcb::Library& lib, const str::keyvals& options)
{
}


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



//---- end unit -------------------------------------------------------------
#endif
