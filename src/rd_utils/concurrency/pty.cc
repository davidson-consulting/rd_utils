#include "pty.hh"

#include <rd_utils/utils/log.hh>
#include <rd_utils/utils/error.hh>

#include <termios.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstring>

#include <sys/ioctl.h>
#include <signal.h>

using namespace rd_utils::utils;

struct termios terminal_local;
struct termios terminal_clean;

bool sizeChanged = false;

void cleanup (int) {
  tcsetattr(0, TCSANOW, &terminal_clean);
}

void onSizeChanged (int) {
  sizeChanged = true;
}

void setSizeSignal () {
  signal (SIGWINCH, &onSizeChanged);
}

namespace rd_utils::concurrency {


  inline char *get_username () {
    struct passwd *pw = getpwuid(getuid());
    return pw ? pw->pw_name : NULL;
  }

  int kbhit()
  {
    struct timeval tv = { 0L, 0L };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv);
  }


  SSHPty::SSHPty (const std::string & hostIp, std::string user, std::string keyPath) :
    _hostIp (hostIp),
    _user (user),
    _keyPath (keyPath)
  {
    if (this-> _keyPath == "") {
      auto username = std::string (get_username ());
      if (username == "root") {
        this-> _keyPath = "/root/.ssh/id_rsa";
      } else {
        this-> _keyPath = "/home/" + username + "/.ssh/id_rsa";
      }
    }
  }

  SSHPty & SSHPty::open () {
    this-> openSession ();
    this-> openChannel ();
    this-> start (this-> _session, this-> _channel);

    return *this;
  }

  void SSHPty::openSession () {
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
    } catch (const Rd_UtilsError & err) {
      error_message = err.what ();
    }

  // exit:
    if (pubset) ssh_key_free (pub);
    if (prvset) ssh_key_free (prv);

    if (error_message != "") throw Rd_UtilsError (error_message);
  }


  void SSHPty::openChannel () {
    this-> _channel = ssh_channel_new(this-> _session);
    if (this-> _channel == NULL) throw Rd_UtilsError ("Error opening channel");

    int rc = ssh_channel_open_session(this-> _channel);
    if (rc != SSH_OK)
      {
        ssh_channel_free(this-> _channel);
        throw Rd_UtilsError ("Error opening channel");
      }
  }


  void SSHPty::start (ssh_session session, ssh_channel channel)
  {

    int rc = ssh_channel_request_pty(channel);
    if (rc != SSH_OK) return ;
    this-> onSizeChanged ();
    if (rc != SSH_OK) return ;
    rc = ssh_channel_request_shell(channel);
    if (rc != SSH_OK) return ;


    int interactive = isatty (0);
    if(interactive){
      tcgetattr(0, &terminal_local);
      memcpy(&terminal_clean, &terminal_local, sizeof(struct termios));
      cfmakeraw (&terminal_local);
      tcsetattr(0, TCSANOW, &terminal_local);
      setSizeSignal ();
      signal(SIGTERM, cleanup);
    }

    char buffer[256];
    int nbytes, nwritten;
    while (ssh_channel_is_open(channel) &&
           !ssh_channel_is_eof(channel))
      {
        struct timeval timeout;
        ssh_channel in_channels[2], out_channels[2];
        fd_set fds;
        int maxfd;
        timeout.tv_sec = 30;
        timeout.tv_usec = 0;
        in_channels[0] = channel;
        in_channels[1] = NULL;
        FD_ZERO(&fds);
        FD_SET(0, &fds);
        FD_SET(ssh_get_fd(session), &fds);
        maxfd = ssh_get_fd(session) + 1;
        ssh_select(in_channels, out_channels, maxfd, &fds, &timeout);
        if (out_channels[0] != NULL)
          {
            nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
            if (nbytes < 0) throw Rd_UtilsError ("in pty");
            if (nbytes > 0)
              {
                nwritten = write(1, buffer, nbytes);
                if (nwritten != nbytes) throw Rd_UtilsError ("in pty");
              }
          }

        if (sizeChanged) {
          this-> onSizeChanged ();
          setSizeSignal ();
        }

        if (FD_ISSET(0, &fds))
          {
            nbytes = read(0, buffer, sizeof(buffer));
            if (nbytes < 0) throw Rd_UtilsError ("in pty");
            if (nbytes > 0)
              {
                nwritten = ssh_channel_write(channel, buffer, nbytes);
                if (nbytes != nwritten) throw Rd_UtilsError ("in pty");
              }
          }
      }

    cleanup (0);
  }

  void SSHPty::onSizeChanged () {
    struct winsize win = { 0, 0, 0, 0 };
    ioctl(1, TIOCGWINSZ, &win);

    ssh_channel_change_pty_size(this-> _channel, win.ws_col, win.ws_row);
  }

  SSHPty::~SSHPty () {
    if (this-> _channel != nullptr) {
      ssh_channel_free (this-> _channel);
      this-> _channel = nullptr;
    }

    if (this-> _session != nullptr) {
      ssh_free (this-> _session);
      this-> _session = nullptr;
    }
  }


}
