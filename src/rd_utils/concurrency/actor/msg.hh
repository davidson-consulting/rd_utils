#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <rd_utils/net/_.hh>
#include <rd_utils/utils/_.hh>

namespace rd_utils::concurrency::actor {

    class ActorSystem;
    class ActorMessage {
    public:

        enum Protocol : uint8_t {
            ACTOR_MSG,
            ACTOR_REQ,
            ACTOR_RESP,
            SYSTEM_KILL_ALL,
            NONE
        };

    private:

        // The system managing the message
        ActorSystem * _sys;

        // The kind of message
        Protocol _kind;

        // The uniq id of the message
        uint64_t _uid;

        // The name of the actor being targeted
        std::string _actorName;

        // The address of actor system that sent the message
        net::SockAddrV4 _addr;

        // The content of the message
        std::shared_ptr <utils::config::ConfigNode> _content;

    public:

        /**
         * Create an empty message
         */
        ActorMessage ();

        /**
         * @params:
         *    - sys: the actor system used to manage the message
         *    - kind: the kind of message
         *    - targetActor: the actor targeted by the message
         *    - content: the content of the message
         */
        ActorMessage (ActorSystem * sys, Protocol kind, const std::string & targetActor, std::shared_ptr <utils::config::ConfigNode> content);

        /**
         * Construct a message by forcing its uniq id (e.g. for reponses)
         * @params:
         *    - sys: the actor system used to manage the message
         *    - uniqId: the uniq id of the message
         *    - kind: the kind of message
         *    - targetActor: the actor targeted by the message
         *    - content: the content of the message
         */
        ActorMessage (ActorSystem * sys, Protocol kind, uint64_t uniqId, const std::string & targetActor, std::shared_ptr <utils::config::ConfigNode> content);

        /*!
         * ====================================================================================================
         * ====================================================================================================
         * =================================          SERIALIZATION          ==================================
         * ====================================================================================================
         * ====================================================================================================
         */

        /**
         * Deserialize a message from raw data
         * @params:
         *    - sys: the actor system that received the message
         *    - content: the content of the message
         */
        static ActorMessage deserialize (ActorSystem * sys, net::Ipv4Address origin, std::vector <uint8_t> & content);

        /**
         * Serialize a message to be sent through a socket
         */
        std::vector <uint8_t> serialize () const;


        /*!
         * ====================================================================================================
         * ====================================================================================================
         * ====================================          GETTERS          =====================================
         * ====================================================================================================
         * ====================================================================================================
         */

        /**
         * @returns: the kind of message
         */
        Protocol kind () const;

        /**
         * @returns: the uniq id of the message (for request management for example)
         */
        uint64_t getUniqId () const;

        /**
         * @returns: the actor targeted by the message
         */
        const std::string & getTargetActor () const;

        /**
         * @returns: the address of the actor system that sent the message
         */
        const net::SockAddrV4 & getSenderAddr () const;

        /**
         * @returns: the content of the message
         */
        const std::shared_ptr <utils::config::ConfigNode> getContent () const;

    };


}
