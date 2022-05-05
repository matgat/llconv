#ifndef GUARD_system_hpp
#define GUARD_system_hpp
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
    #include <shellapi.h> // FindExecutableA
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

    #include <filesystem> // std::filesystem
    namespace fs = std::filesystem;

    #include <regex> // std::regex*
    #include "string-utilities.hpp" // str::glob_match


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
    const DWORD siz =
        ::FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM |
                         FORMAT_MESSAGE_IGNORE_INSERTS|
                         FORMAT_MESSAGE_MAX_WIDTH_MASK,
                         nullptr,
                         e,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                         buf,
                         buf_siz,
                         nullptr );
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

//#include <unistd.h>
//#include <sys/types.h>
//int foo(char *adr[])
//{
//        pid_t pid;
//
//        pid=fork();
//        if (pid==0)
//        {
//                if (execv("/usr/bin/mozilla",adr)<0)
//                        return -1;
//                else
//                        return 1;
//        }
//        else if(pid>0)
//                return 2;
//        else
//                return 0;
//}

// sh_cmd() - executes a command in the background
// returns TRUE is command was executed (not the result of the command though..)
//static gint sh_cmd (gchar * path, gchar * cmd, gchar * args)
//{
//  gchar     cmd_line[256];
//  gchar   **argv;
//  gint      argp;
//  gint      rc = 0;
//
//  if (cmd == NULL)
//    return FALSE;
//
//  if (cmd[0] == '\0')
//    return FALSE;
//
//  if (path != NULL)
//    chdir (path);
//
//  snprintf (cmd_line, sizeof (cmd_line), "%s %s", cmd, args);
//
//  rc = g_shell_parse_argv (cmd_line, &argp, &argv, NULL);
//  if (!rc)
//  {
//    g_strfreev (argv);
//    return rc;
//  }
//
//  rc = g_spawn_async (path, argv, NULL,
//          G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_SEARCH_PATH,
//          NULL, NULL, NULL, NULL);
//
//  g_strfreev (argv);
//
//  return rc;
//}

// static gint get_new_ptable (P_Fah_monitor fm)
// {
//  gint   i_retcode = 0, i_exitcode = 0;
//  gchar cv_filename[384];
//
// #ifdef DLOGIC
//   g_message (CONFIG_NAME":> Entered get_new_ptable(%d)...\n",fm->cb_id);
// #endif
//
//   if ( fm->i_stanford_points )   // TRUE if point table IS out of date
//   {
//     chdir ( fm->path_string );
//
//     i_retcode = g_spawn_command_line_sync (
//                      g_strdup_printf ("wget -N %s", STANDFORD_FAHPOINTS_URL),
//                                          NULL, NULL, &i_exitcode, NULL);
//
//     if( i_retcode )
//     {
//      ... good if retcode = 0
//     }

