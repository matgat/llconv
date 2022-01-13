#ifndef system_hpp
#define system_hpp
/*  ---------------------------------------------
    ©2021-2022 matteo.gattanini@gmail.com

    OVERVIEW
    ---------------------------------------------
    Some system utilities

    DEPENDENCIES:
    --------------------------------------------- */
  #if defined(_WIN32) || defined(_WIN64)
    #define MS_WINDOWS 1
  #else
    #undef MS_WINDOWS
  #endif

  #ifdef MS_WINDOWS
    #include <Windows.h>
    //#include <unistd.h> // _stat
  #else
    #include <fcntl.h> // open
    #include <sys/mman.h> // mmap, munmap
    #include <sys/stat.h> // fstat
    #include <unistd.h> // unlink
  #endif
#include <string>
#include <string_view>
#include <tuple>
#include <stdexcept>
#include <cstdio> // std::fopen, ...
#include <cstdlib> // std::getenv
//#include <fstream>
//#include <chrono> // std::chrono::system_clock
//using namespace std::chrono_literals; // 1s, 2h, ...
#include <ctime> // std::time_t, std::strftime
#include <regex> // std::regex* in glob
#include <filesystem> // std::filesystem
namespace fs = std::filesystem;


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace sys //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{



//---------------------------------------------------------------------------
//std::string expand_env_variables( const std::string& s )
//{
//    const std::string::size_type i_start = s.find("${");
//    if( i_start == std::string::npos ) return s;
//    std::string_view pre( s.data(), i_start );
//    i_start += 2; // Skip "$("
//    const std::string::size_type i_end = s.find('}', i_start);
//    if( i_end == std::string::npos ) return s;
//    std::string_view post = ( s.data()+i_end+1, s.length()-(i_end+1) );
//    std::string_view variable( s.data()+i_start, i_end-i_start );
//    std::string value { std::getenv(variable.c_str()) };
//    return expand_env_variables( fmt::format("{}{}{}",pre,value,post) );
//}


//---------------------------------------------------------------------------
std::string expand_env_variables( std::string s )
{
    static const std::regex env_re{R"--(\$\{([\w_]+)\}|%([\w_]+)%)--"};
    std::smatch match;
    while( std::regex_search(s, match, env_re) )
       {
        s.replace(match[0].first, match[0].second, std::getenv(match[1].str().c_str()));
       }
    return s;
}


#ifdef MS_WINDOWS
//---------------------------------------------------------------------------
// Format system error message
std::string get_lasterr_msg(DWORD e =0) noexcept
{
    if(e==0) e = ::GetLastError(); // ::WSAGetLastError()
    const DWORD buf_siz = 1024;
    TCHAR buf[buf_siz];
    DWORD siz = ::FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM |
                                 FORMAT_MESSAGE_IGNORE_INSERTS|
                                 FORMAT_MESSAGE_MAX_WIDTH_MASK,
                                 nullptr,
                                 e,
                                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                 buf,
                                 buf_siz,
                                 nullptr);
    std::string msg( "[" + std::to_string(e) + "]" );
    if(siz>0) msg += " " + std::string(buf, siz);
    return msg;
}

//---------------------------------------------------------------------------
std::string find_executable(const std::string& pth) noexcept
{
    char buf[MAX_PATH + 1] = {'\0'};
    ::FindExecutableA(pth.c_str(), NULL, buf);
    std::string exe_path(buf);
    return exe_path;
}
#endif


//---------------------------------------------------------------------------
void launch(const std::string& pth, const std::string& args ="") noexcept
{
  #ifdef MS_WINDOWS
    SHELLEXECUTEINFOA ShExecInfo = {0};
    ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFOA);
    ShExecInfo.fMask = 0;
    ShExecInfo.hwnd = NULL;
    ShExecInfo.lpVerb = "open";
    ShExecInfo.lpFile = pth.c_str();
    ShExecInfo.lpParameters = args.c_str();
    ShExecInfo.lpDirectory = NULL;
    ShExecInfo.nShow = SW_SHOW;
    ShExecInfo.hInstApp = NULL;
    ::ShellExecuteEx(&ShExecInfo);
  #else
    //g_spawn_command_line_sync ?
    //#include <unistd.h> // 'exec*'
    // v: take an array parameter to specify the argv[] array of the new program. The end of the arguments is indicated by an array element containing NULL.
    // l: take the arguments of the new program as a variable-length argument list to the function itself. The end of the arguments is indicated by a (char *)NULL argument. You should always include the type cast, because NULL is allowed to be an integer constant, and default argument conversions when calling a variadic function won't convert that to a pointer.
    // e: take an extra argument (or arguments in the l case) to provide the environment of the new program; otherwise, the program inherits the current process's environment. This is provided in the same way as the argv array: an array for execve(), separate arguments for execle().
    // p: search the PATH environment variable to find the program if it doesn't have a directory in it (i.e. it doesn't contain a / character). Otherwise, the program name is always treated as a path to the executable.
    //int execvp (const char *file, char *const argv[]);
    //int execlp(const char *file, const char *arg,.../* (char  *) NULL */);

    //pid = fork();
    //if( pid == 0 )
    //   {
    //    execlp("/usr/bin/xdg-open", "xdg-open", pth.c_str(), nullptr);
    //   }
  #endif
}


