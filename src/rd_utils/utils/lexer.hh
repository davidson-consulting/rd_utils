#pragma once

#include "tokenizer.hh"
#include <map>

namespace rd_utils::utils {

  struct Word {
    std::string str;

    uint32_t line;

    uint32_t col;

    bool eof;
  };

  class Lexer {
  private:

    // The list of words in the lexer
    std::vector <Word> _words;

    // The word infos determined by the tokenizer
    std::vector <TokenResult> _wordInfos;

    // The cursor the current reading word
    uint32_t _wordCursor;

    // The position of the previous read words, to enable rewinding
    std::vector <uint32_t> _rewinders;

    // The endoffile word
    Word _eofWord;

    // True if this-> _wordCursor >= this-> _words.size ()
    bool _eof;

  public:

    /**
     * Empty lexer with no words
     */
    Lexer ();

    /**
     * @params:
     *    - content: the content to split
     *    - tokens: the list of tokens spliting the content
     *    - skipTokens: the list of tokens to skip when the lexer is in default mode
     *    - commentTokens: the list of tokens starting and ending a comment
     */
    Lexer (const std::string & content, const std::vector <std::string> & tokens, const std::vector <std::string> & skipTokens, const std::map<std::string, std::string> & commentTokens);

    /**
     * @returns: the next word in the string
     */
    const Word & next ();

    /**
     * @returns: the next word without taking into account skips, or comments
     */
    const Word & directNext ();

    /**
     * Read a string in the lexer
     * @params:
     *    - closing: the tokens ending the string
     *    - escaping: iif true take into account escaping chars (such as \")
     */
    std::string getString (const std::string & closing, bool escaping = true);

    /**
     * Rewind to a previous location
     * @params:
     *    - nb: the number of tokens to rewind
     */
    void rewind (uint32_t nb = 1);

    /**
     * Rewind to a previous cursor position
     * @params:
     *    - cursor: the cursor position to rewind to
     */
    void rewindTo (uint32_t cursor);

    /**
     * @returns: the word cursor
     * This can be usefull to go back quickly using a previous cursor position
     * @example:
     * =========
     * auto cursor = lex.getCursor ();
     * while (!end) {
     *     auto s = lex.next ();
     *     if (s.str == "") break;
     *     else if (s.str == "}") { end = true; }
     * }
     * if (!end) lex.rewindTo (cursor); // failed to read ending with '}' got back to cursor
     * =========
     */
    uint32_t getCursor () const;

  private:

    /**
     * Tokenize all the string and stores the result in the lexer
     */
    void readAll (const std::string & content, Tokenizer & tokenizer);

    /**
     * Move the word cursor to the next occurence of 'end'
     */
    void gotoNextToken (const std::string & end);

  };


}
