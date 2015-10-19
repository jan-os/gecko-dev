#ifndef mozilla_dom_os_StatSerializer_h
#define mozilla_dom_os_StatSerializer_h

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "ipc/IPCMessageUtils.h"

namespace mozilla {
namespace dom {
namespace os {
class StatWrapper final
{
public:
  StatWrapper() {}

  StatWrapper(struct stat aStat, int aError)
    : mStat(aStat)
    , mError(aError)
  {}

  void SetWrappedObject(struct stat aStat, int aError)
  {
    mStat = aStat;
    mError = aError;
  }

  struct stat GetWrappedObject() const
  {
    return mStat;
  }

  int GetError() const
  {
    return mError;
  }

private:
  struct stat mStat;
  int mError = 0;
};
}
}
}

namespace IPC {
template <>
struct ParamTraits<mozilla::dom::os::StatWrapper>
{
  typedef mozilla::dom::os::StatWrapper paramType;

  static void Write(Message *aMsg, const paramType& aParam)
  {
    struct stat s = aParam.GetWrappedObject();

    WriteParam(aMsg, s.st_dev);
    WriteParam(aMsg, s.st_ino);
    WriteParam(aMsg, s.st_mode);
    WriteParam(aMsg, s.st_nlink);
    WriteParam(aMsg, s.st_uid);
    WriteParam(aMsg, s.st_gid);
    WriteParam(aMsg, s.st_rdev);
    WriteParam(aMsg, s.st_size);
    WriteParam(aMsg, s.st_blksize);
    WriteParam(aMsg, s.st_blocks);
    WriteParam(aMsg, s.st_atime);
    WriteParam(aMsg, s.st_mtime);
    WriteParam(aMsg, s.st_ctime);
    WriteParam(aMsg, aParam.GetError());
  }

  static bool Read(const Message *aMsg, void **aIter, paramType* aResult)
  {
    dev_t dev = 0;
    ino_t ino = 0;
    mode_t mode = 0;
    nlink_t nlink = 0;
    uid_t uid = 0;
    gid_t gid = 0;
    dev_t rdev = 0;
    off_t size = 0;
    blksize_t blksize = 0;
    blkcnt_t blocks = 0;
    time_t atime = 0;
    time_t mtime = 0;
    time_t ctime = 0;
    int error = 0;

    if (!ReadParam(aMsg, aIter, &dev) ||
        !ReadParam(aMsg, aIter, &ino) ||
        !ReadParam(aMsg, aIter, &mode) ||
        !ReadParam(aMsg, aIter, &nlink) ||
        !ReadParam(aMsg, aIter, &uid) ||
        !ReadParam(aMsg, aIter, &gid) ||
        !ReadParam(aMsg, aIter, &rdev) ||
        !ReadParam(aMsg, aIter, &size) ||
        !ReadParam(aMsg, aIter, &blksize) ||
        !ReadParam(aMsg, aIter, &blocks) ||
        !ReadParam(aMsg, aIter, &atime) ||
        !ReadParam(aMsg, aIter, &mtime) ||
        !ReadParam(aMsg, aIter, &ctime) ||
        !ReadParam(aMsg, aIter, &error)) {
      return false;
    }

    struct stat s;
    s.st_dev = dev;
    s.st_ino = ino;
    s.st_mode = mode;
    s.st_nlink = nlink;
    s.st_uid = uid;
    s.st_gid = gid;
    s.st_rdev = rdev;
    s.st_size = size;
    s.st_blksize = blksize;
    s.st_blocks = blocks;
    s.st_atime = atime;
    s.st_mtime = mtime;
    s.st_ctime = ctime;

    aResult->SetWrappedObject(s, error);

    return true;
  }
};
}

#endif
