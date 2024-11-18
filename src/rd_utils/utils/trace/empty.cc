#include <rd_utils/utils/trace/empty.hh>
#include <rd_utils/utils/files.hh>

namespace rd_utils::utils::trace {

    EmptyExporter::EmptyExporter () {}

    void EmptyExporter::setHeader (const std::vector <std::string> & ) {}
    void EmptyExporter::appendFloat (uint32_t, const std::vector <float> &) {}
    void EmptyExporter::appendInt (uint32_t, const std::vector <uint32_t> &) {}
    void EmptyExporter::append (uint32_t, const config::ConfigNode &) {}

    EmptyExporter::~EmptyExporter () {}
}
