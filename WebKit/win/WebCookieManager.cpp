/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "WebKitDLL.h"
#include "WebCookieManager.h"

#if USE(CFNETWORK)
#include <CFNetwork/CFHTTPCookiesPriv.h>
#include <WebCore/CookieStorageWin.h>
#elif USE(CURL)
#include "NotImplemented.h"
#endif

using namespace WebCore;

// WebCookieManager -------------------------------------------------------

WebCookieManager* WebCookieManager::createInstance()
{
#if USE(CFNETWORK)
    WebCookieManager* manager = new WebCookieManager;
    manager->AddRef();
    return manager;    
#else
   return 0;
#endif
}

WebCookieManager::WebCookieManager()
    : m_refCount(0)
{
    gClassCount++;
    gClassNameCount.add("WebCookieManager");
}

WebCookieManager::~WebCookieManager()
{
    gClassCount--;
    gClassNameCount.remove("WebCookieManager");
}

// IUnknown ---------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebCookieManager::QueryInterface(REFIID riid, void** ppvObject)
{
#if USE(CFNETWORK)
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<WebCookieManager*>(this);
    else if (IsEqualGUID(riid, __uuidof(IWebCookieManager)))
        *ppvObject = static_cast<IWebCookieManager*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
#else
   notImplemented();
   return E_FAIL;
#endif
}

ULONG STDMETHODCALLTYPE WebCookieManager::AddRef()
{
    return ++m_refCount;
}

ULONG STDMETHODCALLTYPE WebCookieManager::Release()
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete this;

    return newRef;
}

// IWebCookieManager -------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebCookieManager::cookieStorage( 
    /* [retval][out] */ CFHTTPCookieStorageRef* storage)
{
#if USE(CFNETWORK)
   if (!storage)
        return E_POINTER;

    *storage = currentCookieStorage();
    return S_OK;
#else
   notImplemented();
   return E_FAIL;
#endif
}

HRESULT STDMETHODCALLTYPE WebCookieManager::setCookieStorage( 
    /* [in] */ CFHTTPCookieStorageRef storage)
{
#if USE(CFNETWORK)
    setCurrentCookieStorage(storage);
    return S_OK;
#else
   notImplemented();
   return E_FAIL;
#endif
}
