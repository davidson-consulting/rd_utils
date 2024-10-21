#include <iostream>
#include <rd_utils/concurrency/proc.hh>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/personality.h>
#include <rd_utils/utils/files.hh>

namespace rd_utils::concurrency {

    SubProcess::SubProcess ()
    {}

    SubProcess::SubProcess (const std::string & cmd, const std::vector <std::string> & args, const std::string & cwd) :
         _cwd (cwd)
         ,  _args (args)
         , _cmd (cmd)
    {}


    SubProcess::SubProcess (SubProcess && other) :
        _stdin (std::move (other._stdin)),
        _stdout (std::move (other._stdout)),
        _stderr (std::move (other._stderr)),
        _cwd (std::move (other._cwd)),
        _args (std::move (other._args)),
        _cmd (std::move (other._cmd)),
        _pid (std::move (other._pid))
    {}

    void SubProcess::operator= (SubProcess && other) {
        this-> _stderr.dispose ();
        this-> _stdout.dispose ();
        this-> _stdin.dispose ();
	
        this-> _stdin = std::move (other._stdin);
        this-> _stdout = std::move (other._stdout);
        this-> _stderr = std::move (other._stderr);
        this-> _cwd = std::move (other._cwd);
        this-> _args = std::move (other._args);
        this-> _cmd = std::move (other._cmd);
        this-> _pid = std::move (other._pid);
    }    
    
    /**
     * ================================================================================
     * ================================================================================
     * =========================           RUNNING            =========================
     * ================================================================================
     * ================================================================================
     */

    
    void SubProcess::start (bool redirect) {
        if (!utils::directory_exists (this-> _cwd)) {
            throw std::runtime_error ("failed to chdir in process spawning");
        }

        this-> _pid = ::fork ();
        if (this-> _pid == 0) {
            this-> child (redirect);
        }

        this-> _stdin.ipipe ().dispose ();
        this-> _stdout.opipe ().dispose ();
        this-> _stderr.opipe ().dispose ();
	
        this-> _stdin.opipe ().setNonBlocking ();
        this-> _stdout.ipipe ().setNonBlocking ();
        this-> _stderr.ipipe ().setNonBlocking ();
    }

    void SubProcess::startDebug () {
        this-> _pid = ::fork ();
        if (this-> _pid == 0) {
            personality(ADDR_NO_RANDOMIZE);
            ptrace (PTRACE_TRACEME, 0, nullptr, nullptr);
            this-> child (true);
        }

        this-> _stdin.ipipe ().dispose ();
        this-> _stdout.opipe ().dispose ();
        this-> _stderr.opipe ().dispose ();

        this-> _stdin.opipe ().setNonBlocking ();
        this-> _stdout.ipipe ().setNonBlocking ();
        this-> _stderr.ipipe ().setNonBlocking ();
	
    }    

    void SubProcess::child (bool redirect) {
        this-> _stderr.opipe ().setNonBlocking ();
        this-> _stdout.opipe ().setNonBlocking ();

        if (redirect) {
            ::dup2 (this-> _stdin.ipipe ().getHandle (), STDIN_FILENO);
            ::dup2 (this-> _stdout.opipe ().getHandle (), STDOUT_FILENO);
            ::dup2 (this-> _stderr.opipe ().getHandle (), STDERR_FILENO);

            this-> _stdin.dispose ();
            this-> _stdout.dispose ();
            this-> _stderr.dispose ();
        }

        std::vector <const char*> alls;
        alls.push_back (this-> _cmd.c_str ());
        for (auto & it : this-> _args) {
            alls.push_back (it.c_str ());
        }
        alls.push_back (NULL);

        if (::chdir (this-> _cwd.c_str ()) == -1) {
            exit (-1);
        }

        int out = ::execvp (alls [0],  const_cast<char* const *> (alls.data ()));
        if (out == -1) {
            std::cerr << "Command not found '" << alls [0] << "'" << std::endl;
        }

        exit (0);
    }

    int SubProcess::getPid () const {
        return this-> _pid;
    }

    bool SubProcess::isFinished () const {
        waitpid (this-> _pid, nullptr, 1);
        if (::kill (this-> _pid, 0) == -1) return true;

        return false;
    }
    
    void SubProcess::kill () {
        this-> _stdin.dispose ();
        ::kill (this-> _pid, SIGKILL);
    }

    int SubProcess::wait () {
        int status = 0;
        this-> _stdin.dispose ();
        ::waitpid (this-> _pid, &status, 0);
        return status;
    }

    /**
     * ================================================================================
     * ================================================================================
     * =========================           GETTERS            =========================
     * ================================================================================
     * ================================================================================
     */

    OPipe & SubProcess::stdin () {
        return this-> _stdin.opipe ();
    }

    IPipe & SubProcess::stdout () {
        return this-> _stdout.ipipe ();
    }

    IPipe & SubProcess::stderr () {
        return this-> _stderr.ipipe ();
    }

    SubProcess::~SubProcess () {
        this-> _stderr.dispose ();
        this-> _stdout.dispose ();
        this-> _stdin.dispose ();
    }
    
}

