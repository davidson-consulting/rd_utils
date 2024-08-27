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

  std::string dump (const ConfigNode & node) {
    TomlParser p;
    std::stringstream ss;
    p.dump (ss, node, true, -1);
    return ss.str ();
  }

  TomlParser::TomlParser () {}

  std::shared_ptr <config::ConfigNode> TomlParser::parse (const std::string & content) {
    this-> _lex = Lexer (content,
                         { "[", "]", "{", "}", "=", ",", "'", "\"", " ", "\n", "\t", "\r", "#" },
                         { " ", "\t", "\n", "\r" },
                         { {"#","\n"} });

    return this-> parseGlobal ();
  }

  void TomlParser::dump (std::stringstream & ss, const ConfigNode & node, bool global, int indent) {
    match (node) {
      of (config::Dict, d) {
        if (indent == -1) this-> dumpSuperDict (ss, "", *d);
        else {
          this-> dumpDict (ss, *d, global, indent);
        }
      }
      elof (config::Array, a) {
        this-> dumpArray (ss, *a, indent);
      }
      elof (config::Int, i) {
        this-> dumpInt (ss, *i);
      }
      elof (config::Bool, b) {
        this-> dumpBool (ss, *b);
      }
      elof (config::Float, f) {
        this-> dumpFloat (ss, *f);
      }
      elof (config::String, s) {
        this-> dumpString (ss, *s);
      } elfo {
        throw ConfigError ();
      }
    }
  }

  std::shared_ptr<config::ConfigNode> TomlParser::parseGlobal () {
    std::shared_ptr <config::Dict> glob = std::make_shared<Dict> ();
    while (true) {
      if (this-> readIf ({"["}).str == "[") {
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
      } else {
        auto n = this-> _lex.next ();
        if (n.eof) break;
        else {
          this-> assertRead ({"="});
          auto value = this-> parseValue ();
          glob-> insert (n.str, value);
        }
      }
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
    auto l = this-> _lex.next ();
    int32_t i = std::stoi (l.str);
    return std::make_shared<Int> (i);
  }

  std::shared_ptr<ConfigNode> TomlParser::parseFloat () {
    float i = std::stof (this-> _lex.next ().str);
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


  void TomlParser::dumpDict (std::stringstream & ss, const config::Dict & dict, bool global, int indent) {
    if (global) {
      this-> dumpGlobalDict (ss, dict, 0);
    } else {
      this-> dumpLocalDict (ss, dict, indent);
    }
  }

  void TomlParser::dumpSuperDict (std::stringstream & ss, const std::string & baseName, const config::Dict & dict) {
    std::stringstream innerStream;
    std::stringstream superStream;
    for (auto & k : dict.getKeys ()) {
      match (dict [k]) {
        of (config::Dict, j) {
          bool others = false;
          for (auto & ik : j-> getKeys ()) {
            match ((*j)[ik]) {
              of (config::Dict, d) {}
              elfo { others = true; break; }
            }
          }

          if (others) {
            innerStream << "\n[" << baseName + k << "]\n";
            this-> dumpGlobalDict (innerStream, *j, 0);
          } else {
            this-> dumpSuperDict (innerStream, baseName + k + ".", *j);
          }
        } elof (config::ConfigNode, n) {
          superStream << k << " = ";
          this-> dump (superStream, *n, false, 0);
          superStream << "\n";
        } elfo {
          throw ConfigError ();
        }
      }
    }

    ss << superStream.str () << innerStream.str ();
  }

  void TomlParser::dumpGlobalDict (std::stringstream & ss, const config::Dict & d, int indent) {
    int z = 0;
    for (auto & k : d.getKeys ()) {
      if (z != 0) {
        for (int i = 0 ; i < indent ; i++) ss << " ";
      }

      ss << k << " = ";
      this-> dump (ss, d [k], false, indent + k.length () + 3);
      ss << std::endl;
      z += 1;
    }
  }

  void TomlParser::dumpLocalDict (std::stringstream & ss, const config::Dict & d, int indent) {
    ss << "{";
    int z = 0;
    for (auto & k : d.getKeys ()) {
      if (z != 0) { ss << ", "; }

      ss << k << " = ";
      this-> dump (ss, d [k], false, indent + k.length () + 3);
      z += 1;
    }
    ss << "}";
  }

  void TomlParser::dumpArray (std::stringstream & ss, const config::Array & a, int indent) {
    ss << "[";
    int z = 0;
    bool returned = false;
    for (uint32_t i = 0 ; i < a.getLen () ; i++) {
      if (z != 0) {
        ss << ", ";
        if (returned) {
          ss << "\n";
          for (int j = 0 ; j < indent + 1; j++) ss << " ";
        }
      }

      match (a [i]) {
        of (config::Array, ai) {
          returned = true;
        }
        elof (config::String, s) {
          returned = true;
        }
        elof (config::Dict, d) {
          returned = true;
        } fo;
      };

      this-> dump (ss, a [i], false, indent + 1);
      z += 1;
    }

    ss << "]";
  }

  void TomlParser::dumpString (std::stringstream & ss, const config::String & s) {
    ss << "\"" << s.getStr () << "\"";
  }

  void TomlParser::dumpInt (std::stringstream & ss, const config::Int & i) {
    ss << i.getI ();
  }

  void TomlParser::dumpBool (std::stringstream & ss, const config::Bool & b) {
    if (b.isTrue ()) ss << "true"; else ss << "false";
  }

  void TomlParser::dumpFloat (std::stringstream & ss, const config::Float & f) {
    ss << f.getF ();
  }


}
