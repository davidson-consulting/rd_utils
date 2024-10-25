#include "scp.hh"
#include <rd_utils/utils/log.hh>
#include <rd_utils/utils/error.hh>
#include <rd_utils/utils/files.hh>

#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUF_SIZE 4096

using namespace rd_utils::utils;

namespace rd_utils::concurrency {

  inline char *get_username () {
    struct passwd *pw = getpwuid(getuid());
    return pw ? pw->pw_name : NULL;
  }

  SCPProcess::SCPProcess (const std::string & hostIp, const std::string & in, const std::string & out, bool fromInput, std::string user, std::string keyPath) :
    _in (in),
    _out (out),
    _hostIp (hostIp),
    _user (user),
    _keyPath (keyPath),
    _fromInput (fromInput)
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


  SCPProcess::SCPProcess (SCPProcess && other) :
    _in (std::move (other._in)),
    _out (std::move (other._out)),
    _hostIp (std::move (other._hostIp)),
    _user (std::move (other._user)),
    _keyPath (std::move (other._keyPath)),
    _session (other._session),
    _copy (other._copy),
    _fromInput (std::move (other._fromInput))
  {
    other._session = nullptr;
    other._copy = nullptr;
  }

  void SCPProcess::operator=(SCPProcess && other) {
    this-> dispose ();

    this-> _in = std::move (other._in);
    this-> _out = std::move (other._out);
    this-> _hostIp = std::move (other._hostIp);
    this-> _user = std::move (other._user);
    this-> _keyPath = std::move (other._keyPath);
    this-> _fromInput = other._fromInput;

    this-> _session = other._session;
    this-> _copy = other._copy;

    other._session = nullptr;
    other._copy = nullptr;
  }

  void SCPProcess::upload () {
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

      this-> launchUploadCmd ();
    } catch (Rd_UtilsError & err) {
      error_message = err.what ();
    }

    if (pubset) ssh_key_free (pub);
    if (prvset) ssh_key_free (prv);

