#include "lexer.hh"
#include <sstream>
#include "error.hh"

namespace rd_utils::utils {

    Lexer::Lexer ()
        : _eofWord ({.str = "", .line = 0, .col = 0, .eof = true})
    {}

    Lexer::Lexer (const std::string & content, const std::vector <std::string> & tokens, const std::vector <std::string> & skipTokens, const std::map<std::string, std::string> & commentTokens)
        : _eofWord ({.str = "", .line = 0, .col = 0, .eof = true})
    {
        this-> _rewinders.push_back (0);

        Tokenizer tok;
        for (auto &it : tokens) tok.insert (it, false);
        for (auto &it : skipTokens) tok.insert (it, true);
        for (auto & it : commentTokens) tok.insert (it.first, false, it.second);

        this-> readAll (content, tok);
    }

    const Word & Lexer::next () {
        if (this-> _eof) return this-> _eofWord;
        this-> _rewinders.push_back (this-> _wordCursor);
        while (this-> _wordCursor < this-> _wordInfos.size ()) {
            TokenResult wd = this-> _wordInfos [this-> _wordCursor];
            this-> _wordCursor += 1;

            if (!wd.skip) {
                if (wd.commentEnd != "") {
                    this-> gotoNextToken (wd.commentEnd);
                } else {
                    return this-> _words [this-> _wordCursor - 1];
                }
            }
        }

        this-> _eof = true;
        return this-> _eofWord;
    }

    const Word & Lexer::directNext () {
        if (this-> _eof) return this-> _eofWord;
        this-> _rewinders.push_back (this-> _wordCursor);

        if (this-> _wordCursor < this-> _wordInfos.size ()) {
            this-> _wordCursor += 1;
            return this-> _words [this-> _wordCursor - 1];
        } else {
            this-> _eof = true;
            return this-> _eofWord;
        }
    }

    std::string Lexer::getString (const std::string & closing, bool escaping) {
        std::stringstream ss;
        bool currentEscape = false;
        while (this-> _wordCursor < this-> _wordInfos.size ()) {
            auto wd = this-> _wordInfos [this-> _wordCursor];
            auto ret = this-> _words [this-> _wordCursor];

            this-> _wordCursor += 1;
            if (ret.str == closing && !currentEscape) {
                return ss.str ();
            }

            ss << ret.str;
            currentEscape = escaping && !currentEscape && (ret.str == "\\");
        }

        this-> _eof = true;
        throw LexerError (this-> _words.back ().line, "Unterminated string literal");

        return ss.str ();
    }

    void Lexer::rewind (uint32_t nb) {
        if (this-> _eof) {
            this-> _rewinders.pop_back ();
            this-> _eof = false;
        }

        if (nb > 1) {
            for (uint32_t i = 0 ; i < nb - 1; i++) this-> _rewinders.pop_back ();
        }

        auto x = this-> _rewinders.back ();
        this-> _wordCursor = x;
        this-> _rewinders.pop_back ();
        if (this-> _rewinders.size () == 0) {
            this-> _rewinders.push_back (0);
        }
    }


    void Lexer::rewindTo (uint32_t cursor) {
        if (this-> _eof) {
            this-> _rewinders.pop_back ();
            this-> _eof = false;
        }

        while (this-> _rewinders.size () > 0) {
            if (this-> _rewinders.back () == cursor) {
                this-> _wordCursor = cursor;
                this-> _rewinders.pop_back ();
                break ;
            } else {
                this-> _rewinders.pop_back ();
            }
        }

        if (this-> _rewinders.size () == 0) {
            this-> _rewinders.push_back (0);
        }

    }

    uint32_t Lexer::getCursor () const {
        return this-> _wordCursor;
    }

    void Lexer::gotoNextToken (const std::string & end) {
        while (this-> _wordCursor < this-> _wordInfos.size ()) {
            auto & wd = this-> _wordInfos [this-> _wordCursor];
            auto & word = this-> _words [this-> _wordCursor];
            this-> _wordCursor += 1;
            if (word.str == end) {
                return;
            }
        }

        this-> _eof = true;
    }


    void Lexer::readAll (const std::string & str, Tokenizer & tzer) {
        uint32_t line = 1;
        uint32_t col = 1;

        this-> _wordCursor = 0;
        this-> _eof = false;

        this-> _words.reserve (str.length () / 5);
        this-> _wordInfos.reserve (str.length () / 5);
        const char * data = str.c_str ();
        uint32_t len = str.length ();

        while (len > 0) {
            auto wd = tzer.next (data, len);
            if (wd.len == 0) break;

            auto ret = std::string (data, wd.len);
            this-> _wordInfos.push_back (wd);
            this-> _words.push_back ({.str = ret, .line = line, .col = col, .eof = false});

            data += wd.len;
            len -= wd.len;
            if (ret [0] == '\n') {
                line += 1;
                col = 1;
            } else { col += wd.len; }
        }

        if (len > 0) {
            auto ret = std::string (data, len);
            this-> _wordInfos.push_back ({.len = len, .skip = false, .commentEnd = ""});
            this-> _words.push_back ({.str = ret, .line = line, .col = col, .eof = false});
        }

        this-> _words.shrink_to_fit ();
        this-> _wordInfos.shrink_to_fit ();
    }


}
