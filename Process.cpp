#include <iostream>
#include <sstream>
#include <cstring>
#include <cerrno>
#include <cassert>
#include <unistd.h>
#include <sys/wait.h>
#include "Process.h"

int Process::Run(const std::string cmd, std::vector<std::string>& output)
{
  output.clear();

  ParseArgs(cmd);

  // create pipe we'll use to get child proc's stdout to the parent's stdin
  int fd[2] = { 0, 0 };
  if (pipe(fd)) {
    SetLastErr("pipe()");
    return -1;
  }

  int n = 0, retcode = 0;
  pid_t cpid = fork();
  switch (cpid) {
  case -1:
    SetLastErr("fork()");
    return -1;
    break;
  case 0:  // Grogu
    close(STDIN_FILENO);      // child proc won't be reading from stdin
    close(STDERR_FILENO);     // we don't care about child's stderr either
    close(fd[STDIN_FILENO]);  // close the read end of the child
    dup2(fd[STDOUT_FILENO], STDOUT_FILENO);
    if (execv(m_ArgPtrs[0], &m_ArgPtrs[0]) == -1)
      _exit(errno);           // exit with the errno from execv()
    _exit(EXIT_SUCCESS);
    break;
  default: // Mando
    close(fd[STDOUT_FILENO]); // we (the parent) aren't writing to the child
    dup2(fd[STDIN_FILENO], STDIN_FILENO);

    for (std::string line; std::getline(std::cin, line);)
      output.push_back(line);
    close(fd[STDIN_FILENO]);

    n = Wait(cpid, retcode);
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
