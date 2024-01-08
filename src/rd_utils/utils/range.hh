#pragma once

namespace rd_utils::utils {

  /**
   * A range iterator, used in for loops
   * implemented in utils/range.cc<
   */
  class rIterator {

    long curr;
    long _pad;

  public:

    rIterator(long, long);

    long operator* ();
    void operator++ ();
    bool operator!= (const rIterator&);

  };

  /**
   * A range value used in for loops
   * implemented in utils/range.cc
   * * @example:
   * ====
   * for (auto it : range (1, 10)) {
   *     std::cout << it << std::endl; // 1, 2, 3,...
   * }
   * ====
   */
  class range {
    long _beg;
    long _end;

  public:

    /**
     * \param beg the value of the first element
     * \param end the value of the end
     */
    range(long beg, long end);

    const rIterator begin() const;

    const rIterator end() const;

    long fst();

    long scd();

  };

}

