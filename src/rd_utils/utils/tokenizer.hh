#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace rd_utils::utils {

  struct TokenResult {
    // The length of the token
    uint32_t len;

    // True if the token is skipable
    bool skip;

    // The comments end if the token is a comment starter
    std::string commentEnd;
  };

  class TokenNode {
  private:

    // The current value of the node
    char _key;

    // True if this is a complete token
    bool _isToken = false;

    // The list of continuation of the token
    std::vector <TokenNode> _heads;

    // The list of key heads
    std::vector <char> _keyHeads;

    // True if in addition to be a token, this is skippable
    bool _isSkip = false;

    // True if this tokens start a comments
    bool _isComment = false;

    // Applicable iif this-> _isComment, this is the token that can close the comment
    // e.g. _key = '#', _isComment = true, _closing = "\n"
    std::string _closing;

  public:

    /**
     * @params:
     *    - key: the current char of the token
     *    - isToken: iif true, this is closing a token
     *    - isSkip: iif true this token is skippable
     *    - isComment: iif something else than "", then it is a comment token, and it ends with 'isComment'
     */
    TokenNode (char key, bool isToken = false, bool isSkip = false, const std::string & isComment = "");

    /**
     * Insert a comment in the index tree
     * @params:
     *    - str: the rest to read to create a valid token
     */
    void insert (const std::string & str, bool isSkip, const std::string & isComment = "");

    /**
     * @returns: the length of the token at the beginning of the string content
     * @example:
     * =================
     * let mut node = Node::new ('+', isToken-> true);
     * node = node.insert ("=");
     * // Our grammar accept the tokens, "+" and "+="
     * assert (node.len ("+", 1).len == 1u64); // "+" are accepted
     * assert (node.len ("+=", 2).len == 2u64); // "+=" are accepted
     * assert (node.len (" +=", 3).len == 0u64); // " +=" are not accepted
     * assert (node.len ("+ and some rest", 15).len == 1u64); // " +" are accepted
     * =================
     */
    TokenResult len (const char * content, uint32_t len) const;

    /**
     * @returns: the key of the node
     */
    char key () const;

    /**
     * @returns: true iif the token is skipable
     */
    bool isSkip () const;

    /**
     * @returns: iif something else than "", then it is a comments token ending with the return value
     */
    const std::string & isComment () const;

  };
  
  /**
   * Cut a string into tokens
   */
  class Tokenizer {
  private:

    // The list of node heads
    std::vector <TokenNode> _heads;

    // The keys of the heads
    std::vector <char> _keyHeads;

  public:

    /**
     * Create an empty tokenizer
     */
    Tokenizer ();

    /**
     * Insert a new token in the tokenizer
     */
    void insert (const std::string & token, bool isSkip = false, const std::string & isComment = "");

    /**
     * @returns: the next token information in the string
     * @warning: assuming the string is a valid null terminated string
     */
    TokenResult next (const char * str, uint32_t len) const;

    /**
     * @returns: the next token information in the string
     */
    TokenResult next (const std::string & str) const;

  private:

    /**
     * Find the index of the head of the token 'tok'
     * @returns: the index of the token node, this-> _heads.size () if none
     */
    uint32_t find (char tok) const;

  };

  /**
   * Transform a string into tokens
   * @params:
   *    - str: the string to tokenize
   *    - tokens: the list of tokens
   *    - skipTokens: the list of tokens to skip (assuming \forall i in skipTokens, i in tokens)
   */
  std::vector<std::string> tokenize (const std::string & str, const std::vector <std::string> & tokens, const std::vector <std::string> & skipTokens);

}