//---------------------------------------------------------------------------
void edit_text_file(const std::string& pth, const std::size_t offset) noexcept
{
  #ifdef MS_WINDOWS
    const std::string exe_pth = find_executable(pth);
    //if( exe_pth.contains("notepad++") ) args = " -n"+String(l)+" -c"+String(c)+" -multiInst -nosession \"" + pth + "\"";
    //else if( exe_pth.contains("subl") ) args = " \"" + pth + "\":" + String(l) + ":" + String(c);
    //else if( exe_pth.contains("scite") ) args = "\"-open:" + pth + "\" goto:" + String(l) + "," + String(c);
    //else if( exe_pth.contains("uedit") ) args = " \"" + pth + "\" -l" + String(l) + " -c" + String(c); // or: +"/"+String(l)+"/"+String(c)
    //else args =  " \""+pth+"\"";
    launch(exe_pth, "-nosession -p" + std::to_string(offset) + " \"" + pth + "\"");
  #else
  #endif
}



/////////////////////////////////////////////////////////////////////////////
class MemoryMappedFile //////////////////////////////////////////////////////
{
 public:
    explicit MemoryMappedFile( const std::string& pth )
       {
      #ifdef MS_WINDOWS
        hFile = ::CreateFileA(pth.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, nullptr);
        if(hFile == INVALID_HANDLE_VALUE)
           {
            throw std::runtime_error("Couldn't open " + pth + " (" + get_lasterr_msg() + ")");
           }
        i_bufsiz = ::GetFileSize(hFile, nullptr);

        hMapping = ::CreateFileMappingA(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
        if(hMapping == nullptr)
           {
            ::CloseHandle(hFile);
            throw std::runtime_error("Couldn't map " + pth + " (" + get_lasterr_msg() + ")");
           }
        //
        i_buf = static_cast<const char*>( ::MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0) );
        if(i_buf == nullptr)
           {
            ::CloseHandle(hMapping);
            ::CloseHandle(hFile);
            throw std::runtime_error("Couldn't create view of " + pth + " (" + get_lasterr_msg() + ")");
           }
      #else
        int fd = open(pth.c_str(), O_RDONLY);
        if(fd == -1) throw std::runtime_error("Couldn't open file");

        // obtain file size
        struct stat sbuf {};
        if(fstat(fd, &sbuf) == -1) throw std::logic_error("Cannot stat file size");
        i_bufsiz = sbuf.st_size;

        i_buf = static_cast<const char*>(mmap(nullptr, i_bufsiz, PROT_READ, MAP_PRIVATE, fd, 0U));
        if(i_buf == MAP_FAILED) throw std::runtime_error("Cannot map file");
      #endif
       }

   ~MemoryMappedFile() noexcept
       {
      #ifdef MS_WINDOWS
        ::UnmapViewOfFile(i_buf);
        ::CloseHandle(hMapping);
        ::CloseHandle(hFile);
      #else
        /* int ret = */ munmap(static_cast<void*>(const_cast<char*>(i_buf)), i_bufsiz);
        //if(ret==-1) std::cerr << "munmap() failed\n";
      #endif
       }

   MemoryMappedFile(const MemoryMappedFile& other) = delete;
   MemoryMappedFile& operator=(MemoryMappedFile other) = delete;
   MemoryMappedFile(MemoryMappedFile&& other) = default;
   MemoryMappedFile& operator=(MemoryMappedFile&& other) = default;

   [[nodiscard]] std::size_t size() const noexcept { return i_bufsiz; }
   [[nodiscard]] const char* begin() const noexcept { return i_buf; }
   [[nodiscard]] const char* end() const noexcept { return i_buf + i_bufsiz; }
   [[nodiscard]] std::string_view as_string_view() const noexcept { return std::string_view{i_buf, i_bufsiz}; }

