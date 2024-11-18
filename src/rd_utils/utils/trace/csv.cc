#include <rd_utils/utils/trace/csv.hh>
#include <rd_utils/utils/files.hh>

namespace rd_utils::utils::trace {

    CsvExporter::CsvExporter (const std::string & filename) :
        _filename (filename)
    {
        if (!rd_utils::utils::directory_exists (rd_utils::utils::parent_directory (filename))) {
            rd_utils::utils::create_directory (rd_utils::utils::parent_directory (filename), true);
        }

        this-> _file = std::ofstream (filename, std::ios::out);
    }

    void CsvExporter::setHeader (const std::vector <std::string> & values) {
        this-> _file << "TIMESTAMP ; ";
        int i = 0;
        for (auto & it : values) {
            if (i != 0) this-> _file << " ; ";
            this-> _file << it;
            i += 1;
        }
        this-> _file << std::endl;
        this-> _head = values;
    }

    void CsvExporter::appendFloat (uint32_t timestamp, const std::vector <float> & values) {
        this-> _file << timestamp << " ; ";
        uint32_t i = 0;
        for (auto & it : values) {
            if (i != 0) this-> _file << " ; ";
            this-> _file << it;
            i += 1;
        }
        this-> _file << std::endl;
    }

    void CsvExporter::appendInt (uint32_t timestamp, const std::vector <uint32_t> & values) {
        this-> _file << timestamp << " ; ";
        uint32_t i = 0;
        for (auto & it : values) {
            if (i != 0) this-> _file << " ; ";
            this-> _file << it;
            i += 1;
        }
        this-> _file << std::endl;
    }

    void CsvExporter::append (uint32_t timestamp, const config::ConfigNode & cfg) {
        this-> _file << timestamp << " ; ";
        uint32_t i = 0;
        for (auto & it : this-> _head) {
            if (i != 0) this-> _file << " ; ";
            try {
                auto & value = cfg [it];
                this-> _file << value;
            } catch (...) {}
            i += 1;
        }

        this-> _file << std::endl;
    }

    CsvExporter::~CsvExporter () {
        this-> _file.close ();
    }
}