//#include <stdlib.h>
//#include <unistd.h>
//#include <sys/types.h>
//#include <sys/wait.h>
//#include <errno.h>
// Launch preferred application (in parallel) to open the specified file.
// The function returns errno for (apparent) success,
// and nonzero error code otherwise.
// Note that error cases are visually reported by xdg-open to the user,
// so that there is no need to provide error messages to user.
//int open_preferred(const char *const filename)
//{
//    const char *const args[3] = { "xdg-open", filename, NULL };
//    pid_t child, p;
//    int status;
//
//    if (!filename || !*filename)
//        return errno = EINVAL; // Invalid file name
//
//    // Fork a child process.
//    child = fork();
//    if (child == (pid_t)-1)
//        return errno = EAGAIN; // Out of resources, or similar
//
//    if (!child) {
//        // Child process. Execute.
//        execvp(args[0], (char **)args);
//        // Failed. Return 3, "a required too could not be found".
//        exit(3);
//    }
//
//    // Parent process. Wait for child to exit.
//    do {
//        p = waitpid(child, &status, 0);
//    } while (p == (pid_t)-1 && errno == EINTR);
//    if (p != child)
//        return errno = ECHILD; // Failed; child process vanished
//
//    // Did the child process exit normally?
//    if (!WIFEXITED(status))
//        return errno = ECHILD; // Child process was aborted
//
//    switch (WEXITSTATUS(status)) {
//    case 0:  return errno = 0;       // Success
//    case 1:  return errno = EINVAL;  // Error in command line syntax
//    case 2:  return errno = ENOENT;  // File not found
//    case 3:  return errno = ENODEV;  // Application not found
//    default: return errno = EAGAIN;  // Failed for other reasons.
//    }
//}


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
class MemoryMappedFile final
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
        const int fd = open(pth.c_str(), O_RDONLY);
        if(fd == -1) throw std::runtime_error("Couldn't open file");

        // obtain file size
        struct stat sbuf {};
        if(fstat(fd, &sbuf) == -1) throw std::logic_error("Cannot stat file size");
        i_bufsiz = static_cast<std::size_t>(sbuf.st_size);

        i_buf = static_cast<const char*>(mmap(nullptr, i_bufsiz, PROT_READ, MAP_PRIVATE, fd, 0U));
        if(i_buf == MAP_FAILED)
           {
            i_buf = nullptr;
            throw std::runtime_error("Cannot map file");
           }
      #endif
       }

    ~MemoryMappedFile() noexcept
       {
        if(i_buf)
           {
          #ifdef MS_WINDOWS
            ::UnmapViewOfFile(i_buf);
            if(hMapping) ::CloseHandle(hMapping);
            if(hFile!=INVALID_HANDLE_VALUE) ::CloseHandle(hFile);
          #else
            /* const int ret = */ munmap(static_cast<void*>(const_cast<char*>(i_buf)), i_bufsiz);
            //if(ret==-1) std::cerr << "munmap() failed\n";
          #endif
           }
       }

    // Prevent copy
    MemoryMappedFile(const MemoryMappedFile& other) = delete;
    MemoryMappedFile& operator=(const MemoryMappedFile& other) = delete;

    // Move
    MemoryMappedFile(MemoryMappedFile&& other) noexcept
      : i_bufsiz(other.i_bufsiz)
      , i_buf(other.i_buf)
    #ifdef MS_WINDOWS
      , hFile(other.hFile)
      , hMapping(other.hMapping)
    #endif
       {
        other.i_bufsiz = 0;
        other.i_buf = nullptr;
      #ifdef MS_WINDOWS
        other.hFile = INVALID_HANDLE_VALUE;
        other.hMapping = nullptr;
      #endif
       }
    // Prevent move assignment
    MemoryMappedFile& operator=(MemoryMappedFile&& other) = delete;

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
class file_write final
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

    ~file_write() noexcept
       {
        fclose(i_File);
       }

    file_write(const file_write&) = delete;
    file_write(file_write&&) = delete;
    file_write& operator=(const file_write&) = delete;
    file_write& operator=(file_write&&) = delete;

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
//std::tuple<std::time_t, std::time_t> get_file_dates(const std::string& spth) noexcept
//{
//    //const std::filesystem::path pth(spth);
//    // Note: in c++20 std::filesystem::file_time_type is guaranteed to be epoch
//    //return {std::filesystem::last_creat?_time(pth), std::filesystem::last_write_time(pth)};
//
//  #ifdef MS_WINDOWS
//    HANDLE h = ::CreateFile(spth.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
//    if( h!=INVALID_HANDLE_VALUE )
//       {
//        //BY_HANDLE_FILE_INFORMATION fd{0};
//        //::GetFileInformationByHandle(h, &fd);
//        FILETIME ftCreationTime{0}, ftLastAccessTime{0}, ftLastWriteTime{0};
//        ::GetFileTime(h, &ftCreationTime, &ftLastAccessTime, &ftLastWriteTime);
//        ::CloseHandle(h);
//
//        FILETIME lftCreationTime{0}, lftLastWriteTime{0}; // Local file times
//        ::FileTimeToLocalFileTime(&ftCreationTime, &lftCreationTime);
//        ::FileTimeToLocalFileTime(&ftLastWriteTime, &lftLastWriteTime);
//
//        auto to_time_t = [](const FILETIME& ft) -> std::time_t
//           {
//            // FILETIME is is the number of 100 ns increments since January 1 1601
//            ULARGE_INTEGER wt = { ft.dwLowDateTime, ft.dwHighDateTime };
//            //const ULONGLONG TICKS_PER_SECOND = 10'000'000ULL;
//            //const ULONGLONG EPOCH_DIFFERENCE = 11644473600ULL;
//            return wt.QuadPart / 10000000ULL - 11644473600ULL;
//           };
//
//        return std::make_tuple(to_time_t(lftCreationTime), to_time_t(lftLastWriteTime));
//       }
//  #else
//    struct stat result;
//    if( stat(spth.c_str(), &result )==0 )
//       {
//        return {result.st_ctime, result.st_mtime};
//       }
//  #endif
//  return {0,0};
//}



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
// file_glob("/aaa/bbb/*.txt");
std::vector<fs::path> file_glob(const fs::path pth)
{
    if( str::contains_wildcards(pth.parent_path().string()) )
       {
        throw std::runtime_error("sys::file_glob: Wildcards in directories not supported");
       }

    //if( pth.is_relative() ) pth = fs::absolute(pth); // Ensure absolute path?
    fs::path parent_folder = pth.parent_path();
    if( parent_folder.empty() ) parent_folder = fs::current_path();

    const std::string filename_glob = pth.filename().string();
    std::vector<fs::path> result;
    if( str::contains_wildcards(filename_glob) && fs::exists(parent_folder) )
       {
        result.reserve(16); // Minimize initial allocations
        for( const auto& entry : fs::directory_iterator(parent_folder, fs::directory_options::follow_directory_symlink |
                                                                       fs::directory_options::skip_permission_denied) )
           {// Collect if matches
            if( entry.exists() && entry.is_regular_file() && str::glob_match(entry.path().filename().string().c_str(), filename_glob.c_str()) )
               {
                //const fs::path entry_path = parent_folder.is_absolute() ? entry.path() : fs::proximate(entry.path());
                result.push_back( entry.path() );
               }
           }

        // Using std::regex
        //// Create a regular expression from glob pattern
        //auto glob2regex = [](const std::string& glob_pattern) noexcept -> std::string
        //   {
        //    // Escape special characters in file name
        //    std::string regexp_pattern = std::regex_replace(glob_pattern, std::regex("([" R"(\$\.\+\-\=\[\]\(\)\{\}\|\^\!\:\\)" "])"), "\\$1");
        //    // Substitute pattern
        //    regexp_pattern = std::regex_replace(regexp_pattern, std::regex(R"(\*)"), ".*");
        //    regexp_pattern = std::regex_replace(regexp_pattern, std::regex(R"(\?)"), ".");
        //    //fmt::print("regexp_pattern: {}" ,regexp_pattern);
        //    return regexp_pattern;
        //   };
        //const std::regex reg(glob2regex(filename_glob), std::regex_constants::icase);
        //... std::regex_match(entry.path().filename().string(), reg)
       }
    else
       {// Nothing to glob
        result.reserve(1);
        result.push_back(pth);
       }

    return result;
}



