[Exposed=(Worker,System)]
interface OsManager {
  [Throws]
  OsManagerFile open(DOMString path, long access, long permission);
  [Throws]
  Uint8Array read(OsManagerFile file, long bytes);
  [Throws]
  long write(OsManagerFile file, Uint8Array buffer);
  long close(OsManagerFile file);

  [Throws]
  OsManagerStat lstat(DOMString path);
  [Throws]
  OsManagerStat stat(DOMString path);
  [Throws]
  OsManagerStat fstat(OsManagerFile file);

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
interface OsManagerFile {};
