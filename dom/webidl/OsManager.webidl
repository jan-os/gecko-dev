[Exposed=(Worker,System)]
interface OsManager {
  DOMString fopen(DOMString path, DOMString mode);
  long fclose(DOMString ptr);
};
