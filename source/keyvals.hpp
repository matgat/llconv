#ifndef keyvals_hpp
#define keyvals_hpp
/*  ---------------------------------------------
    ©2021 matteo.gattanini@gmail.com

    DEPENDENCIES:
    --------------------------------------------- */
#include <cctype> // std::isspace, ...
#include <string>
#include <string_view>
#include <map>
#include <optional>

using namespace std::literals; // Use "..."sv


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace str //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

/////////////////////////////////////////////////////////////////////////////
// A map of string pairs
class keyvals
{
 public:
    using string_type = std::string;
    using container_type = std::map<string_type,string_type>;
    using iterator = container_type::iterator;
    using const_iterator = container_type::const_iterator;

    //-----------------------------------------------------------------------
    // Get from strings like: "key1:val1, key2, key3:val3"
    void assign(const std::string_view s, const char sep =',') noexcept
       {
        //string_type key,val;
        const std::size_t i_end = s.size();
        std::size_t i=0;     // Current character index
        while( i<i_end )
           {
            // Skip possible spaces
            while( i<i_end && std::isspace(s[i]) ) ++i;
            // Get key
            const std::size_t i_k0 = i;
            while( i<i_end && !std::isspace(s[i]) && s[i]!=sep && s[i]!=':' && s[i]!='=' ) ++i;
            const std::size_t i_klen = i-i_k0;
            // Skip possible spaces
            while( i<i_end && std::isspace(s[i]) ) ++i;
            // Get possible value
            std::size_t i_v0=0, i_vlen=0;
            if( s[i]==':' || s[i]=='=' )
               {
                // Skip key/value separator
                ++i;
                // Skip possible spaces
                while( i<i_end && std::isspace(s[i]) ) ++i;
                // Collect value
                i_v0 = i;
                while( i<i_end && !std::isspace(s[i]) && s[i]!=sep ) ++i;
                i_vlen = i-i_v0;
                // Skip possible spaces
                while( i<i_end && std::isspace(s[i]) ) ++i;
               }
            // Skip possible delimiter
            if( i<i_end && s[i]==sep ) ++i;

            // Add key if found
            if(i_klen>0)
               {
                // Insert or assign
                i_map[std::string(s.data()+i_k0, i_klen)] = i_vlen>0 ? std::string(s.data()+i_v0, i_vlen) : ""s;
               }
           }
       }

    //-----------------------------------------------------------------------
    string_type to_str(const string_type& sep =",") const
       {
        string_type s;
        const_iterator i = i_map.begin();
        if( i!=i_map.end() )
           {
            s += i->first;
            if( !i->second.empty() ) s += ":" + i->second;
            while( ++i!=i_map.end() )
               {
                s += sep + i->first;
                if( !i->second.empty() ) s += ":" + i->second;
               }
           }
        return s;
       }

    //-----------------------------------------------------------------------
    bool is_empty() const noexcept
       {
        return i_map.empty();
       }

    //-----------------------------------------------------------------------
    bool contains(const string_type& key) const noexcept
       {
        return i_map.find(key)!=i_map.end();
       }

    //-----------------------------------------------------------------------
    std::optional<string_type> value_of(const string_type& key) const noexcept
       {
        std::optional<string_type> val;
        const const_iterator i = i_map.find(key);
        if( i!=i_map.end() ) val = i->second;
        return val;
       }

    //-----------------------------------------------------------------------
    //const string_type& value_of(const string_type& key, const string_type& def_val) const noexcept
    //   {
    //    const const_iterator i = i_map.find(key);
    //    if( i!=i_map.end() ) return i->second;
    //    return def_val;
    //   }

    //-----------------------------------------------------------------------
    friend std::ostream& operator<<(std::ostream& os, const keyvals& kv)
       {
        os << kv.to_str();
        return os;
       }

 private:
    container_type i_map;
};



}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



//---- end unit -------------------------------------------------------------
#endif
