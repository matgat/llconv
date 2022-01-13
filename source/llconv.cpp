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
//#include "h-parser.hpp" // h::*
#include "pll-parser.hpp" // pll::*
#include "plc-elements.hpp" // plcb::*
#include "plclib-writer.hpp" // plclib::*

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
                        if( !fs::is_directory(i_output) )
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
                     "The supported transformations are:\n"
                     "    \"*.h\" -> \"*.pll\"\n"
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
    bool fussy() const noexcept { return i_fussy; }
    bool sort() const noexcept { return i_sort; }
    bool verbose() const noexcept { return i_verbose; }
    const str::keyvals& options() const noexcept { return i_options; }

 private:
    std::vector<fs::path> i_files;
    fs::path i_output = ".";
    bool i_fussy = false;
    bool i_sort = false;
    bool i_verbose = false;
    str::keyvals i_options; // Conversion and writing options
};


//---------------------------------------------------------------------------
// Perform the h -> pll conversion of a file
inline std::string convert_h(const fs::path& pth, const Arguments& args, std::vector<std::string>& issues)
{
    std::string out_path;
    return out_path;
}


//---------------------------------------------------------------------------
// Perform the pll -> plclib conversion of a file
inline void convert_pll(const fs::path& pth, const Arguments& args, std::vector<std::string>& issues)
{
    //std::string out_path;
    const std::string pll_path{pth.string()};

    sys::MemoryMappedFile in_file_buf(pll_path); // Do not deallocate until the end!

    // Show file name and size
    if( args.verbose() )
       {
        std::cout << "Processing " << pll_path;
        std::cout << " (size: ";
        if(in_file_buf.size()>1048576) std::cout << in_file_buf.size()/1048576 << "MB)\n";
        else if(in_file_buf.size()>1024) std::cout << in_file_buf.size()/1024 << "KB)\n";
        else std::cout << in_file_buf.size() << "B)\n";
       }

    // Parse
    plcb::Library lib( pth.stem().string() );
    std::vector<std::string> parse_issues;
    try{
        pll::parse(in_file_buf.as_string_view(), lib, parse_issues, args.fussy());
       }
    catch( dlg::parse_error& e)
       {
        sys::edit_text_file( pll_path, e.pos() );
        throw;
       }
    if(args.verbose()) std::cout << "    " << lib.to_str() << '\n';

    // Handle parsing issues
    if( !parse_issues.empty() )
       {
        // Append to overall issues list
        issues.push_back( fmt::format("____Parsing of {}",pll_path) );
        issues.insert(issues.end(), parse_issues.begin(), parse_issues.end());
        // Log in a file
        const std::string log_file_path{ str::replace_extension(pll_path, ".log") };
        sys::file_write log_file_write( log_file_path );
        log_file_write << sys::human_readable_time_stamp() << '\n';
        log_file_write << "[Parse log of "sv << pll_path << "]\n"sv;
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

    //auto [ctime, mtime] = sys::get_file_dates(pll_path);
    //std::cout << "pll created:" << sys::human_readable_time_stamp(ctime) << " modified:" << sys::human_readable_time_stamp(mtime) << '\n';
    //lib.set_dates(ctime, mtime);

    // Write the output
    if( fs::is_directory( args.output() ) )
       {
        // Create a 'plclib' file in the output directory
        const fs::path opth{ args.output() / str::replace_extension(pth.filename().string(), ".plclib") };
        const std::string out_path{ opth.string() };
        if(args.verbose())  std::cout << "    " "Writing to: "  << out_path << '\n';
        sys::file_write out_file_write( out_path );
        plclib::write(out_file_write, lib, args.options());
       }
    else
       {// Combine in a single 'plcprj' file
        throw std::runtime_error(fmt::format("[!] Combine into existing plcprj file {} not yet supported", args.output().string()));
       }

    //return out_path;
}



//---------------------------------------------------------------------------
int main( int argc, const char* argv[] )
{
    std::ios_base::sync_with_stdio(false); // Better performance

    try{
        Arguments args(argc, argv);

        if( args.verbose() )
           {
            std::cout << "**** llconv (" << __DATE__ << ") ****\n"; // sys::human_readable_time_stamp()
            std::cout << "Running in: " << fs::current_path().string() << '\n';
           }

        std::vector<std::string> issues;
        //auto notify_error = [&args,&issues](const std::string_view msg, auto&&... vals)
        //   {
        //    if(args.fussy()) throw std::runtime_error( fmt::format(msg, std::forward<auto>(vals)) );
        //    else issues.push_back( fmt::format(msg, vals...) );
        //   };
        // Ehmm, with a lambda I cannot use consteval format
        #define notify_error(...) \
           {\
            if(args.fussy()) throw std::runtime_error( fmt::format(__VA_ARGS__) );\
            else issues.push_back( fmt::format(__VA_ARGS__) );\
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
                notify_error("[!] Unhandled extension {} of {}"sv, ext, pth.filename().string());
               }
           }

        // h -> pll conversion
        if( h_files.size()>0 )
           {
            pll_files.reserve( pll_files.size() + h_files.size() );
            for( const auto& pth : h_files )
               {
                // I'll convert to 'plclib' also this new 'pll' created from 'h'
                pll_files.push_back( convert_h(pth, args, issues) );
               }
           }

        // pll -> plclib conversion
        for( const auto& pth : pll_files )
           {
            convert_pll(pth, args, issues);
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
