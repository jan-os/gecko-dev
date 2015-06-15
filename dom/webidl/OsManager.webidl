/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

[Exposed=(Worker,System), AvailableIn=CertifiedApps]
interface OsManager {
  [Throws]
  OsManagerFd open(DOMString path, long access, optional long permission = 0);
  [Throws]
  Uint8Array read(OsManagerFd fd, long bytes);
  [Throws]
  long write(OsManagerFd fd, Uint8Array buffer);
  [Throws]
  void close(OsManagerFd fd);

  [Throws]
  OsManagerStat lstat(DOMString path);
  [Throws]
  OsManagerStat stat(DOMString path);
  [Throws]
  OsManagerStat fstat(OsManagerFd fd);

  [Throws]
  void chmod(DOMString path, long mode);
  [Throws]
  void fchmod(OsManagerFd fd, long mode);

  [Throws]
  void unlink(DOMString path);

  [Throws]
  void utimes(DOMString path, Date actime, Date modtime);
  [Throws]
  void lutimes(DOMString path, Date actime, Date modtime);
  [Throws]
  void futimes(OsManagerFd fd, Date actime, Date modtime);

  [Throws]
  void truncate(DOMString path, long length);
  [Throws]
  void ftruncate(OsManagerFd fd, long length);

  [Throws]
  void mkdir(DOMString path, long mode);
  [Throws]
  void rmdir(DOMString path);

  [Throws]
  void rename(DOMString oldPath, DOMString newPath);

  [Throws]
  sequence<DOMString> readdir(DOMString path);

  [Throws]
  void symlink(DOMString path1, DOMString path2);
  [Throws]
  DOMString readlink(DOMString path);

  /*
   * seek/lseek/fseek
   */

  [Constant]
  readonly attribute long RDONLY;
  [Constant]
  readonly attribute long WRONLY;
  [Constant]
  readonly attribute long RDWR;
  [Constant]
  readonly attribute long APPEND;
  [Constant]
  readonly attribute long CREAT;
  [Constant]
  readonly attribute long DSYNC;
  [Constant]
  readonly attribute long EXCL;
  [Constant]
  readonly attribute long NOCTTY;
  [Constant]
  readonly attribute long NONBLOCK;
  [Constant]
  readonly attribute long SYNC;
  [Constant]
  readonly attribute long TRUNC;

  [Constant]
  readonly attribute long IWRITE;
  [Constant]
  readonly attribute long IREAD;

  [Constant]
  readonly attribute long ISUID;
  [Constant]
  readonly attribute long ISGID;
  [Constant]
  readonly attribute long ISVTX;
  [Constant]
  readonly attribute long IRUSR;
  [Constant]
  readonly attribute long IWUSR;
  [Constant]
  readonly attribute long IXUSR;
  [Constant]
  readonly attribute long IRGRP;
  [Constant]
  readonly attribute long IWGRP;
  [Constant]
  readonly attribute long IXGRP;
  [Constant]
  readonly attribute long IROTH;
  [Constant]
  readonly attribute long IWOTH;
  [Constant]
  readonly attribute long IXOTH;
  [Constant]
  readonly attribute long IRWXU;
  [Constant]
  readonly attribute long IRWXG;
  [Constant]
  readonly attribute long IRWXO;

  // Should have some attribute in manifest that specifies access to this
  // dir... As it's platform specific.
  [Constant, Throws]
  readonly attribute DOMString TEMP_DIR;
};

[Exposed=(Worker,System)]
interface OsManagerStat {
  readonly attribute long dev;
  readonly attribute long ino;
  readonly attribute long mode;
  readonly attribute long nlink;
  readonly attribute long uid;
  readonly attribute long gid;
  readonly attribute long rdev;
  readonly attribute long size;
  readonly attribute long blksize;
  readonly attribute long blocks;
  readonly attribute Date atime;
  readonly attribute Date mtime;
  readonly attribute Date ctime;

  boolean isFile();
  boolean isDirectory();
  boolean isBlockDevice();
  boolean isCharacterDevice();
  boolean isSymbolicLink();
  boolean isFIFO();
  boolean isSocket();
};

[Exposed=(Worker,System)]
interface OsManagerFd {};
