#ifndef GUARD_plc_elements_hpp
#define GUARD_plc_elements_hpp
/*  ---------------------------------------------
    ©2021-2022 matteo.gattanini@gmail.com

    OVERVIEW
    ---------------------------------------------
    IEC 61131-3 stuff and descriptors of PLC elements

    DEPENDENCIES:
    --------------------------------------------- */
#include <string>
#include <string_view>
#include <array>
#include <vector>
#include <algorithm> // std::sort, std::ranges::find
#include <cstdint> // std::uint16_t
#include <ctime> // std::time_t
#include <stdexcept> // std::runtime_error
#include <fmt/core.h> // fmt::format

#include "string-utilities.hpp" // str::as_num

using namespace std::literals; // "..."sv


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace plc //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

// Built in numeric types
static constexpr std::array<std::string_view,15> num_types =
   {
    "BOOL"sv,   // [1] BOOLean [FALSE|TRUE]
    "SINT"sv,   // [1] Short INTeger [-128 … 127]
    "INT"sv,    // [2] INTeger [-32768 … +32767]
    "DINT"sv,   // [4] Double INTeger [-2147483648 … 2147483647]
    "LINT"sv,   // [8] Long INTeger [-2^63 … 2^63-1]
    "USINT"sv,  // [1] Unsigned Short INTeger [0 … 255]
    "UINT"sv,   // [2] Unsigned INTeger [0 … 65535]
    "UDINT"sv,  // [4] Unsigned Double INTeger [0 … 4294967295]
    "ULINT"sv,  // [8] Unsigned Long INTeger [0 … 2^64-1]
    "REAL"sv,   // [4] REAL number [±10^38]
    "LREAL"sv,  // [8] Long REAL number [±10^308]
    "BYTE"sv,   // [1] 1 byte
    "WORD"sv,   // [2] 2 bytes
    "DWORD"sv,  // [4] 4 bytes
    "LWORD"sv   // [8] 8 bytes
   };

//---------------------------------------------------------------------------
// Tell if a string is a recognized numerical type
constexpr bool is_num_type(const std::string_view s)
   {
    return std::ranges::find(num_types, s) != num_types.end();
   }



/////////////////////////////////////////////////////////////////////////////
// Variable address ex. MB700.320
//                      M     B        700  .  320
//                      ↑     ↑        ↑       ↑
//                      type  typevar  index   subindex
class VariableAddress
{
 public:
     VariableAddress(const char typ ='\0', const char vtyp ='\0', const std::uint16_t idx =0, const std::uint16_t sub =0) noexcept
       : i_Type(typ), i_TypeVar(vtyp), i_Index(idx), i_SubIndex(sub) {}

    bool is_empty() const noexcept { return i_Type=='\0'; }

    char type() const noexcept { return i_Type; }
    void set_type(const char typ) noexcept { i_Type = typ; }

    char typevar() const noexcept { return i_TypeVar; }
    void set_typevar(const char vtyp) noexcept { i_TypeVar = vtyp; }

    std::uint16_t index() const noexcept { return i_Index; }
    void set_index(const std::uint16_t idx) noexcept { i_Index = idx; }
    void set_index(const std::string_view s)
       {
        auto idx = str::as_num<std::uint16_t>(s);
        if(!idx.has_value()) throw std::runtime_error(fmt::format("Invalid address index \"{}\"", s));
        i_Index = idx.value();
       }

    std::uint16_t subindex() const noexcept { return i_SubIndex; }
    void set_subindex(const std::uint16_t idx) noexcept { i_SubIndex = idx; }
    void set_subindex(const std::string_view s)
       {
        auto idx = str::as_num<std::uint16_t>(s);
        if(!idx.has_value()) throw std::runtime_error(fmt::format("Invalid address subindex \"{}\"", s));
        i_SubIndex = idx.value();
       }

 private:
    char i_Type; // Typically 'M'
    char i_TypeVar;
    std::uint16_t i_Index;
    std::uint16_t i_SubIndex;
};


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace buf //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{// Content that refers to an external buffer


/////////////////////////////////////////////////////////////////////////////
// A specific vendor directive
class Directive
{
 public:
    std::string_view key() const noexcept { return i_Key; }
    void set_key(const std::string_view s)
       {
        if(s.empty()) throw std::runtime_error("Empty directive key");
        i_Key = s;
       }

