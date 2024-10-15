#pragma once

#include "base.hh"
#include <map>
#include <set>

namespace rd_utils::utils::config {


        class Dict : public ConfigNode {
        private:

                // The dictionnary content
                std::map <std::string, std::shared_ptr<ConfigNode> > _nodes;

                std::set <std::string> _keys;

        public:

                /**
                 * @returns: the value at index 'key'
                 * @throws:
                 *    - config_error: if there is no key in there
                 */
                const ConfigNode& operator[] (const std::string & key) const override;

                const std::shared_ptr <ConfigNode> get (const std::string & key) const override;

                /**
                 * @returns: true iif the dictionnary contains the key 'key'
                 */
                bool contains (const std::string & key) const override;

                /**
                 * @returns: the list of keys in the dict
                 */
                const std::set <std::string> & getKeys () const;
                const std::map<std::string, std::shared_ptr <ConfigNode> > & getValues () const;

                Dict& insert (const std::string & key, std::shared_ptr <ConfigNode> value);
                Dict& insert (const std::string & key, const std::string & value);
                Dict& insert (const std::string & key, int64_t value);
                Dict& insert (const std::string & key, double value);

                /**
                 * @returns: inner dictionnary whose key is key
                 */
                std::shared_ptr<Dict> getInDic (const std::string & key);

                /**
                 * Transform the node into a printable string
                 */
                void format (std::ostream & s) const override;

                ~Dict ();

        };


}
