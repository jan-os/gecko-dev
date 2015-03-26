[Exposed=(Worker,System)]
interface OsManager {
  DOMString fopen(DOMString path, DOMString mode);
  long fclose(DOMString ptr);

  [Throws]
  OsManagerStat lstat(DOMString path);
  [Throws]
  OsManagerStat stat(DOMString path);
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
