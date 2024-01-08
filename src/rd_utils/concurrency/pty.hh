#pragma once

#include <string>
#include <vector>

#include <libssh/libssh.h>

namespace rd_utils::concurrency {


  class SSHPty {

    // The ip of the ssh server
    std::string _hostIp;

    // The user
    std::string _user;

    // THe private key file
    std::string _keyPath;

    ssh_session _session = nullptr;

    ssh_channel _channel = nullptr;

  private:

    SSHPty (const SSHPty & other);
    void operator=(const SSHPty & other);

  public:

    SSHPty (const std::string & hostIp, std::string user = "root", std::string keyPath = "");

    SSHPty & open ();

    ~SSHPty ();


  private:

    void openSession ();

    void openChannel ();

    void start (ssh_session session, ssh_channel ch);

    void onSizeChanged ();

  };

}
