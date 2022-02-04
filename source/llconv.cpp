/*  ---------------------------------------------
    llconv
    ©2021-2022 matteo.gattanini@gmail.com

    OVERVIEW
    ---------------------------------------------
    Conversion utility for Sipro/LogicLab formats

    DEPENDENCIES:
    --------------------------------------------- */
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <stdexcept> // std::runtime_error
#include <fmt/core.h> // fmt::format

#include "system.hpp" // sys::*, fs::*
#include "string-utilities.hpp" // str::tolower
#include "keyvals.hpp" // str::keyvals
#include "logging.hpp" // dlg::parse_error
#include "h-parser.hpp" // h::*
#include "pll-parser.hpp" // pll::*
#include "plc-elements.hpp" // plcb::*
#include "plclib-writer.hpp" // plclib::write
#include "pll-writer.hpp" // pll::write

using namespace std::literals; // Use "..."sv


/////////////////////////////////////////////////////////////////////////////
class Arguments
{
 public:
    Arguments(int argc, const char* argv[])
       {
        //std::vector<std::string> args(argv+1, argv+argc); for( std::string& arg : args )
        // Expecting pll file paths
        i_files.reserve( static_cast<std::size_t>(argc));
        try{
            enum class STS
               {
                SEE_ARG,
                GET_OPTS,
                GET_OUT
               } status = STS::SEE_ARG;

            for( int i=1; i<argc; ++i )
               {
                const std::string_view arg{ argv[i] };
                switch( status )
                   {
                    case STS::SEE_ARG :
                        if( arg[0] == '-' )
                           {// A command switch!
                            if( arg=="-fussy"sv )
                               {
                                i_fussy = true;
                               }
                            else if( arg=="-verbose"sv )
                               {
                                i_verbose = true;
                               }
                            else if( arg=="-sort"sv )
                               {
                                i_sort = true;
                               }
                            else if( arg=="-options"sv )
                               {
                                status = STS::GET_OPTS; // Value expected
                               }
                            else if( arg=="-output"sv )
                               {
                                status = STS::GET_OUT; // Value expected
                               }
                            else if( arg=="-h"sv || arg=="-help"sv )
                               {
                                print_help();
                                throw std::invalid_argument("Aborting after printing help");
                               }
                            else
                               {
                                throw std::invalid_argument(fmt::format("Unknown argument: {}",arg));
                               }
                           }
                        else
                           {// An input file
                            //i_files.emplace_back(arg);
                            const auto globbed = sys::glob(arg);
                            i_files.reserve(i_files.size() + globbed.size());
                            i_files.insert(i_files.end(), globbed.begin(), globbed.end());
                           }
                        break;

                    case STS::GET_OPTS :
                        i_options.assign(arg); // Expecting something like "key1:val1,key2,key3:val3"
                        status = STS::SEE_ARG;
                        break;

                    case STS::GET_OUT :
                        i_output = arg; // Expecting a path
                        if( !fs::exists(i_output) )
                           {
                            throw std::invalid_argument(fmt::format("Output path doesn't exists: {}",arg));
                            //const bool ok = fs::create_directories(i_output);
                           }
                        i_output_isdir = fs::is_directory(i_output);
                        if( !i_output_isdir )
                           {
                            throw std::invalid_argument(fmt::format("Combine into existing output file not yet supported: {}",arg));
                           }
                        status = STS::SEE_ARG;
                        break;
                   }
               } // each argument
           }
        catch( std::exception& e)
           {
            throw std::invalid_argument(e.what());
           }
       }

    static void print_help() noexcept
       {
        std::cout << "\nThis is a conversion utility between these formats:\n"
                     "    \"*.h\" Sipro #defines file\n"
                     "    \"*.pll\" LogicLab3 library file\n"
                     "    \"*.plclib\" LogicLab5 library file\n"
                     "Sipro *.h files resemble a c-like header with #define directives.\n"
                     "LogicLab files are xml containers of IEC 61131-3 ST code.\n"
                     "The supported transformations are:\n"
                     "    \"*.h\" -> \"*.pll\", \"*.plclib\"\n"
                     "    \"*.pll\" -> \"*.plclib\"\n"
                     "\n";
       }

