#include <iostream>
#include <iomanip>

#ifdef _WIN32

#include "windows/SimplePerfmon.h"

#elif __linux__

#include "linux/SimplePerfmon.h"

#else

#error Unsupported platform

#endif

using namespace std;

int main()
{
  SimplePerfmon perfmon;

  for(int i = 0; i < 10000; i++)
  {
    float cpuUsage;

    if (!perfmon.getCpuUsage(&cpuUsage))
      cpuUsage = -1.0f;

    const int diskCount = 3;
    float diskUsage[diskCount];
    const char * disksNames[diskCount] =
    {
    #ifdef _WIN32

      "C",
      "D",
      "E"

    #elif __linux__

      "sda1",
      "md127",
      "md1"

    #endif
    };

    for(int i = 0; i < diskCount; i++)
      if(!perfmon.getDiskUsage(disksNames[i], &diskUsage[i]))
        diskUsage[i] = -1.0f;

    cout << setprecision(3) << fixed;
    cout << "CPU: " << cpuUsage * 100.0f << endl;

    for(int i = 0; i < diskCount; i++)
      cout << disksNames[i] << ": " << diskUsage[i] * 100.0f << endl;

    cout << endl;

    for (int i = 0; i < 20000000; i++)
      cout << "";
  }

}

