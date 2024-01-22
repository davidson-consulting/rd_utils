#pragma once

#include <rd_utils/utils/config/_.hh>
#include <rd_utils/utils/lexer.hh>

namespace rd_utils::utils::toml {

  std::shared_ptr<config::ConfigNode> parse (const std::string & content);
  std::shared_ptr<config::ConfigNode> parseFile (const std::string & content);

  /**
   * Dump toml to string
   */
  std::string dump (const config::ConfigNode & node);

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

    /**
     * Dump a config in toml format
     */
    void dump (std::stringstream & ss, const config::ConfigNode & node, bool global = true, int indent = 0);

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

  private :

    void dumpDict (std::stringstream & ss, const config::Dict & d, bool global, int indent);

    void dumpSuperDict (std::stringstream & ss, const std::string & base, const config::Dict & d);

    void dumpGlobalDict (std::stringstream & ss, const config::Dict & d, int indent);

    void dumpLocalDict (std::stringstream & ss, const config::Dict & d, int indent);

    void dumpArray (std::stringstream & ss, const config::Array & a, int indent);

    void dumpString (std::stringstream & ss, const config::String & s);

    void dumpInt (std::stringstream & ss, const config::Int & i);

    void dumpBool (std::stringstream & ss, const config::Bool & b);

    void dumpFloat (std::stringstream & ss, const config::Float & f);

  } ;

}
