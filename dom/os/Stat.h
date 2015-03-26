/* vim: set shiftwidth=2 tabstop=8 autoindent cindent expandtab: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_os_Stat_h
#define mozilla_dom_os_Stat_h

#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "mozilla/dom/OsManagerBinding.h"
#include "mozilla/dom/Date.h"
#include "../workers/WorkerScope.h"

struct JSContext;

namespace mozilla {
namespace dom {
namespace os {

class Stat : public nsWrapperCache
{
public:
  explicit Stat(workers::WorkerGlobalScope* aScope, struct stat aStat)
    : mScope(aScope)
  {
    this->mStat = aStat;
  }

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(Stat)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(Stat)

  workers::WorkerGlobalScope* GetParentObject() const { return mScope; }
  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  // WebIDL interface
  int64_t Dev() const
  {
    return this->mStat.st_dev;
  }

  int64_t Ino() const
  {
    return this->mStat.st_ino;
  }

  int64_t Mode() const
  {
    return this->mStat.st_mode;
  }

  int64_t Nlink() const
  {
    return this->mStat.st_nlink;
  }

  int64_t Uid() const
  {
    return this->mStat.st_uid;
  }

  int64_t Gid() const
  {
    return this->mStat.st_gid;
  }

  int64_t Rdev() const
  {
    return this->mStat.st_rdev;
  }

  int64_t Size() const
  {
    return this->mStat.st_size;
  }

  int64_t Blksize() const
  {
    return this->mStat.st_blksize;
  }

  int64_t Blocks() const
  {
    return this->mStat.st_blocks;
  }

  // @todo: should we free() this stuff?
  mozilla::dom::Date Atime() const
  {
    return mozilla::dom::Date((double)this->mStat.st_atime * 1000);
  }

  mozilla::dom::Date Mtime() const
  {
    return mozilla::dom::Date((double)this->mStat.st_mtime * 1000);
  }

  mozilla::dom::Date Ctime() const
  {
    return mozilla::dom::Date((double)this->mStat.st_ctime * 1000);
  }

  bool IsFile()
  {
    return (this->mStat.st_mode & S_IFMT) == S_IFREG;
  }

  bool IsDirectory()
  {
    return (this->mStat.st_mode & S_IFMT) == S_IFDIR;
  }

  bool IsBlockDevice()
  {
    return (this->mStat.st_mode & S_IFMT) == S_IFBLK;
  }

  bool IsCharacterDevice()
  {
    return (this->mStat.st_mode & S_IFMT) == S_IFCHR;
  }

  bool IsSymbolicLink()
  {
    return (this->mStat.st_mode & S_IFMT) == S_IFLNK;
  }

  bool IsFIFO()
  {
    return (this->mStat.st_mode & S_IFMT) == S_IFIFO;
  }

  bool IsSocket()
  {
    return (this->mStat.st_mode & S_IFMT) == S_IFSOCK;
  }

private:
  ~Stat() { }
  struct stat mStat;
  nsRefPtr<workers::WorkerGlobalScope> mScope;
};

} // namespace os
} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_os_Stat_h