//---------------------------------------------------------------------------
// ex. const auto removed_count = remove_all_inside(fs::temp_directory_path(), std::regex{R"-(^.*\.(tmp)$)-"});
//std::size_t remove_all_inside(const std::filesystem::path& dir, std::regex&& reg)
//{
//    std::size_t removed_items_count { 0 };
//
//    if( !fs::is_directory(dir) )
//       {
//        throw std::invalid_argument("Not a directory: " + dir.string());
//       }
//
//    for( auto& elem : fs::directory_iterator(dir) )
//       {
//        if( std::regex_match(elem.path().filename().string(), reg) )
//           {
//            removed_items_count += fs::remove_all(elem.path());
//           }
//       }
//
//    return removed_items_count;
//}



//---------------------------------------------------------------------------
// ex. const auto removed_count = remove_files_inside(fs::temp_directory_path(), std::regex{R"-(^.*\.(tmp)$)-"});
std::size_t remove_files_inside(const std::filesystem::path& dir, std::regex&& reg)
{
    std::size_t removed_items_count { 0 };

    if( !fs::is_directory(dir) )
       {
        throw std::invalid_argument("Not a directory: " + dir.string());
       }

    for( auto& elem : fs::directory_iterator(dir) )
       {
        if( elem.is_regular_file() && std::regex_match(elem.path().filename().string(), reg) )
           {
            removed_items_count += fs::remove(elem.path());
           }
       }

    return removed_items_count;
}



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


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



//---- end unit -------------------------------------------------------------
#endif