    std::string_view value() const noexcept { return i_Value; }
    void set_value(const std::string_view s) noexcept { i_Value = s; }

 private:
    std::string_view i_Key;
    std::string_view i_Value;
};



/////////////////////////////////////////////////////////////////////////////
// A variable declaration
class Variable
{
 public:
    std::string_view name() const noexcept { return i_Name; }
    void set_name(const std::string_view s)
       {
        if(s.empty()) throw std::runtime_error("Empty variable name");
        i_Name = s;
       }

    VariableAddress& address() noexcept { return i_Address; }
    const VariableAddress& address() const noexcept { return i_Address; }
    bool has_address() const noexcept { return !i_Address.is_empty(); }

    std::string_view type() const noexcept { return i_Type; }
    void set_type(const std::string_view s)
       {
        if(s.empty()) throw std::runtime_error("Empty variable type");
        i_Type = s;
       }

    bool has_length() const noexcept { return i_Length>0; }
    std::size_t length() const noexcept { return i_Length; }
    void set_length(const std::size_t n) { i_Length = n; }

    bool is_array() const noexcept { return i_ArrayDim>0; }
    std::size_t array_dim() const noexcept { return i_ArrayDim; }
    std::size_t array_startidx() const noexcept { return i_ArrayFirstIdx; }
    std::size_t array_lastidx() const noexcept { return i_ArrayFirstIdx + i_ArrayDim - 1u; } 
    void set_array_range(const std::size_t idx_start, const std::size_t idx_last)
       {
        if( idx_start >= idx_last ) throw std::runtime_error(fmt::format("Invalid array range {}..{} of variable \"{}\"", idx_start, idx_last, name()));
        //if( idx_start!=0u ) throw std::runtime_error(fmt::format("Invalid array start index {} of variable \"{}\"", start_idx, name()));
        i_ArrayFirstIdx = idx_start;
        i_ArrayDim = idx_last - idx_start + 1u;
       }

    std::string_view value() const noexcept { return i_Value; }
    void set_value(const std::string_view s)
       {
        if(s.empty()) throw std::runtime_error("Empty variable initialization value");
        i_Value = s;
       }
    bool has_value() const noexcept { return !i_Value.empty(); }

    std::string_view descr() const noexcept { return i_Descr; }
    void set_descr(const std::string_view s) noexcept { i_Descr = s; }
    bool has_descr() const noexcept { return !i_Descr.empty(); }

 private:
    std::string_view i_Name;
    VariableAddress i_Address;
    std::string_view i_Type;
    std::size_t i_Length = 0;
    std::size_t i_ArrayFirstIdx = 0,
                i_ArrayDim = 0;
    std::string_view i_Value;
    std::string_view i_Descr;
};



/////////////////////////////////////////////////////////////////////////////
// A named group of variables
class Variables_Group
{
 public:
    std::string_view name() const noexcept { return i_Name; }
    void set_name(const std::string_view s) noexcept { i_Name = s; }
    bool has_name() const noexcept { return !i_Name.empty(); }

    const std::vector<Variable>& variables() const noexcept { return i_Variables; }
    std::vector<Variable>& variables() noexcept { return i_Variables; }

    void sort()
       {
        std::sort(variables().begin(), variables().end(), [](const Variable& a, const Variable& b) noexcept -> bool { return a.name() < b.name(); });
       }

 private:
    std::string_view i_Name;
    std::vector<Variable> i_Variables;
};



/////////////////////////////////////////////////////////////////////////////
// A series of groups of variables
class Variables_Groups
{
 public:
    bool is_empty() const noexcept { return groups().empty(); }

    std::size_t size() const noexcept
       {
        std::size_t tot_siz = 0;
        for( const auto& group : groups() ) tot_siz += group.variables().size();
        return tot_siz;
       }

    bool has_named_group() const noexcept
       {
        for( const auto& group : groups() ) if(group.has_name()) return true;
        return false;
       }

    void sort()
       {
        std::sort(groups().begin(), groups().end(), [](const Variables_Group& a, const Variables_Group& b) noexcept -> bool { return a.name() < b.name(); });
       }

    const std::vector<Variables_Group>& groups() const noexcept { return i_Groups; }
    std::vector<Variables_Group>& groups() noexcept { return i_Groups; }

 private:
    std::vector<Variables_Group> i_Groups;
};



