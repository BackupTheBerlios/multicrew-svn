#ifndef CLI_PARSER_HPP
#define CLI_PARSER_HPP
/*------------------------------------------------------------------------------

  Author:    Andy Rushton
  Copyright: (c) Andy Rushton, 2004
  License:   BSD License, see ../docs/license.html

  A subsystem for managing command-line parsing, including using INI files to
  control the default options.

  ------------------------------------------------------------------------------*/
#include "os_fixes.hpp"
#include <string>
#include <stdexcept>
#include "error_handler.hpp"
#include "ini_manager.hpp"

////////////////////////////////////////////////////////////////////////////////
// declarations

// enum to define the basic behaviour of an argument
//   - a switch is an option with no value but which can be switched on or off e.g. -help and -nohelp
//   - a value is an option followed by a value e.g. -output results.txt
//       (a default value can be removed by using the option as a negated switch e.g. -nooutput)
//   - command-line values (i.e. any strings not preceded by '-') are treated internally as an option with no name and must be values
enum cli_kind_t {cli_switch_kind, cli_value_kind};

// the mode controls the behaviour if an option appears more than once in either the command-line or the ini files
//   - a single mode option overrides all previous values so will only be found once in the parsed result
//   - a multiple mode option can be repeated to define multiple values, but overrides values from ini files
//   - a cumulative mode option is a multiple mode option which keeps ini file values as well
enum cli_mode_t {cli_single_mode, cli_multiple_mode, cli_cumulative_mode};

// There are two structures used for defining command-line parameters
//  (1) a C struct which is used in a C array - this is used for declaring command-line parameters in a static declaration
//  (2) a C++ class which is used in an STL vector - this is used for building command-line parameters within code

// The C struct for definitions
struct cli_definition_t
{
  // the name of the option, e.g. "help"
  const char* m_name;
  // the kind of the option, e.g. cli_switch_kind
  cli_kind_t m_kind;
  // the mode e.g. cli_single_mode
  cli_mode_t m_mode;
  // the mnemonic for the message giving usage information for this option
  const char* m_message;
  // built-in default value - null if not present
  const char* m_default;
};

// The C array of the C struct. The array must be terminated by END_CLI_DEFINITIONS.
typedef cli_definition_t cli_definitions_t [];
#define END_CLI_DEFINITIONS {0,cli_switch_kind,cli_single_mode,"",0}

// The C++ class for definitions
class cli_definition
{
public:
  // the name of the option, e.g. "help"
  std::string m_name;
  // the kind of the option, e.g. switch_kind
  cli_kind_t m_kind;
  // the mode e.g. xingle_mode
  cli_mode_t m_mode;
  // the mnemonic for the message giving usag
  std::string m_message;
  // built-in default value - empty string if not present
  std::string m_default;

  // constructor that allows a definition to be created in one line
  cli_definition(const std::string& name, cli_kind_t kind, cli_mode_t mode, 
                 const std::string& message, const std::string& default_value = std::string()) : 
    m_name(name), m_kind(kind), m_mode(mode), m_message(message), m_default(default_value) {}
};

// The C++ vector of the C++ class
typedef std::vector<cli_definition> cli_definitions;

//////////////////////////////////////////////////////////////////////////////
// exceptions that can be thrown by the CLI parser
// they are all derivatives of std::logic_error because all errors are predictable by code inspection
// a correct program will never throw an exception

// thrown if a command-line argument is accessed with the wrong mode - i.e. attempt to get the value of a switch
class cli_mode_error : public std::invalid_argument
{
public:
  cli_mode_error(const std::string& arg) : std::invalid_argument(arg) {}
  ~cli_mode_error(void) throw() {}
};

// similar to std::out_of_range thrown for using an index out of range
class cli_index_error : public std::out_of_range
{
public:
  cli_index_error(const std::string& arg) : std::out_of_range(arg) {}
  ~cli_index_error(void) throw() {}
};

// similar to std::invalid_argument - thrown for passing an illegal argument to a method
class cli_argument_error : public std::invalid_argument
{
public:
  cli_argument_error(const std::string& arg) : std::invalid_argument(arg) {}
  ~cli_argument_error(void) throw() {}
};

////////////////////////////////////////////////////////////////////////////////

class cli_parser
{
public:
  // Type definitions map the global type names onto convenient scoped names

  typedef cli_kind_t kind_t;
  typedef cli_mode_t mode_t;
  typedef cli_definition_t definition_t;
  typedef cli_definitions_t definitions_t;
  typedef cli_definition definition;
  typedef cli_definitions definitions;

  ////////////////////////////////////////////////////////////////////////////////
  // Methods

  // various constructors

  // you have a choice of either creating an uninitialised CLI parser and then
  // calling separate functions to set it up or of calling one of the
  // composite constructors. However, you must set up the error handler in the
  // constructor.

  // set up the parser with its error handler
  // defer everything else
  cli_parser(error_handler& errors)
    throw();

  // constructors using the C definitions_t structure

