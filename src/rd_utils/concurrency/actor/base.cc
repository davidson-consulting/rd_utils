#include <rd_utils/_.hh>
#include "base.hh"
#include "sys.hh"

namespace rd_utils::concurrency::actor {


  ActorBase::ActorBase (const std::string & name, ActorSystem * sys) :
    _name (name)
    , _system (sys)
  {}

  std::shared_ptr<ActorRef> ActorBase::getRef () {
    return this-> _system-> localActor (this-> _name);
  }

  void ActorBase::exit () {
    this-> _system-> remove (this-> _name);
  }


}
