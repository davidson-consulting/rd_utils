#include <rd_utils/utils/trace/json.hh>
#include <rd_utils/utils/files.hh>
#include <sstream>

namespace rd_utils::utils::trace {

    JsonExporter::JsonExporter (const std::string & filename) :
        _filename (filename)
    {
        if (!rd_utils::utils::directory_exists (rd_utils::utils::parent_directory (filename))) {
            rd_utils::utils::create_directory (rd_utils::utils::parent_directory (filename), true);
        }

        this-> _first = true;
        this-> _file = std::ofstream (filename, std::ios::out);
        this-> _file << "[";
        this-> _file << std::endl;
    }

    void JsonExporter::setHeader (const std::vector <std::string> & values) {
        this-> _head = values;
    }

    void JsonExporter::appendFloat (uint32_t timestamp, const std::vector <float> & values) {
        std::stringstream s;
        if (!this-> _first) { s << ",\n"; }
        this-> _first = false;

        s << "\t{";
        uint32_t i = 0;
        s << "\"timestamp\" : " << timestamp;
        for (auto & it : this-> _head) {
            s << ", \"" << it << "\" : " << values [i];
        }
        s << "}";
        this-> _file << s.str ();
        this-> _file << std::endl;
    }

    void JsonExporter::appendInt (uint32_t timestamp, const std::vector <uint32_t> & values) {
        std::stringstream s;
        if (!this-> _first) { s << ",\n"; }
        this-> _first = false;

        s << "\t{";
        uint32_t i = 0;
        s << "\"timestamp\" : " << timestamp;
        for (auto & it : this-> _head) {
            s << ", \"" << it << "\" : " << values [i];
        }
        s << "}";
        this-> _file << s.str ();
        this-> _file << std::endl;
    }

    void JsonExporter::append (uint32_t timestamp, const config::ConfigNode & cfg) {
        std::stringstream s;
        if (!this-> _first) { s << ",\n"; }
        this-> _first = false;

        s << "\t{";
        s << "\"timestamp\" : " << timestamp;
        match (cfg) {
            of (config::Dict, dc) {
                for (auto & it : dc-> getKeys ()) {
                    try {
                        auto & val = cfg [it];
                        match (val) {
                            of (config::String, g) {
                                s << ", \"" << it << "\" : \"" << val << "\"";
                            } elfo {
                                s << ", \"" << it << "\" : " << val;
                            }
                        }
                    } catch (...) {}
                }
            } fo;
        }
        s << "}";
        this-> _file << s.str ();
        this-> _file << std::endl;
    }

    JsonExporter::~JsonExporter () {
        this-> _file << "]\n";
        this-> _file.close ();
    }
}