/////////////////////////////////////////////////////////////////////////////
// A struct
class Struct
{
 public:
    std::string_view name() const noexcept { return i_Name; }
    void set_name(const std::string_view s)
       {
        if(s.empty()) throw std::runtime_error("Empty struct name");
        i_Name = s;
       }

    std::string_view descr() const noexcept { return i_Descr; }
    void set_descr(const std::string_view s) noexcept { i_Descr = s; }
    //bool has_descr() const noexcept { return !i_Descr.empty(); }

    const std::vector<Variable>& members() const noexcept { return i_Members; }
    std::vector<Variable>& members() noexcept { return i_Members; }

 private:
    std::string_view i_Name;
    std::string_view i_Descr;
    std::vector<Variable> i_Members;
};



/////////////////////////////////////////////////////////////////////////////
// A type declaration
class TypeDef
{
 public:
    explicit TypeDef(const Variable& var) : i_Name(var.name()),
                                            i_Type(var.type()),
                                            i_Length(var.length()),
                                            i_ArrayFirstIdx(var.array_startidx()),
                                            i_ArrayDim(var.array_dim()),
                                            i_Descr(var.descr())
       {
        if( var.has_value() ) throw std::runtime_error(fmt::format("Typedef \"{}\" cannot have a value ({})", var.name(), var.value()));
        if( var.has_address() ) throw std::runtime_error(fmt::format("Typedef \"{}\" cannot have an address", var.name()));
        //if(i_Length>0) --i_Length; // WTF Ad un certo punto Axel ha deciso che nei typedef la dimensione è meno uno??
       }

    std::string_view name() const noexcept { return i_Name; }
    //void set_name(const std::string_view s)
    //   {
    //    if(s.empty()) throw std::runtime_error("Empty typedef name");
    //    i_Name = s;
    //   }

    std::string_view type() const noexcept { return i_Type; }
    //void set_type(const std::string_view s)
    //   {
    //    if(s.empty()) throw std::runtime_error("Empty typedef type");
    //    i_Type = s;
    //   }

    bool has_length() const noexcept { return i_Length>0; }
    std::size_t length() const noexcept { return i_Length; }

    bool is_array() const noexcept { return i_ArrayDim>0; }
    std::size_t array_dim() const noexcept { return i_ArrayDim; }
    std::size_t array_startidx() const noexcept { return i_ArrayFirstIdx; }
    std::size_t array_lastidx() const noexcept { return i_ArrayFirstIdx + i_ArrayDim - 1u; }

    std::string_view descr() const noexcept { return i_Descr; }
    void set_descr(const std::string_view s) noexcept { i_Descr = s; }
    //bool has_descr() const noexcept { return !i_Descr.empty(); }

 private:
    std::string_view i_Name;
    std::string_view i_Type;
    std::size_t i_Length = 0;
    std::size_t i_ArrayFirstIdx = 0,
                i_ArrayDim = 0;
    std::string_view i_Descr;
};



/////////////////////////////////////////////////////////////////////////////
// Enumeration definition
class Enum
{
 public:
    /////////////////////////////////////////////////////////////////////////
    class Element
       {
        public:
            std::string_view name() const noexcept { return i_Name; }
            void set_name(const std::string_view s)
                {
                 if(s.empty()) throw std::runtime_error("Empty enum constant name");
                 i_Name = s;
                }

            std::string_view value() const noexcept { return i_Value; }
            void set_value(const std::string_view s)
                {
                 if(s.empty()) throw std::runtime_error(fmt::format("Enum constant {} must have a value",name()));
                 i_Value = s;
                }

            std::string_view descr() const noexcept { return i_Descr; }
            void set_descr(const std::string_view s) noexcept { i_Descr = s; }
            //bool has_descr() const noexcept { return !i_Descr.empty(); }

        private:
            std::string_view i_Name;
            std::string_view i_Value;
            std::string_view i_Descr;
       };

    std::string_view name() const noexcept { return i_Name; }
    void set_name(const std::string_view s)
       {
        if(s.empty()) throw std::runtime_error("Empty enum name");
        i_Name = s;
       }

    std::string_view descr() const noexcept { return i_Descr; }
    void set_descr(const std::string_view s) noexcept { i_Descr = s; }

    const std::vector<Element>& elements() const noexcept { return i_Elements; }
    std::vector<Element>& elements() noexcept { return i_Elements; }

 private:
    std::string_view i_Name;
    std::string_view i_Descr;
    std::vector<Element> i_Elements;
};