 private:
    std::size_t i_bufsiz = 0;
    const char* i_buf = nullptr;
  #ifdef MS_WINDOWS
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE hMapping = nullptr;
  #endif
};


/////////////////////////////////////////////////////////////////////////////
// (Over)Write a file
class file_write
{
 public:
    explicit file_write(const std::string& pth)
       {
      #ifdef MS_WINDOWS
        const errno_t err = fopen_s(&i_File, pth.c_str(), "wb"); // "a" for append
        if(err) throw std::runtime_error("Cannot write to: " + pth);
      #else
        i_File = fopen(pth.c_str(), "wb"); // "a" for append
        if(!i_File) throw std::runtime_error("Cannot write to: " + pth);
      #endif
       }

    file_write(const file_write&) = delete;
    file_write& operator=(const file_write&) = delete;

    ~file_write() noexcept
       {
        fclose(i_File);
       }

    const file_write& operator<<(const char c) const noexcept
       {
        fputc(c, i_File);
        return *this;
       }

    const file_write& operator<<(const std::string_view s) const noexcept
       {
        fwrite(s.data(), sizeof(std::string_view::value_type), s.length(), i_File);
        return *this;
       }

    const file_write& operator<<(const std::string& s) const noexcept
       {
        fwrite(s.data(), sizeof(std::string::value_type), s.length(), i_File);
        return *this;
       }

 private:
    FILE* i_File;
};


//---------------------------------------------------------------------------
// Formatted time stamp
//std::string human_readable_time_stamp(const std::filesystem::file_time_type ftime)
//{
//    std::time_t cftime = std::chrono::system_clock::to_time_t(std::chrono::file_clock::to_sys(ftime));
//    return std::asctime( std::localtime(&cftime) );
//}


//---------------------------------------------------------------------------
// Formatted time stamp
std::string human_readable_time_stamp(const std::time_t t)
{
    //return fmt::format("{:%Y-%m-%d}", std::localtime(&t));
    //std::formatter()
    char buf[64];
    std::size_t len = std::strftime(buf, sizeof(buf), "%F %T", std::localtime(&t));
        // %F  equivalent to "%Y-%m-%d" (the ISO 8601 date format)
        // %T  equivalent to "%H:%M:%S" (the ISO 8601 time format)
    return std::string(buf, len);
}

//---------------------------------------------------------------------------
// Formatted time stamp
std::string human_readable_time_stamp()
{
    const std::time_t now = std::time(nullptr); // std::chrono::system_clock::to_time_t( std::chrono::system_clock::now() );
    return human_readable_time_stamp(now);
}


//---------------------------------------------------------------------------
// The Unix epoch (or Unix time or POSIX time or Unix timestamp) is the
// number of seconds that have elapsed since 1970-01-01T00:00:00Z
//auto epoch_time_stamp()
//{
//    return epoch_time_stamp( std::chrono::system_clock::now() );
//}
//auto epoch_time_stamp(const std::chrono::system_clock::time_point tp)
//{
//    return std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count();
//}



