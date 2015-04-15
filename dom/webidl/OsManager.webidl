[Exposed=(Worker,System)]
interface OsManager {
  [Throws]
  OsManagerFd open(DOMString path, long access, optional long permission = 0);
  [Throws]
  Uint8Array read(OsManagerFd fd, long bytes);
  [Throws]
  long write(OsManagerFd fd, Uint8Array buffer);
  [Throws]
  long close(OsManagerFd fd); // @todo: should be void

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

  /* Missing according to emscripten:
   * utimes
   * truncate
   * mkdir (open?)
   * rename
   * unlink
   * rmdir
   * readdir
   * symlink
   * readlink
   */

  readonly attribute long RDONLY;
  readonly attribute long WRONLY;
  readonly attribute long RDWR;
  readonly attribute long APPEND;
  readonly attribute long CREAT;
  readonly attribute long DSYNC;
  readonly attribute long EXCL;
  readonly attribute long NOCTTY;
  readonly attribute long NONBLOCK;
  readonly attribute long SYNC;
  readonly attribute long TRUNC;

  readonly attribute long IWRITE;
  readonly attribute long IREAD;

  readonly attribute long ISUID;
  readonly attribute long ISGID;
  readonly attribute long ISVTX;
  readonly attribute long IRUSR;
  readonly attribute long IWUSR;
  readonly attribute long IXUSR;
  readonly attribute long IRGRP;
  readonly attribute long IWGRP;
  readonly attribute long IXGRP;
  readonly attribute long IROTH;
  readonly attribute long IWOTH;
  readonly attribute long IXOTH;
  readonly attribute long IRWXU;
  readonly attribute long IRWXG;
  readonly attribute long IRWXO;

  // Should have some attribute in manifest that specifies access to this
  // dir... As it's platform specific.
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
