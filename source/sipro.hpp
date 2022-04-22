#ifndef GUARD_sipro_hpp
#define GUARD_sipro_hpp
/*  ---------------------------------------------
    ©2022 matteo.gattanini@gmail.com

    OVERVIEW
    ---------------------------------------------
    Collects Sipro stuff

    DEPENDENCIES:
    --------------------------------------------- */
#include <array>
#include <string_view>
#include <cstdint> // std::uint16_t
#include <charconv> // std::from_chars


  //#if !defined(__cpp_lib_to_underlying)
  //  template<typename E> constexpr auto to_underlying(const E e) noexcept
  //     {
  //      return static_cast<std::underlying_type_t<E>>(e);
  //     }
  //#else
  //  using std::to_underlying;
  //#endif


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace sipro
{

/////////////////////////////////////////////////////////////////////////////
class Register final
{
 private:
    enum en_regtype : uint8_t
       {
        type_none=0, // !Consecutive indexes!
        type_vb,
        type_vn,
        type_vq,
        type_vd,
        type_va
       };

    static constexpr std::array<std::string_view, type_va+1u>
    reg_iec_types =
       {
        ""sv,      // none
        "BOOL"sv,  // vb
        "INT"sv,   // vn
        "DINT"sv,  // vq
        "LREAL"sv, // vd
        "STRING"sv // va
       };

    static constexpr std::array<char, type_va+1u>
    plc_var_type =
       {
        '\0', // none
        'B',  // vb
        'W',  // vn
        'D',  // vq
        'L',  // vd
        'B'   // va
       };

    static constexpr std::array<std::uint16_t, type_va+1u>
    plc_var_address =
       {
        0,   // none
        300, // vb
        400, // vn
        500, // vq
        600, // vd
        700  // va
       };

    //static constexpr std::array<std::string_view, type_va+1u>
    //plc_mem_map =
    //   {
    //    ""sv,      // none
    //    "MB300"sv, // vb
    //    "MW400"sv, // vn
    //    "MD500"sv, // vq
    //    "ML600"sv, // vd
    //    "MB700"sv  // va
    //   };

 public:
    explicit constexpr Register(const std::string_view s) noexcept
       {// From strings like "vq123"
        // Prefix
        if( s.length()>2 && (s[0]=='v' || s[0]=='V') )
           {
            // Type
                 if(s[1]=='b' || s[1]=='B') i_type = type_vb;
            else if(s[1]=='n' || s[1]=='N') i_type = type_vn;
            else if(s[1]=='q' || s[1]=='Q') i_type = type_vq;
            else if(s[1]=='d' || s[1]=='D') i_type = type_vd;
            else if(s[1]=='a' || s[1]=='A') i_type = type_va;

            // Index
            if( i_type!=type_none )
               {
                const auto i_end = s.data() + s.length();
                const auto [i, ec] = std::from_chars(s.data()+2, i_end, i_index);
                if( ec!=std::errc() || i!=i_end ) i_type = type_none; // Not an index, invalidate
               }
           }
       }

    [[nodiscard]] constexpr std::uint16_t index() const noexcept { return i_index; }
    //[[nodiscard]] constexpr en_regtype type() const noexcept { return i_type; }

    [[nodiscard]] constexpr bool is_valid() const noexcept { return i_type!=type_none; }
    [[nodiscard]] constexpr bool is_va() const noexcept { return i_type==type_va; }

    [[nodiscard]] constexpr std::uint16_t get_va_length() const noexcept { return 80; }

    [[nodiscard]] constexpr std::string_view iec_type() const noexcept { return reg_iec_types[i_type]; }
    [[nodiscard]] constexpr char iec_address_type() const noexcept { return 'M'; }
    [[nodiscard]] constexpr char iec_address_vartype() const noexcept { return plc_var_type[i_type]; }
    [[nodiscard]] constexpr std::uint16_t iec_address_index() const noexcept { return plc_var_address[i_type]; }

 private:
    std::uint16_t i_index = 0;
    en_regtype i_type = type_none;
};

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



//---- end unit -------------------------------------------------------------
#endif
