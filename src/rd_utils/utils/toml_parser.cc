#include "toml_parser.hh"
#include <fstream>
#include "error.hh"
#include <sstream>
#include "files.hh"
#include "print.hh"

namespace rd_utils::utils::toml {

  using namespace config;

  std::shared_ptr <config::ConfigNode> parse (const std::string & content) {
    TomlParser p;
    return p.parse (content);
  }

  std::shared_ptr <config::ConfigNode> parseFile (const std::string & filename) {
    std::ifstream t(filename);
    if (!t.is_open ()) throw FileError ("File not found or permission denied : " + filename);

    std::stringstream buffer;
    buffer << t.rdbuf();

    return parse (buffer.str ());
  }

  TomlParser::TomlParser () {}

  std::shared_ptr <config::ConfigNode> TomlParser::parse (const std::string & content) {
    this-> _lex = Lexer (content,
                         { "[", "]", "{", "}", "=", ",", "'", "\"", " ", "\n", "\t", "\r", "#" },
                         { " ", "\t", "\n", "\r" },
                         { {"#","\n"} });

    return this-> parseGlobal ();
  }

  std::shared_ptr<config::ConfigNode> TomlParser::parseGlobal () {
    std::shared_ptr <config::Dict> glob = std::make_shared<Dict> ();
    while (true) {
      if (this-> assertRead ({"["}, true).str == "[") {
        auto name = this-> _lex.next ();
        std::vector <std::string> names = tokenize (name.str, {"."}, {".", " ", "\t", "\n", "\r"});

        this-> assertRead ({"]"}, false);

        auto value = this-> parseDict (true);
        auto currentName = names [0];
        auto currentDict = glob;

        if (names.size () != 1) {
          for (uint32_t i = 1 ; i < names.size () ; i++) {
            auto innerD = currentDict-> getInDic (currentName);
            if (innerD == nullptr) {
              auto nDic = std::make_shared<Dict> ();
              currentDict-> insert (currentName, nDic);
              currentDict = nDic;
            } else currentDict = innerD;

            currentName = names [i];
          }
        }

        currentDict-> insert (currentName, value);
      } else break;
    }

    return glob;
  }

  std::shared_ptr<ConfigNode> TomlParser::parseDict (bool glob) {
    std::shared_ptr<Dict> d = std::make_shared<Dict> ();

    while (true) {
      auto name = this-> _lex.next ();
      if (name.eof || name.str == "[") {
        if (!glob) throw LexerError (name.line, "expecting identifier or '}', not " + name.str);
        if (!name.eof) this-> _lex.rewind ();
        break;
      }

      this-> assertRead ({"="});
      auto value = this-> parseValue ();
      d-> insert (name.str, value);

      if (!glob) {
        if (this-> assertRead ({"}", ","}).str == "}") break;
      }
    }

    return d;
  }

  std::shared_ptr<ConfigNode> TomlParser::parseArray () {
    std::shared_ptr <Array> arr = std::make_shared<Array> ();
    if (this-> readIf ({"]"}).str != "]") {
      while (true) {
        auto value = this-> parseValue ();
        arr-> insert (value);
        if (this-> assertRead ({",", "]"}).str == "]") break;
      }
    }

    return arr;
  }

  std::shared_ptr<ConfigNode> TomlParser::parseInt () {
    int32_t i = std::stoi (this-> _lex.next ().str);
    return std::make_shared<Int> (i);
  }

  std::shared_ptr<ConfigNode> TomlParser::parseFloat () {
    int32_t i = std::stof (this-> _lex.next ().str);
    return std::make_shared<Float> (i);
  }

  std::shared_ptr<ConfigNode> TomlParser::parseString (const Word & end) {
    std::stringstream ss;
    bool escaping = false;
    for (;;) {
      auto n = this-> _lex.directNext ();

      if (n.eof) throw LexerError (end.line, "Unterminated string literal");
      if (n.str == end.str && !escaping) break;
      ss << n.str;

      if (!escaping && n.str == "\\") {
        escaping = true;
      } else escaping = false;
    }

    return std::make_shared<String> (ss.str ());
  }

  std::shared_ptr<ConfigNode> TomlParser::parseValue () {
    auto begin = this-> _lex.next ();
    if (begin.str  == "{") {
      return this-> parseDict (false);
    }
    else if (begin.str == "[") {
      return this-> parseArray ();
    }
    else if (begin.str == "'" || begin.str == "\"") {
      return this-> parseString (begin);
    }
    else if (begin.str == "false") {
      return std::make_shared <Bool> (false);
    }
    else if (begin.str == "true") {
      return std::make_shared <Bool> (true);
    }
    else {
      this-> _lex.rewind ();
      if (begin.str.find (".") != std::string::npos) {
        return this-> parseFloat ();
      } else {
        return this-> parseInt ();
      }
    }
  }

  const Word& TomlParser::assertRead (const std::vector<std::string> & value, bool canEof) {
    std::stringstream ss;

    auto & next = this-> _lex.next ();
    if (canEof && next.eof) return next;

    int i = 0;
    for (auto & it : value) {
      if (next.str == it) return next;
      else {
        if (i != 0) ss << ", ";
        ss << "'" << it << "'";
        i += 1;
      }
    }

    throw LexerError  (next.line, "read '" + next.str + "' when expecting {" + ss.str () + "} at line : " + std::to_string (next.line));
  }

  Word TomlParser::readIf (const std::vector <std::string> & value) {
    auto & next = this-> _lex.next ();
    if (next.eof) return next;
    for (auto & it : value) {
      if (next.str == it) return next;
    }

    this-> _lex.rewind ();
    return {.str = "", .line = 0, .col = 0, .eof = true};
  }



}
