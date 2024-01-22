#pragma once

#include <rd_utils/utils/config/_.hh>
#include <rd_utils/utils/lexer.hh>

namespace rd_utils::utils::toml {

  std::shared_ptr <config::ConfigNode> parse (const std::string & content);
  std::shared_ptr <config::ConfigNode> parseFile (const std::string & content);


  class TomlParser {
  private :

    // The lexer of the configuration parser
    Lexer _lex;

  public:

    TomlParser ();

    /**
     * Parse a toml string
     * @returns: the config storing the toml content
     */
    std::shared_ptr<config::ConfigNode> parse (const std::string & content);

  private:

    /**
     * Parse the global part of the toml content
     */
    std::shared_ptr <config::ConfigNode> parseGlobal ();

    /**
     * Parse a value in the toml content
     */
    std::shared_ptr<config::ConfigNode> parseValue ();

    /**
     * Parse a dictionnary in the toml content
     */
    std::shared_ptr<config::ConfigNode> parseDict (bool glob = false);

    /**
     * Parse an array
     */
    std::shared_ptr <config::ConfigNode> parseArray ();

    /**
     * Parse a int value
     */
    std::shared_ptr <config::ConfigNode> parseInt ();

    /**
     * Parse a float value
     */
    std::shared_ptr <config::ConfigNode> parseFloat ();

    /**
     * Parse a string value
     */
    std::shared_ptr <config::ConfigNode> parseString (const Word & end);

    /**
     * Parse a bool value
     */
    std::shared_ptr <config::ConfigNode> parseBool ();


    /**
     * Read the next word asserting it is an excepted word within 'excepted'
     * @params:
     *    - canEof: if eof word is accepted
     */
    const Word& assertRead (const std::vector<std::string> & expected, bool canEof = false);

    /**
     * Read the next word if it is in 'values', else do nothing
     */
    Word readIf (const std::vector <std::string> & values);

  } ;

}
