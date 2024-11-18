#pragma once

#include <vector>
#include <cstdint>
#include <rd_utils/utils/config/_.hh>

namespace rd_utils::utils::trace {

    class TraceExporter {
    public:

        TraceExporter ();

        virtual void setHeader (const std::vector <std::string> & values) = 0;
        virtual void appendFloat (uint32_t timestamp, const std::vector <float> & values) = 0;
        virtual void append (uint32_t timestamp, const config::ConfigNode & cfg) = 0;
        virtual void appendInt (uint32_t timestamp, const std::vector <uint32_t> & values) = 0;

        virtual ~TraceExporter () = 0;

    };


}
