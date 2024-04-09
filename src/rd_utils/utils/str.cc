#include "str.hh"

#include <iostream>
#include <sstream>
#include <rd_utils/utils/error.hh>

#include <iomanip>
#include <algorithm>
#include <functional>
#include <cctype>

namespace rd_utils::utils {

  std::string findAndReplaceAll(const std::string & input, const std::string & toSearch, const std::string & replaceStr)  {
    std::string data = input;

    size_t pos = data.find(toSearch);
    while( pos != std::string::npos) {
      data.replace(pos, toSearch.size(), replaceStr);
      pos = data.find(toSearch, pos + replaceStr.size());
    }

    return data;
  }

  std::string findAndReplaceAll (const std::string & content, const std::map <std::string, std::string> & repl) {
    auto res = content;
    for (auto & it : repl) {
      res = findAndReplaceAll (res, it.first, it.second);
    }

    return res;
  }

  std::vector<std::string> splitLines (const std::string & content) {
    std::stringstream ss (content);
    std::vector <std::string> lines;
    std::string line;
    while (getline (ss, line, '\n')) {
      lines.push_back (line);
    }

    return lines;
  }

  std::vector <std::string> splitByString (const std::string & in, const std::string & splitter) {
    size_t pos = 0;
    std::vector <std::string> result ;
    std::string s = in;
    while ((pos = s.find(splitter)) != std::string::npos) {
      auto token = s.substr (0, pos);
      if (token != splitter && token.size () != 0) {
        result.push_back (token);
      }

      s.erase(0, pos + splitter.length());
    }

    if (s.size () != 0) {
      result.push_back (strip (s));
    }

    return result;
  }

  std::string strip (const std::string & str) {
    std::string s = str;

    auto pos = s.find_first_not_of (" \n\r\t");
    if (pos != std::string::npos) s = s.substr (pos);

    pos = s.find_last_not_of(" \n\r\t");
    if (pos != std::string::npos) s.erase(pos + 1);

    return s;
  }

  std::string toUpper (const std::string & content) {
    std::stringstream ss;
    for (auto i : content) {
      ss << (char) toupper (i);
    }

    return ss.str ();
  }


  Table::Table (const std::vector <std::string> & head) :
    head (head)
  {}

  void Table::addRow (const std::vector <std::string> & row) {
    if (row.size () == this-> head.size ()) {
      this-> rows.push_back (row);
    } else {
      throw Rd_UtilsError ("What are you doing !");
    }
  }

  std::string Table::toString () const {
    auto width = this-> computeLineWidth ();
    std::stringstream ss;

    this-> hline (ss, width);
    ss << std::endl;
    this-> writeLine (ss, width, this-> head);
    ss << std::endl;
    this-> hline (ss, width);
    ss << std::endl;
    for (auto & row : this-> rows) {
      this-> writeLine (ss, width, row);
      ss << std::endl;
    }
    this-> hline (ss, width);
    return ss.str ();
  }

  void Table::writeLine (std::stringstream & ss, const std::vector <uint32_t> & width, const std::vector <std::string> & row) const {
    ss << "| ";
    for (unsigned int i = 0 ; i < width.size () ; i++) {
      if (i != 0) ss << " | ";
      if (row [i] == "") {
        ss << std::setw (width [i]) << std::setfill (' ') << "-";
      } else {
        ss << std::setw (width [i]) << std::setfill (' ') << std::left << row [i];
      }
    }
    ss << " |";
  }

  void Table::hline (std::stringstream & ss, const std::vector <uint32_t> & width) const {
    ss << "+-";
    for (unsigned int i = 0 ; i < this-> head.size () ; i++) {
      if (i != 0) ss << "-+-";
      ss << std::setw (width [i]) << std::setfill ('-') << "";
    }
    ss << "-+";
  }

  std::vector <uint32_t> Table::computeLineWidth () const {
    std::vector <uint32_t> width;
    for (unsigned int i = 0 ; i < head.size () ; i++) {
      width.push_back (head [i].size () + 2);
    }

    for (auto & row : this-> rows) {
      for (unsigned i = 0 ; i < head.size () ; i++) {
        if (width [i] < row [i].size () + 2) {
          width [i] = row [i].size () + 2;
        }
      }
    }

    return width;
  }


}
