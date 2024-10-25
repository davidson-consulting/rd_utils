#include "ssh.hh"
#include <rd_utils/utils/log.hh>
#include <rd_utils/utils/error.hh>

#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


using namespace rd_utils::utils;

namespace rd_utils::concurrency {

  inline char *get_username () {
    struct passwd *pw = getpwuid(getuid());
    return pw ? pw->pw_name : NULL;
  }

  SSHProcess::SSHProcess (const std::string & hostIp, const std::string & cmd, std::string user, std::string keyPath) :
    _cmd (cmd),
    _hostIp (hostIp),
    _user (user),
    _keyPath (keyPath)
  {
    if (keyPath == "") {
      auto username = std::string (get_username ());
      if (username == "root") {
        this-> _keyPath = "/root/.ssh/id_rsa";
      } else {
        this-> _keyPath = "/home/" + username + "/.ssh/id_rsa";
      }
    }
  }

  SSHProcess::SSHProcess (SSHProcess && other) :
    _cmd (std::move (other._cmd)),
    _hostIp (std::move (other._hostIp)),
    _user (std::move (other._user)),
    _keyPath (std::move (other._keyPath))
  {
    this-> _session = other._session;
    this-> _channel = other._channel;

    other._session = nullptr;
    other._channel = nullptr;
  }

  void SSHProcess::operator=(SSHProcess && other) {
    this-> dispose ();

    this-> _cmd = std::move (other._cmd);
    this-> _hostIp = std::move (other._hostIp);
    this-> _user = std::move (other._user);
    this-> _keyPath = std::move (other._keyPath);

    this-> _session = other._session;
    this-> _channel = other._channel;
    other._session = nullptr;
    other._channel = nullptr;
  }

  SSHProcess & SSHProcess::start () {
    std::string error_message = "";
    this-> _session = ssh_new ();

    ssh_key pub;
    ssh_key prv;
    bool pubset = false, prvset = false;

    try {
      if (ssh_options_set(this-> _session, SSH_OPTIONS_HOST, this-> _hostIp.c_str ()) != 0) {
        throw Rd_UtilsError ("Error setting host ip");
      }

      if (ssh_options_set(this-> _session, SSH_OPTIONS_USER, this-> _user.c_str ()) != 0) {
        throw Rd_UtilsError ("Error setting host user");
      }

      if (ssh_connect (this-> _session) != SSH_OK) {
        throw Rd_UtilsError ("Error connecting to ssh server");
      }

      if (ssh_pki_import_pubkey_file ((this-> _keyPath + ".pub").c_str (), &pub) != SSH_OK) {
        throw Rd_UtilsError ("Error loading public key file");
      }

      pubset = true;
      if (ssh_userauth_try_publickey (this-> _session, this-> _user.c_str (), pub) != SSH_AUTH_SUCCESS) {
        throw Rd_UtilsError ("Error public key refused");
      }

      if (ssh_pki_import_privkey_file (this-> _keyPath.c_str (), NULL, NULL, NULL, &prv) != SSH_OK) {
        throw Rd_UtilsError ("Error loading private key file");
      }

      prvset = true;
      if (ssh_userauth_publickey (this-> _session, NULL, prv) != SSH_AUTH_SUCCESS) {
        throw Rd_UtilsError ("Error private key refused");
      }

      this-> launchCmd ();
    } catch (Rd_UtilsError & err) {
      error_message = err.what ();
    }

  // exit:
    if (pubset) ssh_key_free (pub);
    if (prvset) ssh_key_free (prv);

    if (error_message != "") throw Rd_UtilsError (error_message);
    return *this;
  }

  void SSHProcess::launchCmd () {
    std::string error_message = "";
    std::stringstream ss (this-> _cmd);

    this-> _channel = ssh_channel_new (this-> _session);
    if (ssh_channel_open_session (this-> _channel) != SSH_OK) {
      error_message = "Error openning channel";
      goto exit;
    }

    if (ssh_channel_request_exec (this-> _channel, ss.str ().c_str ()) != SSH_OK) {
      error_message = "Error sending command";
      ssh_channel_close (this-> _channel);
    }

  exit:
    if (error_message != "") throw Rd_UtilsError (error_message);
  }

  std::string SSHProcess::stdout () {
    std::stringstream ss;
    if (this-> _channel != nullptr) {
      int nbbytes = 0;
      char buffer[256];
      nbbytes = ssh_channel_read (this-> _channel, buffer, sizeof(buffer), 0);
      while (nbbytes > 0) {
        buffer [nbbytes] = 0;
        ss << buffer;
        nbbytes = ssh_channel_read (this-> _channel, buffer, sizeof(buffer), 0);
      }
    }
    return ss.str ();
  }

  int SSHProcess::wait () {
    if (this-> _channel != nullptr) {
      return ssh_channel_get_exit_status (this-> _channel);
    }

    return -1;
  }


  void SSHProcess::dispose () {
    if (this-> _channel != nullptr) {
      ssh_channel_free (this-> _channel);
      this-> _channel = nullptr;
    }

    if (this-> _session != nullptr) {
      ssh_free (this-> _session);
      this-> _session = nullptr;
    }
  }

  SSHProcess::~SSHProcess () {
    this-> dispose ();
  }


}