  // set up the parser with the error handler and define all the command-line options
  // defer default values and parameter parsing
  cli_parser(cli_definitions_t, error_handler& errors)
    throw(cli_mode_error);
  // set up the parser with the error handler and define all the command-line options and their default from the ini files
  // defer parameter parsing
  cli_parser(cli_definitions_t, const ini_manager& defaults, const std::string& ini_section, error_handler& errors)
    throw(cli_mode_error);
  // set up the parser with the error handler and define all the command-line options
  // no ini files used for default values, so only built-in defaults supported
  // then parse the command line
  cli_parser(char* argv[], cli_definitions_t, error_handler& errors)
    throw(cli_mode_error,error_handler_id_error,error_handler_format_error);
  // set up the parser with the error handler and define all the command-line options and their default from the ini files
  // then parse the command line
  cli_parser(char* argv[], cli_definitions_t, const ini_manager& defaults, const std::string& ini_section, error_handler& errors)
    throw(cli_mode_error,error_handler_id_error,error_handler_format_error);

  // constructors using the C++ definitions structure

  // set up the parser with the error handler and define all the command-line options from a C array of structs
  // defer default values and parameter parsing
  cli_parser(cli_definitions, error_handler& errors)
    throw(cli_mode_error);
  // set up the parser with the error handler and define all the command-line options and their default from the ini files
  // defer parameter parsing
  cli_parser(cli_definitions, const ini_manager& defaults, const std::string& ini_section, error_handler& errors)
    throw(cli_mode_error);
  // set up the parser with the error handler and define all the command-line options
  // no ini files used for default values, so only built-in defaults supported
  // then parse the command line
  cli_parser(char* argv[], cli_definitions, error_handler& errors)
    throw(cli_mode_error,error_handler_id_error,error_handler_format_error);
  // set up the parser with the error handler and define all the command-line options and their default from the ini files
  // then parse the command line
  cli_parser(char* argv[], cli_definitions, const ini_manager& defaults, const std::string& ini_section, error_handler& errors)
    throw(cli_mode_error,error_handler_id_error,error_handler_format_error);

  ~cli_parser(void)
    throw();

  // the separate functions for initialising the parser in steps. These are
  // declared in the order of use. Firts, add definitions of command-line
  // arguments. Then optionally load default values from ini files, then
  // finally parse the command line.

  // add a set of C definitions. The definitions will be given ID codes from 0 to the number of elements - 1 in the array
  void add_definitions(cli_definitions_t)
    throw(cli_mode_error);
  // add a single C definition, returning the ID code for it
  unsigned add_definition(const definition_t&)
    throw(cli_mode_error,cli_argument_error);
  // add a set of C++ definitions. The definitions will be given ID codes from 0 to the number of elements - 1 in the array
  void add_definitions(cli_definitions)
    throw(cli_mode_error);
  // add a single C++ definition, returning the ID code for it
  unsigned add_definition(const definition&)
    throw(cli_mode_error);

  // All definitions have an optional built-in default value which is stored
  // in the definition types above. However, these can optionally be
  // overridden by a value from an ini file. If you want this functionality,
  // call this function. If you don't want ini file handling, simply don't
  // call it. The values will be searched for only in the named section of the
  // ini file (sections are labelled by e.g. [vassemble]), so in this case you
  // would specify the section name as "vassemble" (exclude the brackets).
  void set_defaults(const ini_manager& defaults, const std::string& ini_section)
    throw();

  // the final stage of initialisation is to read the command-line and extract
  // the values from it. If parse errors are found, this will report the
  // errors using the error handler and return false.
  bool parse(char* argv[])
    throw(cli_argument_error,error_handler_id_error,error_handler_format_error);

  // test for whether the CLI parser is still valid (no errors have happened) after the initialisation phase
  bool valid(void)
    throw();

  // iteration functions avoiding the use of iterators. Just loop through the
  // arguments from 0 to size()-1 and use the index of the loop to interrogate
  // the command-line for the value at that position.

  // the number of values to read, indexed 0 to size()-1
  unsigned size(void) const
    throw();

  // the argument name
  std::string name(unsigned i) const
    throw(cli_index_error);
  // the argument ID, that is, the offset into the original definitions
  unsigned id(unsigned i) const
    throw(cli_index_error);

  // the kind (switch or value) and short-cut tests for the different kinds
  cli_kind_t kind(unsigned i) const
    throw(cli_index_error);
  bool switch_kind(unsigned i) const
    throw(cli_index_error);
  bool value_kind(unsigned i) const
    throw(cli_index_error);

  // the mode (single, multiple, cumulative) and short-cut tests for the different modes
  // - you rarely need to know this since it mainly controls the parsing
  cli_mode_t mode(unsigned i) const 
    throw(cli_index_error);
  bool single_mode(unsigned i) const
    throw(cli_index_error);
  bool multiple_mode(unsigned i) const
    throw(cli_index_error);
  bool cumulative_mode(unsigned i) const
    throw(cli_index_error);

  // get the switch's value, but only if the value is of switch kind
  bool switch_value(unsigned i) const
    throw(cli_mode_error,cli_index_error);

  // get the option's value, but only if it is of value kind
  std::string string_value(unsigned i) const
    throw(cli_mode_error,cli_index_error);

  // print the usage report - typically in response to the -help switch being on
  void usage(void) const
    throw(std::runtime_error);

private:
  friend class cli_parser_data;
  smart_ptr<cli_parser_data> m_data;
};

#endif
