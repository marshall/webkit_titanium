/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebInspectorClient.h"

#include "WebInspectorDelegate.h"
#include "WebKit.h"
#include "WebMutableURLRequest.h"
#include "WebNodeHighlight.h"
#include "WebView.h"

#pragma warning(push, 0)
#include <WebCore/BString.h>
#include <WebCore/Element.h>
#include <WebCore/FloatRect.h>
#include <WebCore/FrameView.h>
#include <WebCore/InspectorController.h>
#include <WebCore/Page.h>
#include <WebCore/RenderObject.h>
#include <WebCore/WindowMessageBroadcaster.h>
#pragma warning(pop)

#include <tchar.h>
#include <wtf/RetainPtr.h>

using namespace WebCore;

static LPCTSTR kWebInspectorWindowClassName = TEXT("WebInspectorWindowClass");
static ATOM registerWindowClass();
static LPCTSTR kWebInspectorPointerProp = TEXT("WebInspectorPointer");

static const unsigned defaultAttachedHeight = 300;

static const IntRect& defaultWindowRect()
{
    static IntRect rect(60, 200, 750, 650);
    return rect;
}

static CFBundleRef getWebKitBundle()
{
    return CFBundleGetBundleWithIdentifier(CFSTR("com.apple.WebKit"));
}

WebInspectorClient::WebInspectorClient(WebView* webView)
    : m_inspectedWebView(webView)
    , m_hwnd(0)
    , m_webViewHwnd(0)
    , m_attached(false)
{
    ASSERT(m_inspectedWebView);

    m_inspectedWebView->viewWindow((OLE_HANDLE*)&m_inspectedWebViewHwnd);

    // FIXME: Implement window size/position save/restore
#if 0
    [self setWindowFrameAutosaveName:@"Web Inspector"];
#endif
}

WebInspectorClient::~WebInspectorClient()
{
    if (m_hwnd)
        ::DestroyWindow(m_hwnd);
}

void WebInspectorClient::inspectorDestroyed()
{
    delete this;
}

Page* WebInspectorClient::createPage()
{
    if (m_webView)
        return core(m_webView.get());

    ASSERT(!m_hwnd);

    registerWindowClass();

    m_hwnd = ::CreateWindowEx(0, kWebInspectorWindowClassName, 0, WS_OVERLAPPEDWINDOW,
        defaultWindowRect().x(), defaultWindowRect().y(), defaultWindowRect().width(), defaultWindowRect().height(),
        0, 0, 0, 0);

    if (!m_hwnd)
        return 0;

    ::SetProp(m_hwnd, kWebInspectorPointerProp, reinterpret_cast<HANDLE>(this));

    m_webView.adoptRef(WebView::createInstance());

    if (FAILED(m_webView->setHostWindow((OLE_HANDLE)(ULONG64)m_hwnd)))
        return 0;

    RECT rect;
    GetClientRect(m_hwnd, &rect);
    if (FAILED(m_webView->initWithFrame(rect, 0, 0)))
        return 0;

    COMPtr<WebInspectorDelegate> delegate(AdoptCOM, WebInspectorDelegate::createInstance());
    if (FAILED(m_webView->setUIDelegate(delegate.get())))
        return 0;

    // Keep preferences separate from the rest of the client, making sure we are using expected preference values.
    // One reason this is good is that it keeps the inspector out of history via "private browsing".
    // FIXME: It's crazy that we have to do this song and dance to end up with
    // a private WebPreferences object, even within WebKit. We should make this
    // process simpler, and consider whether we can make it simpler for WebKit
    // clients as well.
    COMPtr<WebPreferences> tempPreferences(AdoptCOM, WebPreferences::createInstance());
    COMPtr<IWebPreferences> iPreferences;
    if (FAILED(tempPreferences->initWithIdentifier(BString(L"WebInspectorPreferences"), &iPreferences)))
        return 0;
    COMPtr<WebPreferences> preferences(Query, iPreferences);
    if (!preferences)
        return 0;
    if (FAILED(preferences->setAutosaves(FALSE)))
        return 0;
    if (FAILED(preferences->setPrivateBrowsingEnabled(TRUE)))
        return 0;
    if (FAILED(preferences->setLoadsImagesAutomatically(TRUE)))
        return 0;
    if (FAILED(preferences->setAuthorAndUserStylesEnabled(TRUE)))
        return 0;
    if (FAILED(preferences->setAllowsAnimatedImages(TRUE)))
        return 0;
    if (FAILED(preferences->setLoadsImagesAutomatically(TRUE)))
        return 0;
    if (FAILED(preferences->setPlugInsEnabled(FALSE)))
        return 0;
    if (FAILED(preferences->setJavaEnabled(FALSE)))
        return 0;
    if (FAILED(preferences->setUserStyleSheetEnabled(FALSE)))
        return 0;
    if (FAILED(preferences->setTabsToLinks(FALSE)))
        return 0;
    if (FAILED(preferences->setMinimumFontSize(0)))
        return 0;
    if (FAILED(preferences->setMinimumLogicalFontSize(9)))
        return 0;

    if (FAILED(m_webView->setPreferences(preferences.get())))
        return 0;

    m_webView->setProhibitsMainFrameScrolling(TRUE);

    if (FAILED(m_webView->viewWindow(reinterpret_cast<OLE_HANDLE*>(&m_webViewHwnd))))
        return 0;

    COMPtr<WebMutableURLRequest> request;
    request.adoptRef(WebMutableURLRequest::createInstance());

	CFStringRef urlStringRef = NULL;
	if (m_inspectorURL.length() == 0) {
		RetainPtr<CFURLRef> htmlURLRef(AdoptCF, CFBundleCopyResourceURL(getWebKitBundle(), CFSTR("inspector"), CFSTR("html"), CFSTR("inspector")));
		if (!htmlURLRef)
		    return 0;

		urlStringRef = ::CFURLGetString(htmlURLRef.get());
	}
	else {
		urlStringRef = m_inspectorURL.createCFString();
	}

	if (FAILED(request->initWithURL(BString(urlStringRef), WebURLRequestUseProtocolCachePolicy, 60)))
        return 0;

    if (FAILED(m_webView->topLevelFrame()->loadRequest(request.get())))
        return 0;

    return core(m_webView.get());
}


