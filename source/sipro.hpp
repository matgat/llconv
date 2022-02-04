#ifndef sipro_hpp
#define sipro_hpp
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


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace sipro
{
    enum class RegisterType : std::size_t
       {
        none=0, // !Consecutive indexes!
        vb,
        vn,
        vq,
        vd,
        va
       };


    /////////////////////////////////////////////////////////////////////////
    struct Register
    {
        RegisterType type = RegisterType::none;
        std::uint16_t index = 0;

        constexpr bool is_valid() const noexcept { type!=RegisterType::none; }
    };


    //-----------------------------------------------------------------------
    // Determine register type and index from strings like "vq123"
    constexpr Register parse_register(const std::string_view s) noexcept
       {
        Register reg;
        // Recognize register prefix
        if( s.length()>3 && (s[0]=='v' || s[0]=='V') )
           {
            // Recognize register type
                 if(s[1]=='b' || s[1]=='B') reg.type = RegisterType::vb;
            else if(s[1]=='n' || s[1]=='N') reg.type = RegisterType::vn;
            else if(s[1]=='q' || s[1]=='Q') reg.type = RegisterType::vq;
            else if(s[1]=='d' || s[1]=='D') reg.type = RegisterType::vd;
            else if(s[1]=='a' || s[1]=='A') reg.type = RegisterType::va;

            // If it's recognized, check the index
            if( reg.type!=RegisterType::none )
               {
                const auto i_end = s.data() + s.length();
                const auto [i, ec] = std::from_chars(s.data()+2, i_end, reg.index);
                if( ec!=std::errc() || i!=i_end ) reg.type = RegisterType::none; // Not an index, invalidate
               }
           }
        return reg;
       }

    //-----------------------------------------------------------------------
    constexpr std::string_view iec_type(const RegisterType v) noexcept
       {
        constexpr std::array<std::string_view> reg_iec_types =
           {
            ""sv,          // none
            "BOOL"sv,      // vb
            "INT"sv,       // vn
            "DINT"sv,      // vq
            "LREAL"sv,     // vd
            "STRING[80]"sv // va
           };
        return reg_iec_types[static_cast<std::size_t>(v)];
       }


    //-----------------------------------------------------------------------
    constexpr char iec_address_type(const RegisterType v) noexcept
       {
        return 'M';
       }

    //-----------------------------------------------------------------------
    constexpr char iec_address_vartype(const RegisterType v) noexcept
       {
        constexpr std::array<std::string_view> plc_vartype =
           {
            '\0', // none
            'B',  // vb
            'W',  // vn
            'D',  // vq
            'L',  // vd
            'B'   // va
           };
        return plc_vartype[static_cast<std::size_t>(v)];
       }

    //-----------------------------------------------------------------------
    constexpr std::uint16_t iec_address_index(const RegisterType v) noexcept
       {
        constexpr std::array<std::string_view> plc_vartype =
           {
            0,   // none
            300, // vb
            400, // vn
            500, // vq
            600, // vd
            700  // va
           };
        return plc_vartype[static_cast<std::size_t>(v)];
       }

    //-----------------------------------------------------------------------
    //constexpr std::string_view iec_address(const RegisterType v) noexcept
    //   {
    //    constexpr std::array<std::string_view> plc_mem_map =
    //       {
    //        ""sv,      // none
    //        "MB300"sv, // vb
    //        "MW400"sv, // vn
    //        "MD500"sv, // vq
    //        "ML600"sv, // vd
    //        "MB700"sv  // va
    //       };
    //    return plc_mem_map[static_cast<std::size_t>(v)];
    //   }

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



//---- end unit -------------------------------------------------------------
#endif
