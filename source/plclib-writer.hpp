#ifndef plclib_writer_hpp
#define plclib_writer_hpp
/*  ---------------------------------------------
    ©2021-2022 matteo.gattanini@gmail.com

    OVERVIEW
    ---------------------------------------------
    LogicLab5 'plclib' xml format

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

#include "string-utilities.hpp" // str::to_num, str::hash
#include "keyvals.hpp" // str::keyvals
#include "plc-elements.hpp" // plcb::*
#include "system.hpp" // sys::*, fs::*

using namespace std::string_view_literals; // Use "..."sv


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace plclib //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{


/////////////////////////////////////////////////////////////////////////////
// A class to hold schema version <major-ver>.<minor-ver>
class Version
{
 public:
    constexpr Version(const uint32_t v =0x00020008) noexcept : i_ver(v) {}

    void operator=(const std::string_view s)
       {
        try{
            const std::size_t siz = s.size();
            std::size_t i = 0;
            std::size_t i_start = i;
            while( i<siz && std::isdigit(s[i]) ) ++i;
            const uint16_t majv = str::to_num<uint16_t>( std::string_view(s.data()+i_start, i-i_start) );
            if( i>=siz || s[i]!='.' ) throw std::runtime_error("Missing \'.\' after major version");
            ++i; // Skip '.'
            i_start = i;
            while( i<siz && std::isdigit(s[i]) ) ++i;
            if( i<siz ) throw std::runtime_error("Unexpected content after minor version");
            const uint16_t minv = str::to_num<uint16_t>( std::string_view(s.data()+i_start, i-i_start) );
            set_version(majv, minv);
           }
        catch(std::exception& e)
           {
            throw std::runtime_error( fmt::format("\"{}\" is not a valid version: {}"sv, s, e.what()) );
           }
       }

    uint16_t major_version() const noexcept { return static_cast<uint16_t>(i_ver >> (8 * sizeof(uint16_t))); }
    uint16_t minor_version() const noexcept { return static_cast<uint16_t>(i_ver); }
    //uint16_t major_version() const noexcept { return (i_ver >> 8*sizeof(uint16_t)) & std::numeric_limits<uint16_t>::max; }
    //uint16_t minor_version() const noexcept { return i_ver & std::numeric_limits<uint16_t>::max; }

    void set_version(const uint16_t majv, const uint16_t minv) noexcept {i_ver = combine(majv,minv); }
    //void set_major_version(const uint16_t majv) noexcept {i_ver = combine(majv,minor_version()); }
    //void set_minor_version(const uint16_t minv) noexcept {i_ver = combine(major_version(),minv); }

    auto operator<=>(const Version&) const noexcept = default;

    std::string to_str() const { return fmt::format("{}.{}"sv, major_version(), minor_version()); }

 private:
    uint32_t i_ver;
    //constinit static const uint32_t i_mask = std::numeric_limits<uint16_t>::max();

    constexpr uint32_t combine(const uint16_t majv, const uint16_t minv) { return static_cast<uint32_t>(majv) << (8*sizeof(uint16_t)) | static_cast<uint32_t>(minv); }
};



//---------------------------------------------------------------------------
// Write variable to plclib file
inline void write(const sys::file_write& f, const plcb::Variable& var, const std::string_view tag, const std::string_view ind)
{
    f<< ind << "<"sv << tag << " name=\""sv << var.name() << "\" type=\""sv << var.type() << "\""sv;

    if( var.has_length() ) f<< " length=\""sv << std::to_string(var.length()) << "\""sv;
    if( var.is_array() ) f<< " dim0=\""sv << std::to_string(var.arraydim()) << "\""sv;

    if( var.has_descr() || var.has_value() || var.has_address() )
       {// Tag contains something
        f<< ">\n"sv;

        if( var.has_descr() )
           {
            f<< ind << "\t<descr>"sv << var.descr() << "</descr>\n"sv;
           }

        if( var.has_value() )
           {
            f<< ind << "\t<initValue>"sv << var.value() << "</initValue>\n"sv;
           }

        if( var.has_address() )
           {
            f<< ind << "\t<address type=\""sv << var.address().type()
             <<                "\" typeVar=\""sv << var.address().typevar()
             <<                "\" index=\""sv << std::to_string(var.address().index())
             <<                "\" subIndex=\""sv << std::to_string(var.address().subindex())
             <<                "\"/>\n"sv;
           }

        f<< ind << "</"sv << tag << ">\n"sv;
       }
    else
       {// Empty tag
        f<< "/>\n"sv;
       }
}


//---------------------------------------------------------------------------
// Write POU to plclib file
inline void write(const sys::file_write& f, const plcb::Pou& pou, const std::string_view tag, const std::string_view ind)
{
    f<< ind << "<"sv << tag << " name=\""sv << pou.name() << "\" version=\"1.0.0\" creationDate=\"0\" lastModifiedDate=\"0\" excludeFromBuild=\"FALSE\" excludeFromBuildIfNotDef=\"\">\n"sv;
    if( pou.has_descr() )
       {
        f<< ind << "\t<descr>"sv << pou.descr() << "</descr>\n"sv;
       }
    if( pou.has_return_type() )
       {
        f<< ind << "\t<returnValue>"sv << pou.return_type() << "</returnValue>\n"sv;
       }

    // [Variables]
    f<< ind << "\t<vars>\n"sv;
    if( !pou.inout_vars().empty() )
       {
        f<< ind << "\t\t<inoutVars>\n"sv;
        for( const auto& var : pou.inout_vars() ) write(f, var, "var"sv, "\t\t\t\t\t\t"sv); // TODO: use ind
        f<< ind << "\t\t</inoutVars>\n"sv;
       }
    if( !pou.input_vars().empty() )
       {
        f<< ind << "\t\t<inputVars>\n"sv;
        for( const auto& var : pou.input_vars() ) write(f, var, "var"sv, "\t\t\t\t\t\t"sv); // TODO: use ind
        f<< ind << "\t\t</inputVars>\n"sv;
       }
    if( !pou.output_vars().empty() )
       {
        f<< ind << "\t\t<outputVars>\n"sv;
        for( const auto& var : pou.output_vars() ) write(f, var, "var"sv, "\t\t\t\t\t\t"sv); // TODO: use ind
        f<< ind << "\t\t</outputVars>\n"sv;
       }
    if( !pou.external_vars().empty() )
       {
        f<< ind << "\t\t<externalVars>\n"sv;
        for( const auto& var : pou.external_vars() ) write(f, var, "var"sv, "\t\t\t\t\t\t"sv); // TODO: use ind
        f<< ind << "\t\t</externalVars>\n"sv;
       }
    if( !pou.local_vars().empty() )
       {
        f<< ind << "\t\t<localVars>\n"sv;
        for( const auto& var : pou.local_vars() ) write(f, var, "var"sv, "\t\t\t\t\t\t"sv); // TODO: use ind
        f<< ind << "\t\t</localVars>\n"sv;
       }
    if( !pou.local_constants().empty() )
       {
        f<< ind << "\t\t<localConsts>\n"sv;
        for( const auto& var : pou.local_constants() ) write(f, var, "const"sv, "\t\t\t\t\t\t"sv); // TODO: use ind
        f<< ind << "\t\t</localConsts>\n"sv;
       }
    f<< ind << "\t</vars>\n"sv;

    f<< ind << "\t<iecDeclaration active=\"FALSE\"/>\n"sv;
    if( tag=="functionBlock"sv )
       {
        f<< ind << "\t<interfaces/>\n"sv
         << ind << "\t<methods/>\n"sv;
       }

    // [Body]
    f<< ind << "\t<sourceCode type=\""sv << pou.code_type() << "\">\n"sv
     << ind << "\t\t<![CDATA["sv << pou.body() << "]]>\n"sv
     << ind << "\t</sourceCode>\n"sv;

    f<< ind << "</"sv << tag << ">\n"sv;
}


//---------------------------------------------------------------------------
// Write macro to plclib file
inline void write(const sys::file_write& f, const plcb::Macro& macro, const std::string_view ind)
{
    f<< ind << "<macro name=\""sv << macro.name() << "\">\n"sv;
    if( macro.has_descr() )
       {
        f<< ind << "\t<descr>"sv << macro.descr() << "</descr>\n"sv;
       }

    // [Body]
    f<< ind << "\t<sourceCode type=\""sv << macro.code_type() << "\">\n"sv
     << ind << "\t\t<![CDATA["sv << macro.body() << "]]>\n"sv
     << ind << "\t</sourceCode>\n"sv;

    // [Parameters]
    if( macro.parameters().empty() )
       {
        f<< ind << "\t<parameters/>\n"sv;
       }
    else
       {
        f<< ind << "\t<parameters>\n"sv;
        for( const auto& par : macro.parameters() )
           {
            f<< ind << "\t\t<parameter name=\""sv << par.name() << "\">\n"sv
             << ind << "\t\t\t<descr>"sv << par.descr() << "</descr>\n"sv
             << ind << "\t\t</parameter>\n"sv;
           }
        f<< ind << "\t</parameters>\n"sv;
       }
    f<< ind << "</macro>\n"sv;
}


//---------------------------------------------------------------------------
// Write library to plclib file
void write(const sys::file_write& f, const plcb::Library& lib, const str::keyvals& options)
{
    // . Check options
    // Get possible schema version
    Version schema_ver;
    auto schema_ver_str = options.value_of("schema-ver");
    if( schema_ver_str.has_value() )
       {
        schema_ver = schema_ver_str.value();
       }

    // [Heading]
    f << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n"sv
        << "<plcLibrary schemaVersion=\""sv << schema_ver.to_str() << "\">\n"sv
        << "\t<lib version=\""sv << lib.version() << "\" name=\""sv << lib.name() << "\" fullXml=\"true\">\n"sv
        << "\t\t<descr>"sv << lib.descr() << "</descr>\n"sv;

    // [Workspace]
    f<< "\t\t<libWorkspace>\n"sv;
    f<< "\t\t\t<folder name=\""sv << lib.name() << "\" id=\""sv << std::to_string(str::hash(lib.name())) << "\">\n"sv;
        for( const auto& grp : lib.global_constants().groups() ) if( grp.has_name() )  f<< "\t\t\t\t<GlobalVars name=\""sv << grp.name() << "\"/>\n"sv;
        for( const auto& grp : lib.global_retainvars().groups() ) if( grp.has_name() ) f<< "\t\t\t\t<GlobalVars name=\""sv << grp.name() << "\"/>\n"sv;
        for( const auto& grp : lib.global_variables().groups() ) if( grp.has_name() )  f<< "\t\t\t\t<GlobalVars name=\""sv << grp.name() << "\"/>\n"sv;
        for( const auto& pou : lib.function_blocks() ) f<< "\t\t\t\t<Pou name=\""sv << pou.name() << "\"/>\n"sv;
        for( const auto& pou : lib.functions() )       f<< "\t\t\t\t<Pou name=\""sv << pou.name() << "\"/>\n"sv;
        for( const auto& pou : lib.programs() )        f<< "\t\t\t\t<Pou name=\""sv << pou.name() << "\"/>\n"sv;
        // Definitions
        for( const auto& def : lib.macros() )   f<< "\t\t\t\t<Definition name=\""sv << def.name() << "\"/>\n"sv;
        for( const auto& def : lib.structs() )  f<< "\t\t\t\t<Definition name=\""sv << def.name() << "\"/>\n"sv;
        for( const auto& def : lib.typedefs() ) f<< "\t\t\t\t<Definition name=\""sv << def.name() << "\"/>\n"sv;
        for( const auto& def : lib.enums() )    f<< "\t\t\t\t<Definition name=\""sv << def.name() << "\"/>\n"sv;
        for( const auto& def : lib.subranges() ) f<< "\t\t\t\t<Definition name=\""sv << def.name() << "\"/>\n"sv;
        //for( const auto& def : lib.interfaces() ) f<< "\t\t\t\t<Definition name=\""sv << def.name() << "\"/>\n"sv;
    f<< "\t\t\t</folder>\n"sv;
    f<< "\t\t</libWorkspace>\n"sv;

    // [Global variables]
    if( !lib.global_variables().is_empty() )
       {
        f<< "\t\t<globalVars>\n"sv;
        for( const auto& group : lib.global_variables().groups() )
           {
            f << "\t\t\t<group name=\""sv << group.name() << "\" excludeFromBuild=\"FALSE\" excludeFromBuildIfNotDef=\"\" version=\"1.0.0\">\n"sv;
            for( const auto& var : group.variables() ) write(f, var, "var"sv, "\t\t\t\t"sv);
            f<< "\t\t\t</group>\n"sv;
           }
        f<< "\t\t</globalVars>\n"sv;
       }
    else
       {
        f<< "\t\t<globalVars/>\n"sv;
       }

    // [Global retain variables]
    if( !lib.global_retainvars().is_empty() )
       {
        f<< "\t\t<retainVars>\n"sv;
        for( const auto& group : lib.global_retainvars().groups() )
           {
            f << "\t\t\t<group name=\""sv << group.name() << "\" excludeFromBuild=\"FALSE\" excludeFromBuildIfNotDef=\"\" version=\"1.0.0\">\n"sv;
            for( const auto& var : group.variables() ) write(f, var, "var"sv, "\t\t\t\t"sv);
            f<< "\t\t\t</group>\n"sv;
           }
        f<< "\t\t</retainVars>\n"sv;
       }
    else
       {
        f<< "\t\t<retainVars/>\n"sv;
       }

    // [Global constants]
    if( !lib.global_constants().is_empty() )
       {
        f<< "\t\t<constantVars>\n"sv;
        for( const auto& group : lib.global_constants().groups() )
           {
            f << "\t\t\t<group name=\""sv << group.name() << "\" excludeFromBuild=\"FALSE\" excludeFromBuildIfNotDef=\"\" version=\"1.0.0\">\n"sv;
            for( const auto& var : group.variables() ) write(f, var, "const"sv, "\t\t\t\t"sv);
            f<< "\t\t\t</group>\n"sv;
           }
        f<< "\t\t</constantVars>\n"sv;
       }
    else
       {
        f<< "\t\t<constantVars/>\n"sv;
       }

    // [Global variables groups]
    if( lib.global_constants().has_named_group() ||
        lib.global_retainvars().has_named_group() ||
        lib.global_variables().has_named_group() )
       {
        f << "\t\t<iecVarsDeclaration>\n"sv;

        for( const auto& group : lib.global_constants().groups() )
           {
            if( group.has_name() )
               {
                f<< "\t\t\t<group name=\""sv << group.name() << "\">\n"sv
                 << "\t\t\t\t<iecDeclaration active=\"FALSE\"/>\n"sv
                 << "\t\t\t</group>\n"sv;
               }
           }

        for( const auto& group : lib.global_retainvars().groups() )
           {
            if( group.has_name() )
               {
                f<< "\t\t\t<group name=\""sv << group.name() << "\">\n"sv
                 << "\t\t\t\t<iecDeclaration active=\"FALSE\"/>\n"sv
                 << "\t\t\t</group>\n"sv;
               }
           }

        for( const auto& group : lib.global_variables().groups() )
           {
            if( group.has_name() )
               {
                f<< "\t\t\t<group name=\""sv << group.name() << "\">\n"sv
                 << "\t\t\t\t<iecDeclaration active=\"FALSE\"/>\n"sv
                 << "\t\t\t</group>\n"sv;
               }
           }

        f<< "\t\t</iecVarsDeclaration>\n"sv;
       }
    //else
    //   {
    //    f<< "\t\t<iecVarsDeclaration/>\n"sv;
    //   }

    const std::string_view ind{"\t\t\t"sv};

    // [Functions]
    if( !lib.functions().empty() )
       {
        f<< "\t\t<functions>\n"sv;
        for( const auto& pou : lib.functions() ) write(f, pou, "function"sv, ind);
        f<< "\t\t</functions>\n"sv;
       }
    else
       {
        f<< "\t\t<functions/>\n"sv;
       }

    // [FunctionBlocks]
    if( !lib.function_blocks().empty() )
       {
        f<< "\t\t<functionBlocks>\n"sv;
        for( const auto& pou : lib.function_blocks() ) write(f, pou, "functionBlock"sv, ind);
        f<< "\t\t</functionBlocks>\n"sv;
       }
    else
       {
        f<< "\t\t<functionBlocks/>\n"sv;
       }

    // [Programs]
    if( !lib.programs().empty() )
       {
        f<< "\t\t<programs>\n"sv;
        for( const auto& pou : lib.programs() ) write(f, pou, "program"sv, ind);
        f<< "\t\t</programs>\n"sv;
       }
    else
       {
        f<< "\t\t<programs/>\n"sv;
       }

    // [Macros]
    if( !lib.macros().empty() )
       {
        f<< "\t\t<macros>\n"sv;
        for( const auto& macro : lib.macros() ) write(f, macro, ind);
        f<< "\t\t</macros>\n"sv;
       }
    else
       {
        f<< "\t\t<macros/>\n"sv;
       }

    // [Structs]
    if( !lib.structs().empty() )
       {
        f<< "\t\t<structs>\n"sv;
        for( const auto& strct : lib.structs() )
           {
            f<< ind << "<struct name=\""sv << strct.name() << "\" version=\"1.0.0\">\n"sv;
            f<< ind << "\t<descr>"sv << strct.descr() << "</descr>\n"sv;
            f<< ind << "\t<vars>\n"sv;
            for( const auto& var : strct.members() )
               {
                f<< ind << "\t\t<var name=\""sv << var.name() << "\" type=\""sv << var.type() << "\">\n"sv
                 << ind << "\t\t\t<descr>"sv << var.descr() << "</descr>\n"sv
                 << ind << "\t\t</var>\n"sv;
               }
            f<< ind << "\t</vars>\n"sv;
            f<< ind << "\t<iecDeclaration active=\"FALSE\"/>\n"sv;
            f<< ind << "</struct>\n"sv;
           }
        f<< "\t\t</structs>\n"sv;
       }
    else
       {
        f<< "\t\t<structs/>\n"sv;
       }

    // [Typedefs]
    if( !lib.typedefs().empty() )
       {
        f<< "\t\t<typedefs>\n"sv;
        for( const auto& tdef : lib.typedefs() )
           {
            f<< ind << "<typedef name=\""sv << tdef.name() << "\" type=\""sv << tdef.type() << "\""sv;
            if( tdef.has_length() ) f<< " length=\""sv << std::to_string(tdef.length()) << "\""sv;
            if( tdef.is_array() ) f<< " dim0=\""sv << std::to_string(tdef.arraydim()) << "\""sv;
            f<< ">\n"sv;
            f<< ind << "\t<iecDeclaration active=\"FALSE\"/>\n"sv;
            f<< ind << "\t<descr>"sv << tdef.descr() << "</descr>\n"sv;
            f<< ind << "</typedef>\n"sv;
           }
        f<< "\t\t</typedefs>\n"sv;
       }
    else
       {
        f<< "\t\t<typedefs/>\n"sv;
       }

    // [Enums]
    if( !lib.enums().empty() )
       {
        f<< "\t\t<enums>\n"sv;
        for( const auto& en : lib.enums() )
           {
            f<< ind << "<enum name=\""sv << en.name() << "\" version=\"1.0.0\">\n"sv;
            f<< ind << "\t<descr>"sv << en.descr() << "</descr>\n"sv;
            f<< ind << "\t<elements>\n"sv;
            for( const auto& elem : en.elements() )
               {
                f<< ind << "\t\t<element name=\""sv << elem.name() << "\">\n"sv
                 << ind << "\t\t\t<descr>"sv << elem.descr() << "</descr>\n"sv
                 << ind << "\t\t\t<value>"sv << elem.value() << "</value>\n"sv
                 << ind << "\t\t</element>\n"sv;
               }
            f<< ind << "\t</elements>\n"sv;
            f<< ind << "\t<iecDeclaration active=\"FALSE\"/>\n"sv;
            f<< ind << "</enum>\n"sv;
           }
        f<< "\t\t</enums>\n"sv;
       }
    else
       {
        f<< "\t\t<enums/>\n"sv;
       }

    // [Subranges]
    if( !lib.subranges().empty() )
       {
        f<< "\t\t<subranges>\n"sv;
        for( const auto& subr : lib.subranges() )
           {
            f<< ind << "<subrange name=\""sv << subr.name() << "\" version=\"1.0.0\" type=\""sv << subr.type() << "\">"sv;
            //f<< ind << "\t<title>"sv << subr.title() << "</title>\n"sv;
            f<< ind << "\t<descr>"sv << subr.descr() << "</descr>\n"sv;
            f<< ind << "\t<minValue>"sv << std::to_string(subr.min_value()) << "</minValue>\n"sv;
            f<< ind << "\t<maxValue>"sv << std::to_string(subr.max_value()) << "</maxValue>\n"sv;
            f<< ind << "\t<iecDeclaration active=\"FALSE\"/>\n"sv;
            f<< ind << "</subrange>\n"sv;
           }
        f<< "\t\t</subranges>\n"sv;
       }
    else
       {
        f<< "\t\t<subranges/>\n"sv;
       }

    // [Interfaces]
    //if( !lib.interfaces().empty() )
    //   {
    //    f<< "\t\t<interfaces>\n"sv;
    //    for( const auto& intfc : lib.interfaces() ) write(f, intfc, "interface"sv);
    //    f<< "\t\t</interfaces>\n"sv;
    //   }
    //else
    //   {
          f<< "\t\t<interfaces/>\n"sv;
    //   }

    // [Closing]
    f<< "\t</lib>\n"sv
     << "</plcLibrary>\n"sv;
}



//---------------------------------------------------------------------------
//void list(const sys::file_write& f, const plcb::Library& lib, const int ver)
//{
//    f << "\nglobal vars:\n"sv;
//    for( const auto& group : lib.global_variables().groups() )
//       {
//        f << "    \""sv << group.name() << "\"\n"sv;
//        for( const auto& var : group.variables() )
//           {
//            f << "        "sv << var.name() << '\n';
//           }
//       }
//
//    f << "\nPrograms:\n"sv;
//    for( const auto& pou : lib.programs() )
//       {
//        f << "    "sv << pou.name() << '\n';
//       }
//
//    f << "\nFunction blocks:\n"sv;
//    for( const auto& pou : lib.function_blocks() )
//       {
//        f << "    "sv << pou.name() << '\n';
//       }
//
//    f << "\nFunctions:\n"sv;
//    for( const auto& pou : lib.functions() )
//       {
//        f << "    "sv << pou.name() << ':'  << pou.return_type() <<  '\n';
//       }
//
//    f << "\ntypedefs:\n"sv;
//    for( const auto& tdef : lib.typedefs() )
//       {
//        f << "    "sv << tdef.name() << '\n';
//       }
//
//    f << "\nenums:\n"sv;
//    for( const auto& en : lib.enums() )
//       {
//        f << "    "sv << en.name() << '\n';
//        for( const auto& elem : en.elements() )
//           {
//            f << "        "sv << elem.name() << '\n';
//           }
//       }
//
//    f << "\nmacros:\n"sv;
//    for( const auto& macro : lib.macros() )
//       {
//        f << "    "sv << macro.name() << '\n';
//        for( const auto& par : macro.parameters() )
//           {
//            f << "        "sv << par.name() << '\n';
//           }
//       }
//}


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



//---- end unit -------------------------------------------------------------
#endif
