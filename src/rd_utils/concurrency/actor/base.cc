#include <rd_utils/_.hh>
#include "base.hh"
#include "sys.hh"

namespace rd_utils::concurrency::actor {


  ActorBase::ActorBase (const std::string & name, ActorSystem * sys) :
    _name (name)
    , _system (sys)
  {}

  void ActorBase::onMessage (const rd_utils::utils::config::ConfigNode & msg) {
  }

  std::shared_ptr<rd_utils::utils::config::ConfigNode> ActorBase::onRequest (const rd_utils::utils::config::ConfigNode & msg) {
    return nullptr;
  }

  std::shared_ptr <rd_utils::memory::cache::collection::ArrayListBase> ActorBase::onRequestList (const rd_utils::utils::config::ConfigNode & msg) {
    return nullptr;
  }

  void ActorBase::onStream (const rd_utils::utils::config::ConfigNode & msg, ActorStream & stream) {}

  std::shared_ptr<ActorRef> ActorBase::getRef () {
    return this-> _system-> localActor (this-> _name);
  }

  void ActorBase::exit () {
    this-> _system-> remove (this-> _name);
  }

  void ActorBase::onQuit () {
  }

}
