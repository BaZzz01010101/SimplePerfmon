#pragma once
#include <map>
#include <set>
#include <string>


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
  LastState lastCpuState;
  std::map<std::string, LastState> lastDriveStates;

  enum
  {
    CT_WHITESPACE = 0x01,
    CT_LETTER = 0x02,
    CT_NUMBER = 0x04,
    CT_OTHER = 0x08
  };

  int getCharMask(char ch);
  const char * findChunk(const char * ptr, int mask, int * length);
  const char * skipChunk(const char * ptr, int mask);

public:
  SimplePerfmon();
  ~SimplePerfmon();

  bool getCpuUsage(float * out_cpuUsage);
  bool getDiskUsage(const char * diskName, float * out_diskUsage);
};
