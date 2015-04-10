#ifndef mozilla_dom_os_StatSerializer_h
#define mozilla_dom_os_StatSerializer_h

#include "ipc/IPCMessageUtils.h"
#include "mozilla/dom/os/Stat.h"

using mozilla::AutoJSContext;
using mozilla::dom::os;

namespace IPC {
template <>
struct ParamTraits<Stat*>
{
  typedef Stat* paramType;

  // Function to serialize a MobileCallForwardingOptions.
  static void Write(Message *aMsg, const paramType& aParam)
  {
    bool isNull = !aParam;
    WriteParam(aMsg, isNull);
    // If it is a null object, then we are done.
    if (isNull) {
      return;
    }
  }

  // Function to de-serialize a MobileCallForwardingOptions.
  static bool Read(const Message *aMsg, void **aIter, paramType* aResult)
  {
    // Check if is the null pointer we have transfered.
    bool isNull;
    if (!ReadParam(aMsg, aIter, &isNull)) {
      return false;
    }

    if (isNull) {
      *aResult = nullptr;
      return true;
    }

    // We release this ref after receiver finishes processing.
    // NS_ADDREF(*aResult);

    return true;
  }
};
}

#endif