//---------------------------------------------------------------------------
// auto [ctime, mtime] = get_file_dates("/path/to/file");
std::tuple<std::time_t, std::time_t> get_file_dates(const std::string& spth) noexcept
{
    //const std::filesystem::path pth(spth);
    // Note: in c++20 std::filesystem::file_time_type is guaranteed to be epoch
    //return {std::filesystem::last_creat?_time(pth), std::filesystem::last_write_time(pth)};

  #ifdef MS_WINDOWS
    HANDLE h = ::CreateFile(spth.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if( h!=INVALID_HANDLE_VALUE )
       {
        //BY_HANDLE_FILE_INFORMATION fd{0};
        //::GetFileInformationByHandle(h, &fd);
        FILETIME ftCreationTime{0}, ftLastAccessTime{0}, ftLastWriteTime{0};
        ::GetFileTime(h, &ftCreationTime, &ftLastAccessTime, &ftLastWriteTime);
        ::CloseHandle(h);

        FILETIME lftCreationTime{0}, lftLastWriteTime{0}; // Local file times
        ::FileTimeToLocalFileTime(&ftCreationTime, &lftCreationTime);
        ::FileTimeToLocalFileTime(&ftLastWriteTime, &lftLastWriteTime);

        //auto to_time_t = [](const FILETIME& ft) -> std::time_t
        //   {
        //    #include "date.h" // https://github.com/HowardHinnant/date
        //    ULARGE_INTEGER wt = { ft.dwLowDateTime, ft.dwHighDateTime };
        //    using wfs = date::duration<ULONGLONG, std::ratio<1, 10'000'000>>;
        //    std::chrono::system_clock::time_point tp{ date::floor<system_clock::duration>(wfs{wt} - (date::sys_days{1970_y/jan/1} - date::sys_days{1601_y/jan/1})) };
        //    return epoch_time_stamp(tp);
        //   };

        //auto to_time_t = [](const FILETIME& ft) -> std::time_t
        //   {
        //    SYSTEMTIME st;  ::FileTimeToSystemTime(&lft, &st);
        //    struct tm tmtime = {0};
        //    tmtime.tm_year = st.wYear - 1900;
        //    tmtime.tm_mon = st.wMonth - 1;
        //    tmtime.tm_mday = st.wDay;
        //    tmtime.tm_hour = st.wHour;
        //    tmtime.tm_min = st.wMinute;
        //    tmtime.tm_sec = st.wSecond;
        //    tmtime.tm_wday = 0;
        //    tmtime.tm_yday = 0;
        //    tmtime.tm_isdst = -1;
        //    return mktime(&tmtime);
        //   };

        auto to_time_t = [](const FILETIME& ft) -> std::time_t
           {
            // FILETIME is is the number of 100 ns increments since January 1 1601
            ULARGE_INTEGER wt = { ft.dwLowDateTime, ft.dwHighDateTime };
            //const ULONGLONG TICKS_PER_SECOND = 10'000'000ULL;
            //const ULONGLONG EPOCH_DIFFERENCE = 11644473600ULL;
            return wt.QuadPart / 10000000ULL - 11644473600ULL;
           };

        return std::make_tuple(to_time_t(lftCreationTime), to_time_t(lftLastWriteTime));
       }
  #else
    struct stat result;
    if( stat(spth.c_str(), &result )==0 )
       {
        return {result.st_ctime, result.st_mtime};
       }
  #endif
  return {0,0};
}



//---------------------------------------------------------------------------
void delete_file(const std::string& pth) noexcept
{
  //std::filesystem::remove(pth);
  #ifdef MS_WINDOWS
    ::DeleteFile( pth.c_str() );
  #else
    unlink( pth.c_str() );
  #endif
}


//---------------------------------------------------------------------------
// glob("/aaa/bbb/*.txt");
std::vector<fs::path> glob(const std::string_view str_pattern)
{
    const fs::path path_pattern = expand_env_variables( std::string(str_pattern) );

    auto contains_wildcards = [](const std::string& pth) noexcept -> bool
       {
        //return std::regex_search(pth, std::regex("*?"));
        return pth.rfind('*') != std::string::npos || pth.rfind('?') != std::string::npos;
       };
    if( contains_wildcards(path_pattern.parent_path().string()) ) throw std::runtime_error("glob: Wildcards in directories not supported (" + path_pattern.string() + ")");
    //if( path_pattern.is_relative() ) path_pattern = fs::absolute(path_pattern); // Ensure absolute path?
    const fs::path parent_folder = path_pattern.parent_path().empty() ? fs::current_path() : path_pattern.parent_path();
    const std::string file_pattern = path_pattern.filename().string();
    std::vector<fs::path> result;
    if( contains_wildcards(file_pattern) && fs::exists(parent_folder) )
       {
        try{
            // Prepare match pattern: create a regular expression from glob pattern
            auto glob2regex = [](const std::string& glob_pattern) noexcept -> std::string
               {
                // Escape special characters in file name
                std::string regexp_pattern = std::regex_replace(glob_pattern, std::regex("([" R"(\$\.\+\-\=\[\]\(\)\{\}\|\^\!\:\\)" "])"), "\\$1");
                // Substitute pattern
                regexp_pattern = std::regex_replace(regexp_pattern, std::regex(R"(\*)"), ".*");
                regexp_pattern = std::regex_replace(regexp_pattern, std::regex(R"(\?)"), ".");
                //dlg::print("regexp_pattern: {}" ,regexp_pattern);
                return regexp_pattern;
               };
            const std::regex reg(glob2regex(file_pattern), std::regex_constants::icase);
            for( const auto& entry : fs::directory_iterator(parent_folder, fs::directory_options::follow_directory_symlink |
                                                                           fs::directory_options::skip_permission_denied) )
               {// Collect if matches
                if( entry.exists() && entry.is_regular_file() && std::regex_match(entry.path().filename().string(), reg) )
                   {
                    //const fs::path entry_path = parent_folder.is_absolute() ? entry.path() : fs::proximate(entry.path());
                    result.push_back( entry.path() );
                   }
               }
           }
        catch( std::exception&) {} // Not a directory, do nothing
       }
    else
       {
        result.reserve(1);
        result.push_back(path_pattern);
       }

    return result;
}


/////////////////////////////////////////////////////////////////////////////
//sys::path file("c:\\aaa\\bbb\\ccc\\name.ext");
//std::cout << "  full_path: " << file.full_path()
//          << "  dir: " << std::string(file.directory())
//          << "  name: " << std::string(file.filename())
//          << "  extension: " << std::string(file.extension())
//          << "  stem: " << std::string(file.stem()) << '\n';
//class path
//{
// public:
//    explicit path(const std::string& pth) : i_path(pth)
//       {
//        i_pathpos = i_path.find_last_of("\\/");
//        i_extpos = i_path.rfind('.');
//       }
//
//    const std::string& full_path() const noexcept { return i_path; }
//
//    std::string_view directory() const noexcept
//       {
//        if( i_pathpos == std::string::npos ) return std::string_view(i_path.c_str(),0);
//        return std::string_view(i_path.c_str(), i_pathpos+1);
//       }
//
//    std::string_view filename() const noexcept
//       {
//        if( i_pathpos == std::string::npos ) return std::string_view(i_path);
//        return std::string_view(i_path.c_str()+i_pathpos+1, i_path.length()-(i_pathpos+1));
//       }
//
//    std::string_view extension() const noexcept
//       {
//        if( i_extpos == std::string::npos ) return std::string_view(i_path.c_str(),0);
//        return std::string_view(i_path.c_str()+i_extpos, i_path.length()-i_extpos);
//       }
//
//    std::string_view stem() const noexcept
//       {
//        if( i_extpos == std::string::npos ) return filename();
//        else if( i_pathpos == std::string::npos ) return std::string_view(i_path.c_str(), i_extpos);
//        return std::string_view(i_path.c_str()+i_pathpos+1, i_extpos-i_pathpos-1);
//       }
//
//    std::string replace_extension(std::string newext) const noexcept
//       {
//        return newext = i_path.substr(0,i_extpos) + newext;
//       }
//
// private:
//    std::string i_path;
//    std::string::size_type i_pathpos = std::string::npos;
//    std::string::size_type i_extpos = std::string::npos;
//};


//---------------------------------------------------------------------------
// Append string to existing file using c++ streams
//void append_to_file(const std::string_view path, const std::string_view txt)
//{
//    std::ofstream os;
//    //os.exceptions(os.exceptions() | std::ios::failbit); // Some gcc has a bug bug that raises a useless 'ios_base::failure'
//    os.open(path, std::ios::out | std::ios::app);
//    if(os.fail()) throw std::ios_base::failure(std::strerror(errno));
//    os.exceptions(os.exceptions() | std::ios::failbit | std::ifstream::badbit);
//    os << txt;
//}

//---------------------------------------------------------------------------
// Buffer the content of a file using c++ streams
//std::string read(const std::string_view path)
//{
//    std::ifstream is(path, std::ios::in | std::ios::binary);
//        // Read file size
//        is.seekg(0, std::ios::end);
//        const std::size_t buf_siz = is.tellg();
//        is.seekg(0, std::ios::beg);
//    std::string buf;
//    buf.reserve(buf_siz+1);
//    is.read(buf.data(), buf_siz);
//    buf.set_length(buf_siz);
//    buf[buf_siz] = '\0';
//    return buf;
//}

//---------------------------------------------------------------------------
// Split to lines
//std::vector<std::string_view> split_lines(const std::string_view buf)
//{
//    std::vector<std::string_view> lines;
//    lines.reserve(buf.size() / 40);
//    std::size_t i=0, i_start=0;
//    while( i<buf.size() )
//       {
//        if( buf[i] == '\n' )
//           {
//            lines.emplace_back(buf.data()+i_start, i-i_start);
//            i_start = ++i;
//           }
//        else ++i;
//       }
//    if(i>i_start) lines.emplace_back(buf.data()+i_start, i-i_start);
//    return lines;
//}

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



//---- end unit -------------------------------------------------------------
#endif