/////////////////////////////////////////////////////////////////////////////
// A subrange declaration
class Subrange
{
 public:
    std::string_view name() const noexcept { return i_Name; }
    void set_name(const std::string_view s)
       {
        if(s.empty()) throw std::runtime_error("Empty subrange name");
        i_Name = s;
       }

    std::string_view type() const noexcept { return i_Type; }
    void set_type(const std::string_view s)
       {
        if(s.empty()) throw std::runtime_error("Empty subrange type");
        i_Type = s;
       }

    int min_value() const noexcept { return i_MinVal; }
    int max_value() const noexcept { return i_MaxVal; }
    void set_range(const int min_val, const int max_val)
       {
        if( max_val < min_val ) throw std::runtime_error(fmt::format("Invalid range {}..{} of subrange \"{}\"", min_val, max_val, name()));
        i_MinVal = min_val;
        i_MaxVal = max_val;
       }

    std::string_view descr() const noexcept { return i_Descr; }
    void set_descr(const std::string_view s) noexcept { i_Descr = s; }
    bool has_descr() const noexcept { return !i_Descr.empty(); }

 private:
    std::string_view i_Name;
    std::string_view i_Type;
    int i_MinVal = 0,
        i_MaxVal = 0;
    std::string_view i_Descr;
};



/////////////////////////////////////////////////////////////////////////////
// Generic Program Organization Unit (program, function_block, function)
class Pou
{
 public:
    std::string_view name() const noexcept { return i_Name; }
    void set_name(const std::string_view s)
       {
        if(s.empty()) throw std::runtime_error("Empty POU name");
        i_Name = s;
       }

    std::string_view descr() const noexcept { return i_Descr; }
    void set_descr(const std::string_view s) noexcept { i_Descr = s; }
    bool has_descr() const noexcept { return !i_Descr.empty(); }

    std::string_view return_type() const noexcept { return i_ReturnType; }
    void set_return_type(const std::string_view s) noexcept { i_ReturnType = s; }
    bool has_return_type() const noexcept { return !i_ReturnType.empty(); }

    const std::vector<Variable>& inout_vars() const noexcept { return i_InOutVars; }
    std::vector<Variable>& inout_vars() noexcept { return i_InOutVars; }

    const std::vector<Variable>& input_vars() const noexcept { return i_InputVars; }
    std::vector<Variable>& input_vars() noexcept { return i_InputVars; }

    const std::vector<Variable>& output_vars() const noexcept { return i_OutputVars; }
    std::vector<Variable>& output_vars() noexcept { return i_OutputVars; }

    const std::vector<Variable>& external_vars() const noexcept { return i_ExternalVars; }
    std::vector<Variable>& external_vars() noexcept { return i_ExternalVars; }

    const std::vector<Variable>& local_vars() const noexcept { return i_LocalVars; }
    std::vector<Variable>& local_vars() noexcept { return i_LocalVars; }

    const std::vector<Variable>& local_constants() const noexcept { return i_LocalConsts; }
    std::vector<Variable>& local_constants() noexcept { return i_LocalConsts; }

    std::string_view code_type() const noexcept { return i_CodeType; }
    void set_code_type(const std::string_view s) noexcept { i_CodeType = s; }

    std::string_view body() const noexcept { return i_Body; }
    void set_body(const std::string_view s) noexcept { i_Body = s; }


    void sort_variables()
       {
        std::sort(inout_vars().begin(), inout_vars().end(), [](const Variable& a, const Variable& b) noexcept -> bool { return a.name() < b.name(); });
        std::sort(input_vars().begin(), input_vars().end(), [](const Variable& a, const Variable& b) noexcept -> bool { return a.name() < b.name(); });
        std::sort(output_vars().begin(), output_vars().end(), [](const Variable& a, const Variable& b) noexcept -> bool { return a.name() < b.name(); });
        std::sort(external_vars().begin(), external_vars().end(), [](const Variable& a, const Variable& b) noexcept -> bool { return a.name() < b.name(); });
        std::sort(local_vars().begin(), local_vars().end(), [](const Variable& a, const Variable& b) noexcept -> bool { return a.name() < b.name(); });
        std::sort(local_constants().begin(), local_constants().end(), [](const Variable& a, const Variable& b) noexcept -> bool { return a.name() < b.name(); });
       }

