#pragma once
#include <map>
#include <ctime>
#include <windows.h>

class SimplePerfmon
{
private:

  struct LastState
  {
    unsigned long long totalTime;
    unsigned long long activeTime;
    float usage;
    clock_t clock;

    LastState() :
      totalTime(0),
      activeTime(0),
      usage(0.0f),
      clock(0)
    {
    }
  };

  enum { minIntervalMs = 100 };
  unsigned long long FileTimeToUInt64(FILETIME fileTime);
  std::map<std::string, LastState> lastDriveStates;
  LastState lastCpuState;

public:
  SimplePerfmon();
  virtual ~SimplePerfmon();

  virtual bool getCpuUsage(float * out_cpuUsage);
  virtual bool getDiskUsage(const char * diskName, float * out_diskUsage);
};