    static void print_usage() noexcept
       {
        std::cerr << "\nUsage:\n"
                     "   llconv -fussy -verbose -options schemaver:2.8 path/to/*.pll -output path/\n"
                     "       -fussy (Handle issues as blocking errors)\n"
                     "       -help (Just print help info and abort)\n"
                     "       -options (LogicLab plclib schema version)\n"
                     "            schema-ver:<num> (Indicate a schema version for LogicLab plclib output)\n"
                     "       -output <path> (Set output directory or file)\n"
                     "       -sort (Order objects by name)\n"
                     "       -verbose (Print more info on stdout)\n"
                     "\n";
       }

    const auto& files() const noexcept { return i_files; }
    const auto& output() const noexcept { return i_output; }
    bool output_isdir() const noexcept { return i_output_isdir; }
    bool fussy() const noexcept { return i_fussy; }
    bool sort() const noexcept { return i_sort; }
    bool verbose() const noexcept { return i_verbose; }
    const str::keyvals& options() const noexcept { return i_options; }

 private:
    std::vector<fs::path> i_files;
    fs::path i_output = ".";
    bool i_output_isdir = false; // Cached result
    bool i_fussy = false;
    bool i_sort = false;
    bool i_verbose = false;
    str::keyvals i_options; // Conversion and writing options
};




//---------------------------------------------------------------------------
// Import a file
template<typename F> void import_file(F parsefunct, const fs::path& pth, plcb::Library& lib, const Arguments& args, std::vector<std::string>& issues)
{
    const std::string in_path{pth.string()};

    sys::MemoryMappedFile in_file_buf(in_path); // Do not deallocate until the end!

    // Show file name and size
    if( args.verbose() )
       {
        std::cout << "Processing " << in_path;
        std::cout << " (size: ";
        if(in_file_buf.size()>1048576) std::cout << in_file_buf.size()/1048576 << "MB)\n";
        else if(in_file_buf.size()>1024) std::cout << in_file_buf.size()/1024 << "KB)\n";
        else std::cout << in_file_buf.size() << "B)\n";
       }

    // Parse
    std::vector<std::string> parse_issues;
    try{
        parsefunct(in_file_buf.as_string_view(), lib, parse_issues, args.fussy());
       }
    catch( dlg::parse_error& e)
       {
        sys::edit_text_file( in_path, e.pos() );
        throw;
       }
    if(args.verbose()) std::cout << "    " << lib.to_str() << '\n';

    // Handle parsing issues
    if( !parse_issues.empty() )
       {
        // Append to overall issues list
        issues.push_back( fmt::format("____Parsing of {}",in_path) );
        issues.insert(issues.end(), parse_issues.begin(), parse_issues.end());
        // Log in a file
        const std::string log_file_path{ str::replace_extension(in_path, ".log") };
        sys::file_write log_file_write( log_file_path );
        log_file_write << sys::human_readable_time_stamp() << '\n';
        log_file_write << "[Parse log of "sv << in_path << "]\n"sv;
        for(const std::string& issue : parse_issues)
           {
            log_file_write << "[!] "sv << issue << '\n';
           }
        sys::launch( log_file_path );
       }

    // Check/manipulate the result
    lib.check(); // throws if something's wrong
    if( args.sort() )
       {
        //if( args.verbose() ) std::cout << "Sorting lib " << lib.name() << '\n';
        lib.sort();
       }

    //auto [ctime, mtime] = sys::get_file_dates(in_path);
    //std::cout << "pll created:" << sys::human_readable_time_stamp(ctime) << " modified:" << sys::human_readable_time_stamp(mtime) << '\n';
    //lib.set_dates(ctime, mtime);
}