    if (error_message != "") throw Rd_UtilsError (error_message);
  }

  void SCPProcess::download () {
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


      this-> launchDownloadCmd ();
    } catch (Rd_UtilsError & err) {
      error_message = err.what ();
    }

    if (pubset) ssh_key_free (pub);
    if (prvset) ssh_key_free (prv);

    if (error_message != "") throw Rd_UtilsError (error_message);
  }


  void SCPProcess::launchUploadCmd () {
    if (this-> _fromInput) {
      this-> launchUploadCmdFromInput ();
      return;
    }

    std::cout << parent_directory (this-> _out) << " " << this-> _out << std::endl;
    this-> _copy = ssh_scp_new (this-> _session, SSH_SCP_WRITE | SSH_SCP_RECURSIVE , parent_directory(this-> _out).c_str ());
    if (this-> _copy == nullptr) {
      throw Rd_UtilsError ("Failed to open scp");
    }

    std::ifstream in (this-> _in, std::ios::binary | std::ios::ate);
    int length = in.tellg ();

    int rc = ssh_scp_init (this-> _copy);
    if (rc != SSH_OK) {
      std::string err = ssh_get_error (this-> _session);
      std::cout << err << std::endl;
      ssh_scp_free (this-> _copy);
      this-> _copy = nullptr;
      throw Rd_UtilsError (err);
    }

    std::string p = parent_directory (this-> _out);
    int l = 0;
    if (p != "") l = p.size () + 1;

    rc = ssh_scp_push_file (this-> _copy, this-> _out.substr (l).c_str (),
                            length, S_IRUSR |  S_IWUSR);
    if (rc != SSH_OK) {
      std::string err = ssh_get_error (this-> _session);
      ssh_scp_free (this-> _copy);
      this-> _copy = nullptr;
      throw Rd_UtilsError (err);
    }

    char buffer[BUF_SIZE];
    in.seekg (0, in.beg);

    for (;;) {
      in.read (buffer, BUF_SIZE);
      if (in || in.gcount ()) {

        int len = in.gcount ();
        if (in) len = BUF_SIZE;
        rc = ssh_scp_write(this-> _copy, buffer, len);
        if (rc != SSH_OK) {
          std::string err = ssh_get_error (this-> _session);
          ssh_scp_free (this-> _copy);
          this-> _copy = nullptr;
          throw Rd_UtilsError (err);
        }
        if (len != BUF_SIZE) break;
      } else break;
    }

    ssh_scp_close (this-> _copy);
    ssh_scp_free (this-> _copy);
    this-> _copy = nullptr;
  }


  void SCPProcess::launchDownloadCmd () {
    this-> _copy = ssh_scp_new (this-> _session, SSH_SCP_READ | SSH_SCP_RECURSIVE , this-> _in.c_str ());
    if (this-> _copy == nullptr) {
      throw Rd_UtilsError ("Failed to open scp");
    }

    int rc = ssh_scp_init (this-> _copy);
    if (rc != SSH_OK) {
      std::string err = ssh_get_error (this-> _session);
      ssh_scp_free (this-> _copy);
      this-> _copy = nullptr;
      throw Rd_UtilsError (err);
    }


    char buffer[1024];

    if (ssh_scp_pull_request(this-> _copy) == SSH_SCP_REQUEST_NEWFILE) {
      ssh_scp_request_get_size (this-> _copy);
      ssh_scp_accept_request (this-> _copy);

      std::ofstream out (this-> _out, std::ios::binary);
      do {
        int len = ssh_scp_read (this-> _copy, buffer, 1023);
        if (len != 0 && len != -1) {
          out.write (buffer, len);
        }

        if (len == -1) break;
      } while (true);


      out.close ();
    } else {
      ssh_scp_close (this-> _copy);
      ssh_scp_free (this-> _copy);
      this-> _copy = nullptr;
      throw Rd_UtilsError ("Failed to read scp file");
    }

    ssh_scp_close (this-> _copy);
    ssh_scp_free (this-> _copy);
    this-> _copy = nullptr;
  }

  void SCPProcess::launchUploadCmdFromInput () {
    this-> _copy = ssh_scp_new (this-> _session, SSH_SCP_WRITE | SSH_SCP_RECURSIVE , parent_directory(this-> _out).c_str ());
    if (this-> _copy == nullptr) {
      throw Rd_UtilsError ("Failed to open scp");
    }

    int rc = ssh_scp_init (this-> _copy);
    if (rc != SSH_OK) {
      std::string err = ssh_get_error (this-> _session);
      std::cout << err << std::endl;
      ssh_scp_free (this-> _copy);
      this-> _copy = nullptr;
      throw Rd_UtilsError (err);
    }

    std::string p = parent_directory (this-> _out);
    int l = 0;
    if (p != "") l = p.size () + 1;

    rc = ssh_scp_push_file (this-> _copy, this-> _out.substr (l).c_str (),
                            this-> _in.length (), S_IRUSR |  S_IWUSR);
    if (rc != SSH_OK) {
      std::string err = ssh_get_error (this-> _session);
      ssh_scp_free (this-> _copy);
      this-> _copy = nullptr;
      throw Rd_UtilsError (err);
    }


    rc = ssh_scp_write(this-> _copy, this-> _in.c_str (), this-> _in.length ());
    if (rc != SSH_OK) {
      std::string err = ssh_get_error (this-> _session);
      ssh_scp_free (this-> _copy);
      this-> _copy = nullptr;
      throw Rd_UtilsError (err);
    }


    ssh_scp_close (this-> _copy);
    ssh_scp_free (this-> _copy);
    this-> _copy = nullptr;
  }

  void SCPProcess::dispose () {
    if (this-> _copy != nullptr) {
      ssh_scp_close (this-> _copy);
      ssh_scp_free (this-> _copy);
      this-> _copy = nullptr;
    }


    if (this-> _session != nullptr) {
      ssh_free (this-> _session);
      this-> _session = nullptr;
    }
  }

  SCPProcess::~SCPProcess () {
    this-> dispose ();
  }

}
