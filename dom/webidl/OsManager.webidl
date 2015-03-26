[Exposed=(Worker,System)]
interface OsManager {
  // hmm, is long enough? JS number is not big enough I think...
  
  DOMString fopen(DOMString path, DOMString mode);
  long fclose(DOMString ptr);
};