//---------------------------------------------------------------------------
// Write PLC library to plclib format
inline void write_plclib(const plcb::Library& lib, const fs::path& inpth, const Arguments& args)
{
    //if( args.output_isdir() )
    //   {// Create a 'plclib' file in the output directory
        const fs::path opth{ args.output() / str::replace_extension(inpth.filename().string(), ".plclib") };
        const std::string out_path{ opth.string() };
        if(args.verbose()) std::cout << "    " "Writing to: "  << out_path << '\n';
        sys::file_write out_file_write( out_path );
        plclib::write(out_file_write, lib, args.options());
    //   }
    //else
    //   {// Combine in a single 'plcprj' file
    //    throw std::runtime_error(fmt::format("Combine into existing plcprj file {} not yet supported", args.output().string()));
    //   }
}

//---------------------------------------------------------------------------
// Write PLC library to pll format
inline void write_pll(const plcb::Library& lib, const fs::path& inpth, const Arguments& args)
{
    //if( args.output_isdir() )
    //   {// Create a 'plclib' file in the output directory
        const fs::path opth{ args.output() / str::replace_extension(inpth.filename().string(), ".pll") };
        const std::string out_path{ opth.string() };
        if(args.verbose()) std::cout << "    " "Writing to: "  << out_path << '\n';
        sys::file_write out_file_write( out_path );
        pll::write(out_file_write, lib, args.options());
    //   }
    //else
    //   {// Combine in a single 'pll' file
    //    throw std::runtime_error(fmt::format("Combine into existing pll file {} not yet supported", args.output().string()));
    //   }
}


//---------------------------------------------------------------------------
int main( int argc, const char* argv[] )
{
    std::ios_base::sync_with_stdio(false); // Better performance

    try{
        Arguments args(argc, argv);
        std::vector<std::string> issues;

        if( args.verbose() )
           {
            std::cout << "**** llconv (" << __DATE__ << ") ****\n"; // sys::human_readable_time_stamp()
            std::cout << "Running in: " << fs::current_path().string() << '\n';
           }

        // Check input files, divide them in h and pll
        if( args.files().empty() )
           {
            throw std::invalid_argument("No files passed");
           }
        std::vector<fs::path> h_files, pll_files;
        h_files.reserve( args.files().size() );
        pll_files.reserve( args.files().size() );
        for( const auto& pth : args.files() )
           {
            // Recognize by file extension
            const std::string ext {str::tolower(pth.extension().string())};
            if( ext == ".pll" )
               {
                pll_files.push_back(pth);
               }
            else if( ext == ".h" )
               {
                h_files.push_back(pth);
               }
            else
               {
                const std::string msg = fmt::format("Unhandled extension {} of {}"sv, ext, pth.filename().string());
                if(args.fussy()) throw std::runtime_error(msg);
                else issues.push_back(msg);
               }
           }

        // h -> pll,plclib
        for( const auto& pth : h_files )
           {
            plcb::Library lib( pth.stem().string() );
            import_file(h::parse, pth, lib, args, issues);
            write_pll(lib, pth, args);
            write_plclib(lib, pth, args);
           }

        // pll -> plclib
        for( const auto& pth : pll_files )
           {
            plcb::Library lib( pth.stem().string() );
            import_file(pll::parse, pth, lib, args, issues);
            write_plclib(lib, pth, args);
           }

        if( issues.size()>0 )
           {
            std::cerr << "[!] " << issues.size() << " issues found\n";
            for( const auto& issue : issues )
               {
                std::cerr << "    " << issue << '\n';
               }
            return -1;
           }
        return 0;
       }

    catch(std::invalid_argument& e)
       {
        std::cerr << "!! " << e.what() << '\n';
        Arguments::print_usage();
       }

    catch(std::exception& e)
       {
        std::cerr << "!! Error: " << e.what() << '\n';
       }

    return -1;
}