 private:
    std::string_view i_Name;
    std::string_view i_Descr;
    std::string_view i_ReturnType;

    std::vector<Variable> i_InOutVars;
    std::vector<Variable> i_InputVars;
    std::vector<Variable> i_OutputVars;
    std::vector<Variable> i_ExternalVars;
    std::vector<Variable> i_LocalVars;
    std::vector<Variable> i_LocalConsts;

    std::string_view i_CodeType;
    std::string_view i_Body;
};



/////////////////////////////////////////////////////////////////////////////
// A macro expansion
class Macro
{
 public:
    /////////////////////////////////////////////////////////////////////////
    class Parameter
       {
        public:
            std::string_view name() const noexcept { return i_Name; }
            void set_name(const std::string_view s)
               {
                if(s.empty()) throw std::runtime_error("Empty parameter name");
                i_Name = s;
               }

            std::string_view descr() const noexcept { return i_Descr; }
            void set_descr(const std::string_view s) noexcept { i_Descr = s; }
            //bool has_descr() const noexcept { return !i_Descr.empty(); }

        private:
            std::string_view i_Name;
            std::string_view i_Descr;
       };

    std::string_view name() const noexcept { return i_Name; }
    void set_name(const std::string_view s)
       {
        if(s.empty()) throw std::runtime_error("Empty macro name");
        i_Name = s;
       }

    std::string_view descr() const noexcept { return i_Descr; }
    void set_descr(const std::string_view s) noexcept { i_Descr = s; }
    bool has_descr() const noexcept { return !i_Descr.empty(); }

    const std::vector<Parameter>& parameters() const noexcept { return i_Parameters; }
    std::vector<Parameter>& parameters() noexcept { return i_Parameters; }

    std::string_view code_type() const noexcept { return i_CodeType; }
    void set_code_type(const std::string_view s) noexcept { i_CodeType = s; }

    std::string_view body() const noexcept { return i_Body; }
    void set_body(const std::string_view s) noexcept { i_Body = s; }

 private:
    std::string_view i_Name;
    std::string_view i_Descr;
    std::vector<Parameter> i_Parameters;
    std::string_view i_CodeType;
    std::string_view i_Body;
};



/////////////////////////////////////////////////////////////////////////////
// The whole PLC library data aggregate
class Library
{
 public:
    explicit Library(const std::string& nam) noexcept
      : i_Name(nam)
      , i_Version("1.0.0")
      , i_Description("PLC library")
      //, i_CreatDate(0)
      //, i_ModifDate(0)
      {}

    const std::string& name() const noexcept { return i_Name; }

    const std::string& version() const noexcept { return i_Version; }
    void set_version(const std::string_view s) noexcept { i_Version = s; }

    const std::string& descr() const noexcept { return i_Description; }
    void set_descr(const std::string_view s) noexcept { i_Description = s; }
    //bool has_descr() const noexcept { return !i_Descr.empty(); }

    //std::time_t creat_date() const noexcept { return i_CreatDate; }
    //std::time_t modif_date() const noexcept { return i_ModifDate; }
    //void set_dates(const std::tuple<std::time_t, std::time_t>& tuple) noexcept { auto& [ ct, mt ] = tuple; i_CreatDate=ct; i_ModifDate=mt; }
    //void set_dates(const std::time_t ct, const std::time_t mt) noexcept { i_CreatDate=ct; i_ModifDate=mt; }


    const Variables_Groups& global_constants() const noexcept { return i_GlobalConst; }
    Variables_Groups& global_constants() noexcept { return i_GlobalConst; }

    const Variables_Groups& global_retainvars() const noexcept { return i_GlobalRetainVars; }
    Variables_Groups& global_retainvars() noexcept { return i_GlobalRetainVars; }

    const Variables_Groups& global_variables() const noexcept { return i_GlobalVars; }
    Variables_Groups& global_variables() noexcept { return i_GlobalVars; }


    const std::vector<Pou>& programs() const noexcept { return i_Programs; }
    std::vector<Pou>& programs() noexcept { return i_Programs; }

    const std::vector<Pou>& function_blocks() const noexcept { return i_FunctionBlocks; }
    std::vector<Pou>& function_blocks() noexcept { return i_FunctionBlocks; }

    const std::vector<Pou>& functions() const noexcept { return i_Functions; }
    std::vector<Pou>& functions() noexcept { return i_Functions; }

