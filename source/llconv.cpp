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
#include "format_string.hpp" // fmtstr::parse_error
#include "h-parser.hpp" // h::*
#include "pll-parser.hpp" // pll::*
#include "plc-elements.hpp" // plcb::*
#include "plclib-writer.hpp" // plclib::write
#include "pll-writer.hpp" // pll::write

using namespace std::literals; // "..."sv

//#define PLL_TEST // Check *.pll parser and writer


/////////////////////////////////////////////////////////////////////////////
class Arguments final
{
 public:
    Arguments(int argc, const char* argv[])
       {
        //std::vector<std::string> args(argv+1, argv+argc); for( std::string& arg : args )
        // Expecting pll file paths
        i_files.reserve( static_cast<std::size_t>(argc) );
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
                        if( arg.length()>1 && arg[0]=='-' )
                           {// A command switch!
                            // Skip hyphen, tolerate also doubled ones
                            const std::size_t skip = arg[1]=='-' ? 2 : 1;
                            const std::string_view swtch{ arg.data()+skip, arg.length()-skip};
                            if( swtch=="fussy"sv )
                               {
                                i_fussy = true;
                               }
                            else if( swtch=="verbose"sv || swtch=="v"sv )
                               {
                                i_verbose = true;
                               }
                            else if( swtch=="clear"sv )
                               {
                                i_clear = true;
                               }
                            else if( swtch=="options"sv )
                               {
                                status = STS::GET_OPTS; // stringlist expected
                               }
                            else if( swtch=="output"sv || swtch=="o"sv )
                               {
                                status = STS::GET_OUT; // path expected
                               }
                            else if( swtch=="help"sv || swtch=="h"sv )
                               {
                                print_help();
                                throw std::invalid_argument("Aborting after printing help");
                               }
                            else
                               {
                                throw std::invalid_argument(fmt::format("Unknown command switch: {}",swtch));
                               }
                           }
                        else
                           {// Probably an input file
                            //i_files.emplace_back(arg);
                            // Support environment variables? Nah, not very useful, they're already been expanded
                            //const fs::path path = sys::expand_env_variables( std::string(arg) );
                            // Support globbing (just in file name)
                            const auto in_paths = sys::file_glob( fs::path(arg) );
                            if( in_paths.empty() )
                               {
                                throw std::invalid_argument(fmt::format("File(s) not found: {}",arg));
                               }
                            i_files.reserve(i_files.size() + in_paths.size());
                            i_files.insert(i_files.end(), in_paths.begin(), in_paths.end());
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
                     "LogicLab files are text containers of IEC 61131-3 ST code.\n"
                     "The supported transformations are:\n"
                     "    \"*.h\" -> \"*.pll\", \"*.plclib\"\n"
                     "    \"*.pll\" -> \"*.plclib\"\n"
                     "\n";
       }

    static void print_usage() noexcept
       {
        std::cerr << "\nUsage:\n"
                     "   llconv -fussy -verbose -options sort,schemaver:2.8 path/to/*.pll -output path/\n"
                     "       -clear (Delete existing files in output folder. Use with care!)\n"
                     "       -fussy (Handle issues as blocking errors)\n"
                     "       -help (Just print help info and abort)\n"
                     "       -options (LogicLab plclib schema version)\n"
                     "            schema-ver:<num> (Indicate a schema version for LogicLab plclib output)\n"
                     "            sort:<str> (Sort objects by criteria default:by-name)\n"
                     "       -output <path> (Set output directory or file)\n"
                     "       -verbose (Print more info on stdout)\n"
                     "\n";
       }

    const auto& files() const noexcept { return i_files; }
    const auto& output() const noexcept { return i_output; }
    bool output_isdir() const noexcept { return i_output_isdir; }
    bool output_isdefault() const noexcept { return i_output==i_default_output; }
    bool fussy() const noexcept { return i_fussy; }
    bool verbose() const noexcept { return i_verbose; }
    bool clear() const noexcept { return i_clear; }
    const str::keyvals& options() const noexcept { return i_options; }


 private:
    std::vector<fs::path> i_files;
    inline static const fs::path i_default_output = ".";
    fs::path i_output = i_default_output;
    bool i_output_isdir = false; // Cached result
    bool i_fussy = false;
    bool i_verbose = false;
    bool i_clear = false;
    str::keyvals i_options; // Conversion and writing options
};



//---------------------------------------------------------------------------
// Import a file
template<typename F> void parse_buffer(F parsefunct, const std::string_view buf, const fs::path& pth, const std::string& str_pth, plcb::Library& lib, const Arguments& args, std::vector<std::string>& issues)
{
    std::vector<std::string> parse_issues;
    try{
        parsefunct(buf, lib, parse_issues, args.fussy());
       }
    catch( fmtstr::parse_error& e)
       {
        sys::edit_text_file( str_pth, e.pos() );
        throw;
       }
    if(args.verbose()) std::cout << "    " << lib.to_str() << '\n';

    // Handle parsing issues
    if( !parse_issues.empty() )
       {
        // Append to overall issues list
        issues.push_back( fmt::format("____Parsing of {}",str_pth) );
        issues.insert(issues.end(), parse_issues.begin(), parse_issues.end());
        // Log in a file
        //const std::string log_file_path{ str::replace_extension(str_pth, ".log") }; // Same folder as input
        //const std::string log_file_path{ (fs::temp_directory_path() / pth.filename()).replace_extension(".log").string() }; // Temporary folder
        const std::string log_file_path{ (args.output() / pth.filename()).concat(".log").string() }; // In output folder
        sys::file_write log_file_write( log_file_path );
        log_file_write << sys::human_readable_time_stamp() << '\n';
        log_file_write << "[Parse log of "sv << str_pth << "]\n"sv;
        for(const std::string& issue : parse_issues)
           {
            log_file_write << "[!] "sv << issue << '\n';
           }
        sys::launch( log_file_path );
       }

    // Check the result
    lib.check(); // throws if something's wrong
    if( lib.is_empty() )
        {
         issues.push_back( fmt::format("{} generated an empty library",str_pth) );
        }

    // Manipulate the result
    //const auto [ctime, mtime] = sys::get_file_dates(str_pth);
    //std::cout << "created:" << sys::human_readable_time_stamp(ctime) << " modified:" << sys::human_readable_time_stamp(mtime) << '\n';
    //lib.set_dates(ctime, mtime);
    if( args.options().contains("sort")) // args.options().value_of("sort")=="name"
       {
        //if( args.verbose() ) std::cout << "Sorting lib " << lib.name() << '\n';
        lib.sort();
       }

}

//---------------------------------------------------------------------------
// Write PLC library to plclib format
void write_plclib(const plcb::Library& lib, const std::string& pth, const Arguments& args)
{
    //if( args.output_isdir() )
    //   {
        if(args.verbose()) std::cout << "    " "Writing to: "  << pth << '\n';
        sys::file_write out_file_write(pth);
        plclib::write(out_file_write, lib, args.options());
    //   }
    //else
    //   {// Combine in a single 'plcprj' file
    //    throw std::runtime_error(fmt::format("Combine into existing plcprj file {} not yet supported", args.output().string()));
    //   }
}

//---------------------------------------------------------------------------
// Write PLC library to pll format
void write_pll(const plcb::Library& lib, const std::string& pth, const Arguments& args)
{
    //if( args.output_isdir() )
    //   {
        if(args.verbose()) std::cout << "    " "Writing to: "  << pth << '\n';
        sys::file_write out_file_write(pth);
        pll::write(out_file_write, lib, args.options());
    //   }
    //else
    //   {// Combine in a single 'pll' file
    //    throw std::runtime_error(fmt::format("Combine into existing pll file {} not yet supported", args.output().string()));
    //   }
}



#ifdef PLL_TEST
//---------------------------------------------------------------------------
// Una funzione di test
void test_pll(const std::string& fbasename, const plcb::Library& lib, const Arguments& args, std::vector<std::string>& issues)
{
    // Riscrivo come pll la libreria in ingresso...
    const fs::path out_pll_pth{ args.output() / fmt::format("{}-1.pll", fbasename) };
    const std::string out_pll_pth_str{ out_pll_pth.string() };
    write_pll(lib, out_pll_pth_str, args);
    // ...Lo rileggo generando una nuova libreria...
    const sys::MemoryMappedFile buf2(out_pll_pth_str);
    plcb::Library lib2( out_pll_pth.stem().string() );
    parse_buffer(pll::parse, buf2.as_string_view(), out_pll_pth, out_pll_pth_str, lib2, args, issues);
    // ...E la scrivo di nuovo
    const fs::path out_pll2_pth{ args.output() / fmt::format("{}-2.pll", fbasename) };
    write_pll(lib2, out_pll2_pth.string(), args);
}
#endif



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

