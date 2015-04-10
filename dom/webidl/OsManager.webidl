[Exposed=(Worker,System)]
interface OsManager {
  [Throws]
  OsManagerFile open(DOMString path, long access, long permission);
  [Throws]
  Uint8Array read(OsManagerFile file, long bytes);
  long write(OsManagerFile file, Uint8Array buffer, long bytes);
  long close(OsManagerFile file);

  [Throws]
  OsManagerStat lstat(DOMString path);
  [Throws]
  OsManagerStat stat(DOMString path);

  readonly attribute long RDONLY;
  readonly attribute long WRONLY;
  readonly attribute long RDWR;
  readonly attribute long APPEND;
  readonly attribute long IWRITE;
  readonly attribute long IREAD;
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