    const std::vector<Macro>& macros() const noexcept { return i_Macros; }
    std::vector<Macro>& macros() noexcept { return i_Macros; }

    const std::vector<Struct>& structs() const noexcept { return i_Structs; }
    std::vector<Struct>& structs() noexcept { return i_Structs; }

    const std::vector<TypeDef>& typedefs() const noexcept { return i_TypeDefs; }
    std::vector<TypeDef>& typedefs() noexcept { return i_TypeDefs; }

    const std::vector<Enum>& enums() const noexcept { return i_Enums; }
    std::vector<Enum>& enums() noexcept { return i_Enums; }

    const std::vector<Subrange>& subranges() const noexcept { return i_Subranges; }
    std::vector<Subrange>& subranges() noexcept { return i_Subranges; }

    //const std::vector<Interface>& interfaces() const noexcept { return i_Interfaces; }
    //std::vector<Interface>& interfaces() noexcept { return i_Interfaces; }

    bool is_empty() const noexcept
       {
        return     global_constants().size()==0
                && global_retainvars().size()==0
                && global_variables().size()==0
                && programs().empty()
                && function_blocks().empty()
                && functions().empty()
                && macros().empty()
                && structs().empty()
                && typedefs().empty()
                && enums().empty()
                && subranges().empty();
                // && interfaces().empty();
       }

    void check() const
       {
        // Global constants must have a value (already checked in parsing)
        for( const auto& consts_grp : global_constants().groups() )
           {
            for( const auto& cvar : consts_grp.variables() )
               {
                if( !cvar.has_value() ) throw std::runtime_error(fmt::format("Global constant \"{}\" has no value",cvar.name()));
               }
           }

        // Functions must have a return type and cannot have certain variables type
        for( const auto& funct : functions() )
           {
            if( !funct.has_return_type() ) throw std::runtime_error(fmt::format("Function \"{}\" has no return type",funct.name()));
            if( !funct.output_vars().empty() ) throw std::runtime_error(fmt::format("Function \"{}\" cannot have output variables",funct.name()));
            if( !funct.inout_vars().empty() ) throw std::runtime_error(fmt::format("Function \"{}\" cannot have in-out variables",funct.name()));
            if( !funct.external_vars().empty() ) throw std::runtime_error(fmt::format("Function \"{}\" cannot have external variables",funct.name()));
           }

        // Programs cannot have a return type and cannot have certain variables type
        for( const auto& prog : programs() )
           {
            if( prog.has_return_type() ) throw std::runtime_error(fmt::format("Program \"{}\" cannot have a return type",prog.name()));
            if( !prog.input_vars().empty() ) throw std::runtime_error(fmt::format("Program \"{}\" cannot have input variables",prog.name()));
            if( !prog.output_vars().empty() ) throw std::runtime_error(fmt::format("Program \"{}\" cannot have output variables",prog.name()));
            if( !prog.inout_vars().empty() ) throw std::runtime_error(fmt::format("Program \"{}\" cannot have in-out variables",prog.name()));
            if( !prog.external_vars().empty() ) throw std::runtime_error(fmt::format("Program \"{}\" cannot have external variables",prog.name()));
           }
       }

    void sort()
       {
        global_constants().sort();
        global_retainvars().sort();
        global_variables().sort();

        std::sort(programs().begin(), programs().end(), [](const Pou& a, const Pou& b) noexcept -> bool { return a.name() < b.name(); });
        std::sort(function_blocks().begin(), function_blocks().end(), [](const Pou& a, const Pou& b) noexcept -> bool { return a.name() < b.name(); });
        std::sort(functions().begin(), functions().end(), [](const Pou& a, const Pou& b) noexcept -> bool { return a.name() < b.name(); });
        std::sort(macros().begin(), macros().end(), [](const Macro& a, const Macro& b) noexcept -> bool { return a.name() < b.name(); });
        std::sort(typedefs().begin(), typedefs().end(), [](const TypeDef& a, const TypeDef& b) noexcept -> bool { return a.name() < b.name(); });
        std::sort(enums().begin(), enums().end(), [](const Enum& a, const Enum& b) noexcept -> bool { return a.name() < b.name(); });
        //std::sort(subranges().begin(), subranges().end(), [](const Subrange& a, const Subrange& b) noexcept -> bool { return a.name() < b.name(); });
        //std::sort(interfaces().begin(), interfaces().end(), [](const Interface& a, const Interface& b) noexcept -> bool { return a.name() < b.name(); });

        //for( auto& pou : programs() )        pou.sort_variables();
        //for( auto& pou : function_blocks() ) pou.sort_variables();
        //for( auto& pou : functions() )       pou.sort_variables();
       }

