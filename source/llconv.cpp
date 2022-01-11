/*  ---------------------------------------------
    llconv
    ©2021-2022 matteo.gattanini@gmail.com

    OVERVIEW
    ---------------------------------------------
    Conversion utility that can transform a
    Sipro definitions 'h' file into a LogicLab
    library file and a LogicLab3 'pll' file
    (IEC61131-3 ST library) into the new xml
    format 'plclib' for LogicLab5

    DEPENDENCIES:
    --------------------------------------------- */
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <stdexcept> // std::runtime_error
#include "system.hpp" // sys::*, fs::*
#include "string-utilities.hpp" // str::tolower
#include "logging.hpp" // dlg::error
//#include "h-parser.hpp" // h::*
#include "plc-elements.hpp" // plcb::*
#include "pll-parser.hpp" // pll::*
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
                GET_VER,
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
                            else if( arg=="-schemaver"sv )
                               {
                                status = STS::GET_VER; // Value expected
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

                    case STS::GET_VER :
                        i_schemaver = arg; // Expecting something like "2.8"
                        status = STS::SEE_ARG;
                        break;

                    case STS::GET_OUT :
                        i_output = arg; // Expecting a path
                        if( !fs::exists(i_output) )
                           {
                            throw std::invalid_argument(fmt::format("Output path doesn't exists: {}",arg));
                            //bool ok = fs::create_directories(i_output);
                           }
                        if( !fs::is_directory(i_output) )
                           {
                            throw std::invalid_argument(fmt::format("Combine to same output file not yet supported: {}",arg));
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
                     "The supported tranformations are:\n"
                     "    \"*.h\" -> \"*.pll\"\n"
                     "    \"*.pll\" -> \"*.plclib\"\n";
       }

    static void print_usage() noexcept
       {
        std::cerr << "\nUsage:\n"
                     "   llconv -fussy -verbose -schemaver 2.8 path/to/*.pll -output path/\n"
                     "       -fussy: Handle issues as blocking errors\n"
                     "       -output <path>: Set output directory or file (combining input files)\n"
                     "       -schemaver <num>: LogicLab plclib schema version\n"
                     "       -sort: Order objects by name\n"
                     "       -verbose: Print more info on stdout\n"
                     "       -help: Just print help info and abort\n";
       }

    const auto& files() const noexcept { return i_files; }
    const auto& output() const noexcept { return i_output; }
    bool fussy() const noexcept { return i_fussy; }
    bool sort() const noexcept { return i_sort; }
    bool verbose() const noexcept { return i_verbose; }
    plclib::Version schema_ver() const noexcept { return i_schemaver; }

 private:
    std::vector<fs::path> i_files;
    fs::path i_output = ".";
    bool i_fussy = false;
    bool i_sort = false;
    bool i_verbose = false;
    plclib::Version i_schemaver; // LogicLab plclib schema version
};




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
        if( args.files().empty() ) throw std::invalid_argument("No files passed");

        std::size_t n_issues = 0;
        for( const auto& in_file_path : args.files() )
           {
            // Open the input file
            const std::string ext {str::tolower(in_file_path.extension().string())};
            if( ext != ".pll" )
               {
                throw dlg::error("[!] Unhandled extension {} of file {}",in_file_path.extension().string(),in_file_path.filename().string());
               }

            sys::MemoryMappedFile in_file_buf(in_file_path.string()); // Do not deallocate until the end!

            // Show file name and size
            if( args.verbose() )
               {
                std::cout << "Processing " << in_file_path.string();
                std::cout << " (size: ";
                if(in_file_buf.size()>1048576) std::cout << in_file_buf.size()/1048576 << "MB)\n";
                else if(in_file_buf.size()>1024) std::cout << in_file_buf.size()/1024 << "KB)\n";
                else std::cout << in_file_buf.size() << "B)\n";
               }

            // Parse
            plcb::Library lib( in_file_path.stem().string() );
            std::vector<std::string> parse_issues;
            try{
                pll::parse(in_file_buf.as_string_view(), lib, parse_issues, args.fussy());
               }
            catch( dlg::parse_error& e)
               {
                sys::edit_text_file( in_file_path.string(), e.pos() );
                throw;
               }
            if(args.verbose()) std::cout << "    " << lib.to_str() << '\n';

            // Log parsing issues
            const std::string log_file_path{ sys::replace_extension(in_file_path.string(), ".log") };
            sys::delete_file( log_file_path );
            if( !parse_issues.empty() )
               {
                n_issues += parse_issues.size();
                sys::file_write log_file_write( log_file_path );
                log_file_write << '[' << sys::human_readable_time_stamp() << " Parse log of "sv << in_file_path.string() << ']' << '\n';
                for(const std::string& issue_txt : parse_issues)
                   {
                    log_file_write << "[!] "sv << issue_txt << '\n';
                   }
                sys::launch( log_file_path );
               }

            lib.check(); // throws if something wrong

            if( args.sort() )
               {
                //if( args.verbose() ) std::cout << "Sorting lib " << lib.name() << '\n';
                lib.sort();
               }

            //auto [ctime, mtime] = sys::get_file_dates(in_file_path.string());
            //std::cout << "pll created:" << sys::human_readable_time_stamp(ctime) << " modified:" << sys::human_readable_time_stamp(mtime) << '\n';
            //lib.set_dates(ctime, mtime);

            // Create output file
            if( fs::is_directory( args.output() ) )
               {// Create a 'plclib' file in the output directory
                const fs::path out_file_path{ args.output() / sys::replace_extension(in_file_path.filename().string(), ".plclib") };
                if(args.verbose())  std::cout << "    " "Writing to: "  << out_file_path.string() << '\n';
                sys::file_write out_file_write( out_file_path.string() );
                plclib::write(out_file_write, lib, args.schema_ver());
               }
            else
               {// Combine libraries in a single 'plcprj' file
                throw dlg::error("[!] Combine to plcprj file {} not yet supported", args.output().string());
               }
           } // each input file

        if( n_issues>0 )
           {
            std::cerr << "[!] " << n_issues << " issues found\n";
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
