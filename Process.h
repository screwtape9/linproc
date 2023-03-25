#ifndef __JOEMAMA_PROCESS_H_
#define __JOEMAMA_PROCESS_H_

#include <string>
#include <vector>

class Process
{
public:
  Process() { }
  ~Process() { }
  int Run(const std::string cmd, std::vector<std::string>& output);
  std::string GetLastErr() const { return m_LastErr; }
private:
  Process(Process const&) = delete;
  Process& operator=(Process const&) = delete;
  void ParseArgs(std::string const& cmd);
  int Wait(const pid_t pid, int& retcode);
  void SetLastErr(const std::string func, const int err = 0);

  std::string m_LastErr;
  std::vector<std::string> m_Args;
  std::vector<char *> m_ArgPtrs;
};

#endif // __JOEMAMA_PROCESS_H_
