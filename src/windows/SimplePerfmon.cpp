#include "SimplePerfmon.h"

SimplePerfmon::SimplePerfmon()
{
}

SimplePerfmon::~SimplePerfmon()
{
}

unsigned long long SimplePerfmon::FileTimeToUInt64(FILETIME fileTime)
{
  return (static_cast<unsigned long long>(fileTime.dwHighDateTime) << 32) + fileTime.dwLowDateTime;
}

bool SimplePerfmon::getCpuUsage(float * out_ñpuUsage)
{
  clock_t now = clock();

  if (now - lastCpuState.clock >= minIntervalMs * CLOCKS_PER_SEC / 1000)
  {
    FILETIME idleTime, kernelTime, userTime;

    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime))
      return false;

    // values can overflows, but it's not affect the correctness of the result
    unsigned long long totalTime = FileTimeToUInt64(kernelTime) + FileTimeToUInt64(userTime);
    unsigned long long activeTime = totalTime - FileTimeToUInt64(idleTime);
    unsigned long long totalTimePassed = totalTime - lastCpuState.totalTime;
    unsigned long long activeTimePassed = activeTime - lastCpuState.activeTime;

    if (!totalTimePassed)
      return false;

    lastCpuState.totalTime = totalTime;
    lastCpuState.activeTime = activeTime;
    lastCpuState.usage = float(activeTimePassed) / totalTimePassed;
    lastCpuState.clock = now;
  }

  if (out_ñpuUsage)
    *out_ñpuUsage = lastCpuState.usage;

  return true;
}

bool SimplePerfmon::getDiskUsage(const char * diskName, float * out_diskUsage)
{
  const int maxPhysicalDrivesCount = 64;
  static char fullDiskName[] = "\\\\.\\C:";
  fullDiskName[4] = diskName[0];

  HANDLE hDisk = CreateFileA(fullDiskName, 0, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

  static struct VOLUME_DISK_EXTENTS_
  {
    DWORD NumberOfDiskExtents;
    DISK_EXTENT Extents[maxPhysicalDrivesCount];
  } volumeDiskExtends;

  DWORD bytesReturned = 0;
  BOOL ioControlResult = DeviceIoControl(hDisk, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0,
                                         &volumeDiskExtends, sizeof(volumeDiskExtends),
                                         &bytesReturned, NULL);
  CloseHandle(hDisk);

  if (!ioControlResult)
    return false;

  float maxUsage = -1.0f;
  clock_t now = clock();

  for (int i = 0; i < (int)volumeDiskExtends.NumberOfDiskExtents; i++)
  {
    DISK_EXTENT & diskExtent = volumeDiskExtends.Extents[i];
    static std::string fullDriveName = "\\\\.\\PhysicalDrive0";
    fullDriveName[17] = (char)diskExtent.DiskNumber + '0';
    LastState & lastState = lastDriveStates[fullDriveName];

    if (now - lastState.clock >= minIntervalMs * CLOCKS_PER_SEC / 1000)
    {
      HANDLE hDrive = CreateFileA(fullDriveName.c_str(), 0, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

      if (hDrive == INVALID_HANDLE_VALUE)
        return false;

      DISK_PERFORMANCE diskPerformance;
      DWORD bytesReturned = 0;
      BOOL ioControlResult = DeviceIoControl(hDrive, IOCTL_DISK_PERFORMANCE, NULL, 0, &diskPerformance,
                                             sizeof(diskPerformance), &bytesReturned, NULL);
      CloseHandle(hDrive);

      if (!ioControlResult)
        return false;

      unsigned long long totalTime = diskPerformance.ReadTime.QuadPart +
        diskPerformance.WriteTime.QuadPart +
        diskPerformance.IdleTime.QuadPart;
      unsigned long long activeTime = diskPerformance.ReadTime.QuadPart +
        diskPerformance.WriteTime.QuadPart;
      unsigned long long totalTimePassed = totalTime - lastState.totalTime;
      unsigned long long activeTimePassed = activeTime - lastState.activeTime;

      if (!totalTimePassed)
        return false;

      lastState.totalTime = totalTime;
      lastState.activeTime = activeTime;
      lastState.usage = float(activeTimePassed) / totalTimePassed;
      lastState.clock = now;
    }

    maxUsage = max(lastState.usage, maxUsage);
  }

  if (out_diskUsage)
    *out_diskUsage = maxUsage;

  return true;
}

