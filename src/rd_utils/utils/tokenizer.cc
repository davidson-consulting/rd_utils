#include <rd_utils/utils/tokenizer.hh>
#include <rd_utils/utils/range.hh>
#include <iostream>
#include <algorithm>

namespace rd_utils::utils {

  /**
   * ===================================================================================
   * ===================================================================================
   * ===============================     TOKENNODE     =================================
   * ===================================================================================
   * ===================================================================================
   * */

  TokenNode::TokenNode (char key, bool isToken, bool isSkip, const std::string & isComment) :
    _key (key)
    , _isToken (isToken)
    , _isSkip (isSkip)
    , _isComment (isComment != "")
    , _closing (isComment)
  {}

  void TokenNode::insert (const std::string & str, bool isSkip, const std::string & isComment) {
    if (str.length () == 0) {
      this-> _isToken = true;
      this-> _isSkip = isSkip;
      this-> _isComment = (isComment != "");
      this-> _closing = isComment;
      return;
    }

    for (uint32_t i = 0 ; i < this-> _keyHeads.size () ; i++) {
      if (this-> _keyHeads [i] == str [0]) {
        this-> _heads [i].insert (str.substr (1), isSkip, isComment);
        return;
      }
    }

    this-> _keyHeads.push_back (str [0]);
    TokenNode node (str [0]);
    node.insert (str.substr (1), isSkip, isComment);

    this-> _heads.push_back (node);
  }

  TokenResult TokenNode::len (const char * content, uint32_t len) const {
    if (len == 0) {
      if (this-> _isToken) {
        return {.len = 1, .skip = this-> _isSkip, .commentEnd = this-> _closing};
      } else return {.len = 0, .skip = false, .commentEnd = ""};
    }

    for (uint32_t i = 0 ; i < this-> _keyHeads.size () ; i++) {
      if (content [0] == this-> _keyHeads [i]) {
        auto sub = this-> _heads [i].len (content + 1, len - 1);
        if (sub.len != 0) {
          sub.len += 1;
          return sub;
        }
      }
    }

    if (this-> _isToken) {
      return {.len = 1, .skip = this-> _isSkip, .commentEnd = this-> _closing};
    }

    return {.len = 0, .skip = false, .commentEnd = ""};
  }

  char TokenNode::key () const {
    return this-> _key;
  }

  bool TokenNode::isSkip () const {
    return this-> _isSkip;
  }

  const std::string & TokenNode::isComment () const {
    return this-> _closing;
  }


  /**
   * ===================================================================================
   * ===================================================================================
   * ===============================     TOKENIZER     =================================
   * ===================================================================================
   * ===================================================================================
   * */

  Tokenizer::Tokenizer () {}

  void Tokenizer::insert (const std::string & token, bool isSkip, const std::string & isComment) {
    if (token.length () != 0) {
      for (uint32_t i = 0 ; i < this-> _keyHeads.size () ; i++) {
        if (this-> _keyHeads [i] == token [0]) {
          this-> _heads [i].insert (token.substr (1), isSkip, isComment);
          return;
        }
      }

      this-> _keyHeads.push_back (token [0]);
      TokenNode node (token [0]);
      node.insert (token.substr (1), isSkip, isComment);
      this-> _heads.push_back (node);
    }
  }

  TokenResult Tokenizer::next (const char * str, uint32_t len) const {
    if (len == 0) return {.len = 0, .skip = false, .commentEnd = ""};
    for (uint32_t i = 0 ; i < len ; i++) {
      auto index = this-> find (str [i]);
      if (index != this-> _keyHeads.size ()) {
        if (i != 0) return {.len = i, .skip = false, .commentEnd = ""}; // something else than a token

        auto sub = this-> _heads [index].len (str + 1, len - 1);
        if (sub.len != 0) { // There is token
          if (i == 0) { // found a token at the start of the string
            return sub;
          } else { // if sub.len != 0 && i != 0, then there is a token here, but there is something before it
            return {.len = i, .skip = false, .commentEnd = ""};
          }
        } // not a token, just started like one
      }
    }

    return {.len = 0, .skip = false, .commentEnd = ""};
  }

  TokenResult Tokenizer::next (const std::string & str) const {
    return this-> next (str.c_str (), str.length ());
  }


  uint32_t Tokenizer::find (char tok) const {
    for (uint32_t i = 0 ; i < this-> _keyHeads.size () ; i++) {
      if (tok == this-> _keyHeads [i]) return i;
    }
    return this-> _keyHeads.size ();
  }

  std::vector <std::string> tokenize (const std::string & str, const std::vector <std::string> & tokens, const std::vector <std::string> & skips) {
    Tokenizer t;
    for (auto & i : tokens) t.insert (i, false);
    for (auto & i : skips) t.insert (i, true);


    std::vector <std::string> result;
    const char * data = str.c_str ();
    uint32_t len = str.length ();
    while (len > 0) {
      auto res = t.next (data, len);
      if (res.len != 0) {
        if (!res.skip) {
          result.push_back (std::string (data, res.len));
        }

        data += res.len;
        len -= res.len;
      } else {
        break;
      }
    }

    if (len != 0) {
      result.push_back (std::string (data));
    }

    return result;
  }

}
