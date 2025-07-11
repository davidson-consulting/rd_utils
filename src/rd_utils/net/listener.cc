#define LOG_LEVEL 10

#include <rd_utils/net/listener.hh>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <rd_utils/utils/_.hh>


namespace rd_utils {

  namespace net {

    bool ignoreSigPipe () {
      // LOG_DEBUG ("Disabling SIGPIPE");
      signal(SIGPIPE, SIG_IGN); // ignore
      return true;
    }
	
    bool __SIG__ = ignoreSigPipe ();

    TcpListener::TcpListener () :
      _addr (SockAddrV4 (Ipv4Address (0), 0))
    {}

    TcpListener::TcpListener (SockAddrV4 addr) :
      _addr (addr)
    {}

    void TcpListener::start () {
      this-> bind ();
    }

    std::shared_ptr<TcpStream> TcpListener::accept () {      
      sockaddr_in client = { 0 };
      unsigned int len = sizeof (sockaddr_in);

      auto sock = ::accept (this-> _sockfd, (sockaddr*) (&client), &len);
      if (sock <= 0) {
        throw utils::Rd_UtilsError ("Failed to accept client");
      }

      struct linger linger;
      linger.l_onoff = 1;
      linger.l_linger = 0;
      ::setsockopt(sock, SOL_SOCKET, SO_LINGER, (char *) &linger, sizeof(linger));
      
      auto addr = SockAddrV4 (Ipv4Address (client.sin_addr.s_addr), ntohs (client.sin_port));
      return std::make_shared <TcpStream> (sock, addr);
    }

    int TcpListener::getHandle () const {
      return this-> _sockfd;
    }

    void TcpListener::close () {
      if (this-> _sockfd != 0) {
        ::shutdown (this-> _sockfd, SHUT_RDWR);

        struct linger linger;
        linger.l_onoff = 1;
        linger.l_linger = 0;

        ::setsockopt(this-> _sockfd, SOL_SOCKET, SO_LINGER, (char *) &linger, sizeof(linger));

        ::close (this-> _sockfd);
        this-> _sockfd = 0;
      }
    }

    unsigned short TcpListener::port () const {
      return this-> _port;
    }

	
    void TcpListener::bind () {
      this-> _sockfd = socket (AF_INET, SOCK_STREAM, 0);
      if (this-> _sockfd == -1) {
        throw utils::Rd_UtilsError ("Error creating socket");
      }

      sockaddr_in sin = { 0 };
      sin.sin_addr.s_addr = this-> _addr.ip ().toN ();
      sin.sin_port = htons(this-> _addr.port ());
      sin.sin_family = AF_INET;

      if (::bind (this-> _sockfd, (sockaddr*) &sin, sizeof (sockaddr_in)) != 0) {
        ::close (this-> _sockfd);
        this-> _sockfd = 0;
        throw utils::Rd_UtilsError ("Error binding socket");
      }

      if (listen (this-> _sockfd, 2048) != 0) {
        ::close (this-> _sockfd);
        this-> _sockfd = 0;

        throw utils::Rd_UtilsError ("Error listening to socket");
      }

      if (this-> _addr.port () == 0) {
        unsigned int len = sizeof (sockaddr_in);
        auto r = getsockname (this-> _sockfd, (sockaddr*) &sin, &len);
        if (r == 0) {
          this-> _port = ntohs (sin.sin_port);
        }
      } else this-> _port = this-> _addr.port ();
	    
    }



    TcpListener::~TcpListener () {
      this-> close ();
    }

  }
}
