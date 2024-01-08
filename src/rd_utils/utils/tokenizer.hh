#pragma once

#include <string>
#include <list>

namespace rd_utils::utils {

  /**
   * Cut a string into tokens
   */
  class tokenizer {
  private:

    std::string _content;

    std::list <std::string> _tokens;

    std::list <std::string> _skips;

    std::list <std::string> _comments;

    bool _doComments;

    std::list <int> _lastLocs;

    int _cursor = 0;

    bool _doSkip = true;

  public:

    /**
     *
     */
    tokenizer(const std::string& content, const std::list <std::string>& tokens, const std::list <std::string>& skips, const std::list <std::string>& comments = {});

    /**
     * @return: the next token
     */
    std::string next();

    /**
     * go back to the previous token location
     */
    void rewind();

    /**
     * Toggle token skipping
     */
    void skip(bool enable = true);

    /**
     * Toggle comments skipping
     */
    void doComments (bool enable = true);

    /**
     * @return the line number at the cursor position
     */
    int getLineNumber();

  private:

    std::string nextPure();

  };

}

