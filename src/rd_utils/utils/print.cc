#include <rd_utils/utils/print.hh>

std::ostream & operator<< (std::ostream & stream, const std::vector <char> & vec) {
    stream << "[";
    for (uint32_t i = 0 ; i < vec.size () ;i++) {
        if (i != 0) stream << ", ";
        stream << vec[i] << '[' << (int) (vec [i]) << ']';
    }
    stream << "]";
    return stream;
}
