#pragma once

#include <string>
#include <vector>
#include <map>

namespace rd_utils::utils {

  /**
   * Find and replace a list of occurence of string in 'content'
   */
  std::string findAndReplaceAll (const std::string & content, const std::map <std::string, std::string> & repl);

  /**
   * Find and replace all occurence of the string 'toSearch' in 'data' by 'replace'
   */
  std::string findAndReplaceAll(const std::string & data, const std::string & toSearch, const std::string & replaceStr);

  /**
   * Split the string by line delimiter (\n)
   */
  std::vector <std::string> splitLines (const std::string & content);

  /**
   * Split a string by a string delimiter
   */
  std::vector<std::string> splitByString (const std::string & s, const std::string & splitter);

  /**
   * Transform a string into upper case
   */
  std::string toUpper (const std::string & content);

  /**
   * Remove the white spaces at start and end of the string
   */
  std::string strip (const std::string & content);

  /**
   * @returns: true iif 's' is a prefix of 'of'
   */
  bool is_prefix (const std::string & s, const std::string & of);

  /**
   * A table formatter
   */
  class Table {

    std::vector <std::string> head;

    std::vector<std::vector <std::string> > rows;

  public :

    /**
     * @params:
     *     - head: the list of column heads
     */
    Table (const std::vector <std::string> & head);

    /**
     * Add a row in the array
     */
    void addRow (const std::vector <std::string> & row);

    /**
     * Format the array into a string
     */
    std::string toString () const;

  private:

    std::vector <uint32_t> computeLineWidth () const;

    void hline (std::stringstream & ss, const std::vector <uint32_t> & width) const;

    void writeLine (std::stringstream & ss, const std::vector <uint32_t> & width, const std::vector <std::string> & row) const;

  };

}