String WebInspectorClient::localizedStringsURL()
{
	if (m_localizedStringsURL.length() == 0) {
		RetainPtr<CFURLRef> url(AdoptCF, CFBundleCopyResourceURL(getWebKitBundle(), CFSTR("localizedStrings"), CFSTR("js"), 0));
		if (!url)
		    return String();

		return CFURLGetString(url.get());
	}
	else {
		return m_localizedStringsURL;
	}
}

void WebInspectorClient::setInspectorURL(const String& url)
{
	m_inspectorURL = url;
}

void WebInspectorClient::setLocalizedStringsURL(const String& url)
{
	m_localizedStringsURL = url;
}

void WebInspectorClient::showWindow()
{
    if (!m_hwnd)
        return;

    updateWindowTitle();
    ::SetWindowPos(m_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
    m_inspectedWebView->page()->inspectorController()->setWindowVisible(true);
}

void WebInspectorClient::closeWindow()
{
    if (!m_webView)
        return;

    ::ShowWindow(m_hwnd, SW_HIDE);
    m_inspectedWebView->page()->inspectorController()->setWindowVisible(false);
}

bool WebInspectorClient::windowVisible()
{
    return !!::IsWindowVisible(m_hwnd);
}

void WebInspectorClient::attachWindow()
{
    ASSERT(m_hwnd);
    ASSERT(m_webView);
    ASSERT(m_inspectedWebViewHwnd);

    if (m_attached)
        return;

    WindowMessageBroadcaster::addListener(m_inspectedWebViewHwnd, this);

    HWND hostWindow;
    if (FAILED(m_inspectedWebView->hostWindow((OLE_HANDLE*)&hostWindow)))
        return;

    m_webView->setHostWindow((OLE_HANDLE)(ULONG64)hostWindow);
    ::ShowWindow(m_hwnd, SW_HIDE);
    m_attached = true;

    ::SendMessage(hostWindow, WM_SIZE, 0, 0);

    if (m_highlight && m_highlight->isShowing())
        m_highlight->update();
}

void WebInspectorClient::detachWindow()
{
    if (!m_attached)
        return;

    WindowMessageBroadcaster::removeListener(m_inspectedWebViewHwnd, this);

    m_attached = false;

    m_webView->setHostWindow((OLE_HANDLE)(ULONG64)m_hwnd);
    ::ShowWindow(m_hwnd, SW_SHOW);
    ::SendMessage(m_hwnd, WM_SIZE, 0, 0);

    HWND hostWindow;
    if (SUCCEEDED(m_inspectedWebView->hostWindow((OLE_HANDLE*)&hostWindow)))
        ::SendMessage(hostWindow, WM_SIZE, 0, 0);

    if (m_highlight && m_highlight->isShowing())
        m_highlight->update();
}

void WebInspectorClient::setAttachedWindowHeight(unsigned height)
{
    // FIXME: implement this.
}

void WebInspectorClient::highlight(Node*)
{
    bool creatingHighlight = !m_highlight;

    if (creatingHighlight)
        m_highlight.set(new WebNodeHighlight(m_inspectedWebView));

    if (m_highlight->isShowing())
        m_highlight->update();
    else
        m_highlight->setShowsWhileWebViewIsVisible(true);

    if (creatingHighlight && IsWindowVisible(m_hwnd))
        m_highlight->placeBehindWindow(m_hwnd);
}

void WebInspectorClient::hideHighlight()
{
    if (m_highlight)
        m_highlight->setShowsWhileWebViewIsVisible(false);
}

void WebInspectorClient::inspectedURLChanged(const String& newURL)
{
    m_inspectedURL = newURL;
    updateWindowTitle();
}

void WebInspectorClient::updateWindowTitle()
{
    // FIXME: The series of appends should be replaced with a call to String::format()
    // when it can be figured out how to get the unicode em-dash to show up.
    String title = "Web Inspector ";
    title.append((UChar)0x2014); // em-dash
    title.append(' ');
    title.append(m_inspectedURL);
    ::SetWindowText(m_hwnd, title.charactersWithNullTermination());
}

LRESULT WebInspectorClient::onGetMinMaxInfo(WPARAM, LPARAM lParam)
{
    MINMAXINFO* info = reinterpret_cast<MINMAXINFO*>(lParam);
    POINT size = {400, 400};
    info->ptMinTrackSize = size;

    return 0;
}

LRESULT WebInspectorClient::onSize(WPARAM, LPARAM)
{
    RECT rect;
    ::GetClientRect(m_hwnd, &rect);

    ::SetWindowPos(m_webViewHwnd, 0, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER);

    return 0;
}

LRESULT WebInspectorClient::onClose(WPARAM, LPARAM)
{
    ::ShowWindow(m_hwnd, SW_HIDE);
    m_inspectedWebView->page()->inspectorController()->setWindowVisible(false);

    hideHighlight();

    return 0;
}

LRESULT WebInspectorClient::onSetFocus()
{
    SetFocus(m_webViewHwnd);
    return 0;
}

void WebInspectorClient::onWebViewWindowPosChanging(WPARAM, LPARAM lParam)
{
    ASSERT(m_attached);

    WINDOWPOS* windowPos = reinterpret_cast<WINDOWPOS*>(lParam);
    ASSERT_ARG(lParam, windowPos);

    if (windowPos->flags & SWP_NOSIZE)
        return;

    windowPos->cy -= defaultAttachedHeight;

    ::SetWindowPos(m_webViewHwnd, 0, windowPos->x, windowPos->y + windowPos->cy, windowPos->cx, defaultAttachedHeight, SWP_NOZORDER);
}

static LRESULT CALLBACK WebInspectorWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WebInspectorClient* client = reinterpret_cast<WebInspectorClient*>(::GetProp(hwnd, kWebInspectorPointerProp));
    if (!client)
        return ::DefWindowProc(hwnd, msg, wParam, lParam);

    switch (msg) {
        case WM_GETMINMAXINFO:
            return client->onGetMinMaxInfo(wParam, lParam);
        case WM_SIZE:
            return client->onSize(wParam, lParam);
        case WM_CLOSE:
            return client->onClose(wParam, lParam);
        case WM_SETFOCUS:
            return client->onSetFocus();
        default:
            break;
    }

    return ::DefWindowProc(hwnd, msg, wParam, lParam);
}

void WebInspectorClient::windowReceivedMessage(HWND, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_WINDOWPOSCHANGING:
            onWebViewWindowPosChanging(wParam, lParam);
            break;
        default:
            break;
    }
}

static ATOM registerWindowClass()
{
    static bool haveRegisteredWindowClass = false;

    if (haveRegisteredWindowClass)
        return true;

    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = 0;
    wcex.lpfnWndProc    = WebInspectorWndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = 0;
    wcex.hIcon          = 0;
    wcex.hCursor        = LoadCursor(0, IDC_ARROW);
    wcex.hbrBackground  = 0;
    wcex.lpszMenuName   = 0;
    wcex.lpszClassName  = kWebInspectorWindowClassName;
    wcex.hIconSm        = 0;

    haveRegisteredWindowClass = true;

    return ::RegisterClassEx(&wcex);
}
