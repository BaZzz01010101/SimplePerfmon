#include <ctime>
#include <stdio.h>
#include <fstream>
#include <climits>
#include <cstring>
#include "SimplePerfmon.h"
#include <cstdio>

using namespace std;

SimplePerfmon::SimplePerfmon()
{
  //ctor
}

SimplePerfmon::~SimplePerfmon()
{
  //dtor
}

int SimplePerfmon::getCharMask(char ch)
{
  if(ch == ' ' || ch == '\t')
    return CT_WHITESPACE;
  else if((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'))
    return CT_LETTER;
  else if(ch >= '0' && ch <= '9')
    return CT_NUMBER;
  else
    return CT_OTHER;
}

const char * SimplePerfmon::findChunk(const char * ptr, int mask, int * length)
{
  while(*ptr && *ptr != '\n' && !(getCharMask(*ptr) & mask))
    ptr++;

  const char * start = ptr;

  while(*ptr && *ptr != '\n' && (getCharMask(*ptr) & mask))
    ptr++;

  if(length)
    *length = ptr - start;

  return start;
}

const char * SimplePerfmon::skipChunk(const char * ptr, int mask)
{
  while(*ptr && *ptr != '\n' && !(getCharMask(*ptr) & mask))
    ptr++;

  while(*ptr && *ptr != '\n' && (getCharMask(*ptr) & mask))
    ptr++;

  return ptr;
}

bool SimplePerfmon::getCpuUsage(float * out_cpuUsage)
{
  clock_t cpuClock = clock();

  if(cpuClock - lastCpuState.clock >= minIntervalMs * CLOCKS_PER_SEC / 1000)
  {
    unsigned long long userTime, userLowTime, systemTime, idleTime;
    FILE* file = fopen("/proc/stat", "r");

    if(!file)
      return false;

    int scannedCount = fscanf(file, "cpu %llu %llu %llu %llu", &userTime, &userLowTime, &systemTime, &idleTime);
    fclose(file);

    if(scannedCount < 4)
      return false;

    // values can overflows, but it's not affect the correctness of the result
    unsigned long long activeTime = userTime + userLowTime + systemTime;
    unsigned long long totalTime = activeTime + idleTime;
    unsigned long long activeTimePassed = activeTime - lastCpuState.activeTime;
    unsigned long long totalTimePassed = totalTime - lastCpuState.totalTime;

    if(!totalTimePassed)
      return false;

    lastCpuState.clock = cpuClock;
    lastCpuState.usage = float(activeTimePassed) / totalTimePassed;
    lastCpuState.activeTime = activeTime;
    lastCpuState.totalTime = totalTime;
  }

  if(out_cpuUsage)
    *out_cpuUsage = lastCpuState.usage;

  return true;
}

bool SimplePerfmon::getDiskUsage(const char * diskName, float * out_diskUsage)
{
  const int bufSize = 4096;
  static char buf[bufSize];
  std::set<std::string> physDisks;
  fstream statsFile("/proc/mdstat", ios_base::in);

  if(statsFile.good())
  {
    while(!statsFile.eof())
    {
      statsFile.getline(buf, bufSize, '\n');
      const char * ptr = buf;
      int length = 0;
      ptr = findChunk(ptr, CT_LETTER | CT_NUMBER, &length);

      if(length && ptr[0] == 'm' && ptr[1] == 'd')
      {
        string mdVolumeName(ptr, length);

        if(!strcmp(diskName, mdVolumeName.c_str()))
        {
          ptr += length;
          ptr = skipChunk(ptr, CT_LETTER);
          ptr = skipChunk(ptr, CT_LETTER | CT_NUMBER);

          while(1)
          {
            ptr = findChunk(ptr, CT_LETTER, &length);

            if(!length)
              break;

            string physicalDiskName(ptr, length);
            physDisks.insert(physicalDiskName);
            ptr += length;
          }

          break;
        }
      }
    }
  }

  if(physDisks.empty())
  {
    int length = 0;
    const char * start = findChunk(diskName, CT_LETTER, &length);
    string physDiskName(start, length);
    physDisks.insert(physDiskName);
  }

  statsFile.close();
  statsFile.open("/proc/diskstats", ios_base::in);

  if(!statsFile.good())
    return false;

  float maxUsage = 0.0f;

  while(!statsFile.eof())
  {
    statsFile.getline(buf, bufSize, '\n');
    const int maxDiskName = 256;
    static char statsDiskName[maxDiskName];
    unsigned long long activeTime = 0;
    int scanCnt = sscanf(buf, "%*i %*i %255s %*i %*i %*i %*i %*i %*i %*i %*i %*i %llu",
                         statsDiskName, &activeTime);

    if(scanCnt == 2 && physDisks.find(statsDiskName) != physDisks.end())
    {
      LastState & lastState = lastDriveStates[statsDiskName];
      timespec uptimespec;

      if(clock_gettime(CLOCK_BOOTTIME, &uptimespec))
        return false;

      unsigned long long totalTime = int(uptimespec.tv_sec) * 1000ULL +
                                     uptimespec.tv_nsec / 1000000;
      unsigned long long totalTimePassed = totalTime - lastState.totalTime;
      unsigned long long activeTimePassed = activeTime - lastState.activeTime;

      if(totalTimePassed >= minIntervalMs)
      {
        lastState.totalTime = totalTime;
        lastState.activeTime = activeTime;
        lastState.usage = float(activeTimePassed) / totalTimePassed;
      }

      maxUsage = max(lastState.usage, maxUsage);
    }
  }

  if(out_diskUsage)
    *out_diskUsage = maxUsage;

  return true;
}
