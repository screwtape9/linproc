#include <ext/stdio_filebuf.h>
#include <iostream>
#include <sstream>
#include <regex>
#include <cstring>
#include <cerrno>
#include <cassert>
#include <csignal>
#include <unistd.h>
#include <sys/wait.h>
#include "Process.h"

int Process::Run(const std::string cmd,
                 std::vector<std::string>& out,
                 std::vector<std::string>& err,
                 const bool searchPath)
{
  out.clear();
  err.clear();

  ParseArgs(cmd);

  if (searchPath) {
    // is the binary to exec in our current working dir?
    if (access(m_Args[0].c_str(), F_OK)) {
      // no, so try to find it in $PATH. take the first one we find.
      char *szPATH = getenv("PATH");
      if (szPATH) {
        const std::string strPATH(szPATH);
        const std::regex rex(":");
        std::vector<std::string> dirs;
        std::copy(std::sregex_token_iterator(strPATH.begin(),
                                             strPATH.end(),
                                             rex,
                                             -1),
                  std::sregex_token_iterator(),
                  std::back_inserter(dirs));
        for (auto dir : dirs) {
          if (dir.empty())
            continue;
          if ((*dir.rbegin()) != '/')
            dir += '/';
          dir += m_Args[0];
          if (!access(dir.c_str(), F_OK)) {
            m_Args[0] = dir;
            m_ArgPtrs[0] = (char *)m_Args[0].c_str();
            break;
          }
        }
      }
    }
  }

  // create pipe we'll use to get child proc's stdout
  int outfd[2] = { 0, 0 };
  if (pipe(outfd)) {
    SetLastErr("pipe()");
    return -1;
  }

  // create pipe we'll use to get child proc's stderr
  int errfd[2] = { 0, 0 };
  if (pipe(errfd)) {
    close(outfd[STDIN_FILENO]);
    close(outfd[STDOUT_FILENO]);
    SetLastErr("pipe()");
    return -1;
  }

  sigset_t blockMask, origMask;
  sigemptyset(&blockMask);
  sigaddset(&blockMask, SIGCHLD);
  // TODO: probably (usually) need to use pthread_sigmask() instead
  sigprocmask(SIG_BLOCK, &blockMask, &origMask);

  struct sigaction saIgnore, saOrigQuit, saOrigInt, saDefault;
  saIgnore.sa_handler = SIG_IGN;
  saIgnore.sa_flags = 0;
  sigemptyset(&saIgnore.sa_mask);
  sigaction(SIGINT, &saIgnore, &saOrigInt);
  sigaction(SIGQUIT, &saIgnore, &saOrigQuit);

  int n = 0, retcode = 0;
  pid_t cpid = fork();
  switch (cpid) {
  case -1:
    SetLastErr("fork()");
    sigprocmask(SIG_SETMASK, &origMask, NULL);
    sigaction(SIGINT, &saOrigInt, NULL);
    sigaction(SIGQUIT, &saOrigQuit, NULL);
    return -1;
    break;
  case 0:  // Grogu
    saDefault.sa_handler = SIG_DFL;
    saDefault.sa_flags = 0;
    sigemptyset(&saDefault.sa_mask);

    if (saOrigInt.sa_handler != SIG_IGN)
      sigaction(SIGINT, &saDefault, NULL);
    if (saOrigQuit.sa_handler != SIG_IGN)
      sigaction(SIGQUIT, &saDefault, NULL);

    sigprocmask(SIG_SETMASK, &origMask, NULL);
    // the child isn't reading anything, so close stdin and
    // the read end of the pipes
    close(STDIN_FILENO);
    close(outfd[STDIN_FILENO]);
    close(errfd[STDIN_FILENO]);
    // dup stdout and stderr to the write end of the pipes, so the
    // parent can read them
    dup2(outfd[STDOUT_FILENO], STDOUT_FILENO);
    dup2(errfd[STDOUT_FILENO], STDERR_FILENO);
    execv(m_ArgPtrs[0], &m_ArgPtrs[0]);
    // we only get this far if execv failed
    _exit(errno);
    break;
  default: // Mando
    // the parent isn't writing anything to the child, so close the write
    // end of the pipes
    close(outfd[STDOUT_FILENO]);
    close(errfd[STDOUT_FILENO]);

    // read the child's stdout from its pipe
    __gnu_cxx::stdio_filebuf<char> fbout(outfd[STDIN_FILENO], std::ios::in);
    std::istream outin(&fbout);
    for (std::string line; std::getline(outin, line); out.push_back(line));

    // read the child's stderr from its pipe
    __gnu_cxx::stdio_filebuf<char> fberr(errfd[STDIN_FILENO], std::ios::in);
    std::istream errin(&fberr);
    for (std::string line; std::getline(errin, line); err.push_back(line));

    n = Wait(cpid, retcode);

    sigprocmask(SIG_SETMASK, &origMask, NULL);
    sigaction(SIGINT, &saOrigInt, NULL);
    sigaction(SIGQUIT, &saOrigQuit, NULL);

    return (((!n) && (!retcode)) ? 0 : -1);
    break;
  }
  assert(false);
  return -1; // we should never get here
}

void Process::ParseArgs(std::string const& cmd)
{
  std::istringstream ss(cmd);
  std::string token;

  m_Args.clear();
  while (ss >> token)
    m_Args.push_back(token);

  m_ArgPtrs.clear();
  for (auto& s : m_Args)
    m_ArgPtrs.push_back((char *)s.c_str());
  m_ArgPtrs.push_back(nullptr);
}

int Process::Wait(const pid_t pid, int& retcode)
{
  // wait on pid until child exits or is killed by a signal
  for (;;) {
    int status = 0;
    pid_t wpid = waitpid(pid, &status, 0);
    if (wpid == -1) {
      SetLastErr("waitpid()");
      return -1;
    }

    if (WIFEXITED(status)) {
      retcode = WEXITSTATUS(status);
      if (retcode)
        SetLastErr("execv()", retcode);
      return 0;
    }

    if (WIFSIGNALED(status)) {
      retcode = WTERMSIG(status);
      SetLastErr("waitpid()", EINTR);
      return EINTR;
    }
  }

  assert(false);
  return -1; // we should never get here
}

void Process::SetLastErr(const std::string func, const int err)
{
  char buf[512] = { '\0' };
  m_LastErr = func;
  m_LastErr += " : ";
  m_LastErr += strerror_r((err ? err : errno), buf, sizeof(buf));
}