    std::string to_str() const noexcept
       {
        std::string s;
        s.reserve(256);
        s += "Library " + name();
        if( !global_constants().is_empty() ) s += ", " + std::to_string(global_constants().size()) + " global constants";
        if( !global_retainvars().is_empty() ) s += ", " + std::to_string(global_retainvars().size()) + " global retain variables";
        if( !global_variables().is_empty() ) s += ", " + std::to_string(global_variables().size()) + " global variables";
        if( !programs().empty() ) s += ", " + std::to_string(programs().size()) + " programs";
        if( !function_blocks().empty() ) s += ", " + std::to_string(function_blocks().size()) + " function blocks";
        if( !functions().empty() ) s += ", " + std::to_string(functions().size()) + " functions";
        if( !macros().empty() ) s += ", " + std::to_string(macros().size()) + " macros";
        if( !structs().empty() ) s += ", " + std::to_string(structs().size()) + " structs";
        if( !typedefs().empty() ) s += ", " + std::to_string(typedefs().size()) + " typedefs";
        if( !enums().empty() ) s += ", " + std::to_string(enums().size()) + " enums";
        //if( !subranges().empty() ) s += ", " + std::to_string(subranges().size()) + " subranges";
        //if( !interfaces().empty() ) s += ", " + std::to_string(interfaces().size()) + " interfaces";
        return s;
       }

    //---------------------------------------------------------------------------
    //void write(const sys::file_write& f) const
    //{
    //    f << "\nglobal vars:\n"sv;
    //    for( const auto& group : global_variables().groups() )
    //       {
    //        f << "    \""sv << group.name() << "\"\n"sv;
    //        for( const auto& var : group.variables() )
    //           {
    //            f << "        "sv << var.name() << '\n';
    //           }
    //       }
    //
    //    f << "\nFunctions:\n"sv;
    //    for( const auto& pou : functions() )
    //       {
    //        f << "    "sv << pou.name() << ':'  << pou.return_type() <<  '\n';
    //       }
    //
    //    f << "\nFunction blocks:\n"sv;
    //    for( const auto& pou : function_blocks() )
    //       {
    //        f << "    "sv << pou.name() << '\n';
    //       }
    //
    //    f << "\nPrograms:\n"sv;
    //    for( const auto& pou : programs() )
    //       {
    //        f << "    "sv << pou.name() << '\n';
    //       }
    //
    //    f << "\nmacros:\n"sv;
    //    for( const auto& macro : macros() )
    //       {
    //        f << "    "sv << macro.name() << '\n';
    //        for( const auto& par : macro.parameters() )
    //           {
    //            f << "        "sv << par.name() << '\n';
    //           }
    //       }
    //
    //    f << "\ntypedefs:\n"sv;
    //    for( const auto& tdef : typedefs() )
    //       {
    //        f << "    "sv << tdef.name() << '\n';
    //       }
    //
    //    f << "\nenums:\n"sv;
    //    for( const auto& en : enums() )
    //       {
    //        f << "    "sv << en.name() << '\n';
    //        for( const auto& elem : en.elements() )
    //           {
    //            f << "        "sv << elem.name() << '\n';
    //           }
    //       }
    //}

 private:
    std::string i_Name;
    std::string i_Version;
    std::string i_Description;
    //std::time_t i_CreatDate, i_ModifDate;

    Variables_Groups i_GlobalConst;
    Variables_Groups i_GlobalRetainVars;
    Variables_Groups i_GlobalVars;
    std::vector<Pou> i_Programs;
    std::vector<Pou> i_FunctionBlocks;
    std::vector<Pou> i_Functions;
    std::vector<Macro> i_Macros;
    std::vector<Struct> i_Structs;
    std::vector<TypeDef> i_TypeDefs;
    std::vector<Enum> i_Enums;
    std::vector<Subrange> i_Subranges;
    //std::vector<Interface> i_Interfaces;
};

}//:::: buf :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

}//:::: plc :::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::


namespace plcb = plc::buf; // Objects that refer to an external buffer


//---- end unit -------------------------------------------------------------
#endif