        if( args.files().empty() )
           {
            throw std::invalid_argument("No files passed");
           }

        if( args.clear() && args.output_isdir() )
           {
            if( args.output_isdefault() )
               {
                if( args.verbose() )
                   {
                    std::cout << "Won't clear files in default output folder\n";
                   }
               }
            else
               {
                const auto removed_count = sys::remove_files_inside(args.output(), std::regex{R"-(^.*\.(?:log|pll|plclib)$)-"});
                if( args.verbose() )
                   {
                    std::cout << "Cleared " << removed_count << " files in " << args.output().string() << '\n';
                   }
                // Check uncleared files
                // Nota: nella directory di sviluppo posso avere ".gitignore"
                if( !fs::is_empty(args.output()) )
                   {
                    //issues.push_back("Output directory contains uncleared files");
                    for( const auto& elem : fs::directory_iterator(args.output()) )
                       {
                        if( !elem.path().filename().string().starts_with('.') )
                           {
                            issues.push_back(fmt::format("Uncleared file in output dir: {}", elem.path().string()));
                           }
                       }
                   }
               }
           }

        for( const auto& pth : args.files() )
           {
            // Prepare the file buffer
            // Note: Extension not recognized is an exceptional case,
            //       so there's nor arm to confidently open the file
            const std::string str_pth{pth.string()};
            const sys::MemoryMappedFile file_buf(str_pth); // Do not deallocate until the very end!

            // Show file name and size
            if( args.verbose() )
               {
                std::cout << "\nProcessing " << str_pth;
                std::cout << " (size: ";
                if(file_buf.size()>1048576) std::cout << file_buf.size()/1048576 << "MB)\n";
                else if(file_buf.size()>1024) std::cout << file_buf.size()/1024 << "KB)\n";
                else std::cout << file_buf.size() << "B)\n";
               }

            const std::string fbasename = pth.stem().string();
            plcb::Library lib( fbasename ); // This will refer to 'file_buf'!

            // Recognize by file extension
            const std::string ext {str::tolower(pth.extension().string())};
            if( ext == ".pll" )
               {// pll -> plclib
                parse_buffer(pll::parse, file_buf.as_string_view(), pth, str_pth, lib, args, issues);
              #ifdef PLL_TEST
                test_pll(fbasename, lib, args, issues);
              #else
                const fs::path out_plclib_pth{ args.output() / fmt::format("{}.plclib", fbasename) };
                write_plclib(lib, out_plclib_pth.string(), args);
              #endif
               }
            else if( ext == ".h" )
               {// h -> pll,plclib
                parse_buffer(h::parse, file_buf.as_string_view(), pth, str_pth, lib, args, issues);

                const fs::path out_pll_pth{ args.output() / fmt::format("{}.pll", fbasename) };
                write_pll(lib, out_pll_pth.string(), args);

                const fs::path out_plclib_pth{ args.output() / fmt::format("{}.plclib", fbasename) };
                write_plclib(lib, out_plclib_pth.string(), args);
               }
            else
               {
                const std::string msg = fmt::format("Unhandled extension {} of {}"sv, ext, pth.filename().string());
                if(args.fussy()) throw std::runtime_error(msg);
                else issues.push_back(msg);
               }
           }

        if( issues.size()>0 )
           {
            std::cerr << "[!] " << issues.size() << " issues found\n";
            for( const auto& issue : issues )
               {
                std::cerr << "    " << issue << '\n';
               }
            return 1;
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

    return 2;
}
