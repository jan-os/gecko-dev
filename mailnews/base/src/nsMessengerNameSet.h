/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */


#ifndef __nsMessengerNameSet_h
#define __nsMessengerNameSet_h

#include "nsIScriptExternalNameSet.h"
#include "nsIScriptContext.h"


class nsMessengerNameSet : public nsIScriptExternalNameSet
{

public:

  nsMessengerNameSet();
  virtual ~nsMessengerNameSet();

  NS_DECL_ISUPPORTS

  NS_IMETHOD InitializeClasses(nsIScriptContext *aScriptContext);
  NS_IMETHOD AddNameSet(nsIScriptContext* aScriptContext);

};

#endif
