#include "msg.hh"
#include "sys.hh"
#include <rd_utils/utils/raw_parser.hh>

using namespace rd_utils::utils;

namespace rd_utils::concurrency::actor {

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =====================================          CTORS          ======================================
     * ====================================================================================================
     * ====================================================================================================
     */

    ActorMessage::ActorMessage ()
        : _sys (nullptr)
        , _kind (Protocol::NONE)
        , _uid (0)
        , _actorName ()
        , _addr (net::SockAddrV4 (0, 0))
        , _content (nullptr)
    {}

    ActorMessage::ActorMessage (ActorSystem * sys, ActorMessage::Protocol kind, const std::string & targetActor, std::shared_ptr <utils::config::ConfigNode> content)
        : _sys (sys)
        , _kind (kind)
        , _uid (0)
        , _actorName (targetActor)
        , _addr (sys-> addr ())
        , _content (content)
    {}

    ActorMessage::ActorMessage (ActorSystem * sys, ActorMessage::Protocol kind, uint64_t uid, const std::string & targetActor, std::shared_ptr <utils::config::ConfigNode> content)
        : _sys (sys)
        , _kind (kind)
        , _uid (uid)
        , _actorName (targetActor)
        , _addr (sys-> addr ())
        , _content (content)
    {}

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * ====================================          GETTERS          =====================================
     * ====================================================================================================
     * ====================================================================================================
     */

    ActorMessage::Protocol ActorMessage::kind () const {
        return this-> _kind;
    }

    uint64_t ActorMessage::getUniqId () const {
        return this-> _uid;
    }

    const std::string & ActorMessage::getTargetActor () const {
        return this-> _actorName;
    }

    const net::SockAddrV4 & ActorMessage::getSenderAddr () const {
        return this-> _addr;
    }

    const std::shared_ptr <utils::config::ConfigNode> ActorMessage::getContent () const {
        return this-> _content;
    }

    /*!
     * ====================================================================================================
     * ====================================================================================================
     * =================================          SERIALIZATION          ==================================
     * ====================================================================================================
     * ====================================================================================================
     */

    ActorMessage ActorMessage::deserialize (ActorSystem * sys, net::Ipv4Address origin, std::vector <uint8_t> & content) {
        uint8_t * buffer = content.data ();
        uint32_t len = content.size ();

        ActorMessage result;

        result._sys = sys;
        result._kind = static_cast <ActorMessage::Protocol> (raw::readU8 (buffer, len));
        result._uid = raw::readU64 (buffer, len);
        uint32_t port = raw::readU32 (buffer, len);
        result._addr = net::SockAddrV4 (origin, port);

        uint32_t actorNameSize = raw::readU32 (buffer, len);
        result._actorName = raw::readString (buffer, len, actorNameSize);
        uint8_t hasContent = raw::readU8 (buffer, len);

        if (hasContent != 0) {
            raw::RawParser msgParser;
            result._content =  msgParser.parse (buffer, len);
        }

        return result;
    }

    std::vector <uint8_t> ActorMessage::serialize () const {
        std::vector <uint8_t> buffer;
        raw::writeU8 (buffer, static_cast <uint8_t> (this-> _kind));
        raw::writeU64 (buffer, this-> _uid);
        raw::writeU32 (buffer, this-> _addr.port ());
        raw::writeU32 (buffer, this-> _actorName.size ());
        raw::writeString (buffer, this-> _actorName);
        if (this-> _content == nullptr) {
            raw::writeU8 (buffer, 0);
        } else {
            raw::writeU8 (buffer, 1);
            raw::RawParser msgParser;
            msgParser.dump (buffer, *this-> _content);
        }

        return buffer;
    }


}
