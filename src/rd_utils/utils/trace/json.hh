#pragma once

#include <vector>
#include <string>
#include <fstream>

#include "base.hh"

namespace rd_utils::utils::trace {

    /**
     * Trace exporter to /dev/null
     */
    class JsonExporter : public TraceExporter {

        std::string _filename;
        std::ofstream _file;
        std::vector <std::string> _head;
        bool _first = true;

    public:

        JsonExporter (const std::string & filename);

        void setHeader (const std::vector <std::string> & values) override;
        void appendFloat (uint32_t timestamp, const std::vector <float> & values) override;
        void append (uint32_t timestamp, const config::ConfigNode & cfg) override;
        void appendInt (uint32_t timestamp, const std::vector <uint32_t> & values) override;

        ~JsonExporter () ;

    };

}
