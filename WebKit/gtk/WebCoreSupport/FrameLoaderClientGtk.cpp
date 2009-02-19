/*
 *  Copyright (C) 2007, 2008 Alp Toker <alp@atoker.com>
 *  Copyright (C) 2007, 2008 Holger Hans Peter Freyther
 *  Copyright (C) 2007 Christian Dywan <christian@twotoasts.de>
 *  Copyright (C) 2008 Collabora Ltd.  All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "FrameLoaderClientGtk.h"

#include "Color.h"
#include "DocumentLoader.h"
#include "FrameLoader.h"
#include "FrameView.h"
#include "FrameTree.h"
#include "HTMLFormElement.h"
#include "HTMLFrameElement.h"
#include "HTMLFrameOwnerElement.h"
#include "HTMLNames.h"
#include "JSDOMWindow.h"
#include "Language.h"
#include "MIMETypeRegistry.h"
#include "NotImplemented.h"
#include "PlatformString.h"
#include "PluginDatabase.h"
#include "RenderPart.h"
#include "ResourceRequest.h"
#include "CString.h"
#include "ProgressTracker.h"
#include "JSDOMBinding.h"
#include "ScriptController.h"
#include "webkitwebview.h"
#include "webkitnetworkrequest.h"
#include "webkitwebframe.h"
#include "webkitprivate.h"

#include <JavaScriptCore/APICast.h>
#include <stdio.h>
#if PLATFORM(UNIX)
#include <sys/utsname.h>
#endif

using namespace WebCore;

namespace WebKit {

FrameLoaderClient::FrameLoaderClient(WebKitWebFrame* frame)
    : m_frame(frame)
    , m_userAgent("")
    , m_pluginView(0)
    , m_hasSentResponseToPlugin(false)
{
    ASSERT(m_frame);
}

static String agentPlatform()
{
#ifdef GDK_WINDOWING_X11
    return "X11";
#elif defined(GDK_WINDOWING_WIN32)
    return "Windows";
#elif defined(GDK_WINDOWING_QUARTZ)
    return "Macintosh";
#elif defined(GDK_WINDOWING_DIRECTFB)
    return "DirectFB";
#else
    notImplemented();
    return "Unknown";
#endif
}

static String agentOS()
{
#if PLATFORM(DARWIN)
#if PLATFORM(X86)
    return "Intel Mac OS X";
#else
    return "PPC Mac OS X";
#endif
#elif PLATFORM(UNIX)
    struct utsname name;
    if (uname(&name) != -1)
        return String::format("%s %s", name.sysname, name.machine);
    else
        return "Unknown";
#elif PLATFORM(WIN_OS)
    // FIXME: Compute the Windows version
    return "Windows";
#else
    notImplemented();
    return "Unknown";
#endif
}

static String composeUserAgent()
{
    // This is a liberal interpretation of http://www.mozilla.org/build/revised-user-agent-strings.html
    // See also http://developer.apple.com/internet/safari/faq.html#anchor2

    String ua;

    // Product
    ua += "Mozilla/5.0";

    // Comment
    ua += " (";
    ua += agentPlatform(); // Platform
    ua += "; U; "; // Security
    ua += agentOS(); // OS-or-CPU
    ua += "; ";
    ua += defaultLanguage(); // Localization information
    ua += ") ";

    // WebKit Product
    // FIXME: The WebKit version is hardcoded
    static const String webKitVersion = "528.5+";
    ua += "AppleWebKit/" + webKitVersion;
    ua += " (KHTML, like Gecko, ";
    // We mention Safari since many broken sites check for it (OmniWeb does this too)
    // We re-use the WebKit version, though it doesn't seem to matter much in practice
    ua += "Safari/" + webKitVersion;
    ua += ") ";

    // Vendor Product
    ua += g_get_prgname();

    return ua;
}

String FrameLoaderClient::userAgent(const KURL&)
{
    if (m_userAgent.isEmpty())
        m_userAgent = composeUserAgent();

    return m_userAgent;
}

WTF::PassRefPtr<WebCore::DocumentLoader> FrameLoaderClient::createDocumentLoader(const WebCore::ResourceRequest& request, const SubstituteData& substituteData)
{
    return DocumentLoader::create(request, substituteData);
}

void FrameLoaderClient::dispatchWillSubmitForm(FramePolicyFunction policyFunction, PassRefPtr<FormState>)
{
    // FIXME: This is surely too simple
    ASSERT(policyFunction);
    if (!policyFunction)
        return;
    (core(m_frame)->loader()->*policyFunction)(PolicyUse);
}


void FrameLoaderClient::committedLoad(DocumentLoader* loader, const char* data, int length)
{
    if (!m_pluginView) {
        ASSERT(loader->frame());
        // Setting the encoding on the frame loader is our way to get work done that is normally done
        // when the first bit of data is received, even for the case of a document with no data (like about:blank).
        String encoding = loader->overrideEncoding();
        bool userChosen = !encoding.isNull();
        if (!userChosen)
            encoding = loader->response().textEncodingName();

        FrameLoader* frameLoader = loader->frameLoader();
        frameLoader->setEncoding(encoding, userChosen);
        if (data)
            frameLoader->addData(data, length);
    }

    if (m_pluginView) {
        if (!m_hasSentResponseToPlugin) {
            m_pluginView->didReceiveResponse(loader->response());
            m_hasSentResponseToPlugin = true;
        }
        m_pluginView->didReceiveData(data, length);
    }
}

void FrameLoaderClient::dispatchDidReceiveAuthenticationChallenge(DocumentLoader*, unsigned long  identifier, const AuthenticationChallenge&)
{
    notImplemented();
}

void FrameLoaderClient::dispatchDidCancelAuthenticationChallenge(DocumentLoader*, unsigned long  identifier, const AuthenticationChallenge&)
{
    notImplemented();
}

void FrameLoaderClient::dispatchWillSendRequest(DocumentLoader*, unsigned long, ResourceRequest&, const ResourceResponse&)
{
    notImplemented();
}

void FrameLoaderClient::assignIdentifierToInitialRequest(unsigned long identifier, DocumentLoader*, const ResourceRequest&)
{
    notImplemented();
}

void FrameLoaderClient::postProgressStartedNotification()
{
    WebKitWebView* webView = getViewFromFrame(m_frame);
    g_signal_emit_by_name(webView, "load-started", m_frame);
}

void FrameLoaderClient::postProgressEstimateChangedNotification()
{
    WebKitWebView* webView = getViewFromFrame(m_frame);
    Page* corePage = core(webView);

    g_signal_emit_by_name(webView, "load-progress-changed", lround(corePage->progress()->estimatedProgress()*100));
}

void FrameLoaderClient::postProgressFinishedNotification()
{
    WebKitWebView* webView = getViewFromFrame(m_frame);

    g_signal_emit_by_name(webView, "load-finished", m_frame);
}

void FrameLoaderClient::frameLoaderDestroyed()
{
    g_object_unref(m_frame);
    m_frame = 0;
    delete this;
}

void FrameLoaderClient::dispatchDidReceiveResponse(DocumentLoader*, unsigned long, const ResourceResponse& response)
{
    m_response = response;
}

void FrameLoaderClient::dispatchDecidePolicyForMIMEType(FramePolicyFunction policyFunction, const String&, const ResourceRequest&)
{
    // FIXME: we need to call directly here (comment copied from Qt version)
    ASSERT(policyFunction);
    if (!policyFunction)
        return;
    (core(m_frame)->loader()->*policyFunction)(PolicyUse);
}

void FrameLoaderClient::dispatchDecidePolicyForNewWindowAction(FramePolicyFunction policyFunction, const NavigationAction&, const ResourceRequest& resourceRequest, PassRefPtr<FormState>, const String& frameName)
{
    ASSERT(policyFunction);
    if (!policyFunction)
        return;

    WebKitWebView* webView = getViewFromFrame(m_frame);
    WebKitNetworkRequest* request = webkit_network_request_new(strdup(resourceRequest.url().string().utf8().data()));
    WebKitNavigationResponse response;
    gchar* frame_name = strdup(frameName.utf8().data());

    printf("in-args: %x %x %x\n", (int) m_frame, (int) request, (int) frame_name);
    g_signal_emit_by_name(webView, "new-window-navigation-requested", m_frame, request, frame_name, &response);

    free(frame_name);
    g_object_unref(request);

    if (response == WEBKIT_NAVIGATION_RESPONSE_IGNORE) {
        (core(m_frame)->loader()->*policyFunction)(PolicyIgnore);
    } else {
        (core(m_frame)->loader()->*policyFunction)(PolicyUse);
    }
}

void FrameLoaderClient::dispatchDecidePolicyForNavigationAction(FramePolicyFunction policyFunction, const NavigationAction& action, const ResourceRequest& resourceRequest, PassRefPtr<FormState>)
{
    ASSERT(policyFunction);
    if (!policyFunction)
        return;

    WebKitWebView* webView = getViewFromFrame(m_frame);
    WebKitNetworkRequest* request = webkit_network_request_new(resourceRequest.url().string().utf8().data());
    WebKitNavigationResponse response;

    printf("str: %s\n", webkit_network_request_get_uri(request));
    g_signal_emit_by_name(webView, "navigation-requested", m_frame, request, &response);

    g_object_unref(request);

    if (response == WEBKIT_NAVIGATION_RESPONSE_IGNORE) {
        (core(m_frame)->loader()->*policyFunction)(PolicyIgnore);
        return;
    }

    (core(m_frame)->loader()->*policyFunction)(PolicyUse);
}

Widget* FrameLoaderClient::createPlugin(const IntSize& pluginSize, Element* element, const KURL& url, const Vector<String>& paramNames, const Vector<String>& paramValues, const String& mimeType, bool loadManually)
{
    PluginView* pluginView = PluginView::create(core(m_frame), pluginSize, element, url, paramNames, paramValues, mimeType, loadManually);

    if (pluginView->status() == PluginStatusLoadedSuccessfully)
        return pluginView;

    return 0;
}

PassRefPtr<Frame> FrameLoaderClient::createFrame(const KURL& url, const String& name, HTMLFrameOwnerElement* ownerElement,
                                                 const String& referrer, bool allowsScrolling, int marginWidth, int marginHeight)
{
    Frame* coreFrame = core(webFrame());

    ASSERT(core(getViewFromFrame(webFrame())) == coreFrame->page());
    WebKitWebFrame* gtkFrame = WEBKIT_WEB_FRAME(webkit_web_frame_init_with_web_view(getViewFromFrame(webFrame()), ownerElement));
    RefPtr<Frame> childFrame(adoptRef(core(gtkFrame)));

    coreFrame->tree()->appendChild(childFrame);

    childFrame->tree()->setName(name);
    childFrame->init();
    childFrame->loader()->loadURL(url, referrer, String(), FrameLoadTypeRedirectWithLockedHistory, 0, 0);

    // The frame's onload handler may have removed it from the document.
    if (!childFrame->tree()->parent())
        return 0;

    return childFrame.release();
}

void FrameLoaderClient::redirectDataToPlugin(Widget* pluginWidget)
{
    ASSERT(!m_pluginView);
    m_pluginView = static_cast<PluginView*>(pluginWidget);
    m_hasSentResponseToPlugin = false;
}

Widget* FrameLoaderClient::createJavaAppletWidget(const IntSize&, Element*, const KURL& baseURL,
                                                  const Vector<String>& paramNames, const Vector<String>& paramValues)
{
    notImplemented();
    return 0;
}

ObjectContentType FrameLoaderClient::objectContentType(const KURL& url, const String& mimeType)
{
    String type = mimeType;
    // We don't use MIMETypeRegistry::getMIMETypeForPath() because it returns "application/octet-stream" upon failure
    if (type.isEmpty())
        type = MIMETypeRegistry::getMIMETypeForExtension(url.path().substring(url.path().reverseFind('.') + 1));

    if (type.isEmpty())
        return WebCore::ObjectContentFrame;

    if (MIMETypeRegistry::isSupportedImageMIMEType(type))
        return WebCore::ObjectContentImage;

    if (PluginDatabase::installedPlugins()->isMIMETypeRegistered(mimeType))
        return WebCore::ObjectContentNetscapePlugin;

    if (MIMETypeRegistry::isSupportedNonImageMIMEType(type))
        return WebCore::ObjectContentFrame;

    return WebCore::ObjectContentNone;
}

String FrameLoaderClient::overrideMediaType() const
{
    notImplemented();
    return String();
}

void FrameLoaderClient::windowObjectCleared()
{
    // Is this obsolete now?
    g_signal_emit_by_name(m_frame, "cleared");

    Frame* coreFrame = core(webFrame());
    ASSERT(coreFrame);

    Settings* settings = coreFrame->settings();
    if (!settings || !settings->isJavaScriptEnabled())
        return;

    // TODO: Consider using g_signal_has_handler_pending() to avoid the overhead
    // when there are no handlers.
    JSGlobalContextRef context = toGlobalRef(coreFrame->script()->globalObject()->globalExec());
    JSObjectRef windowObject = toRef(coreFrame->script()->globalObject());
    ASSERT(windowObject);

    WebKitWebView* webView = getViewFromFrame(m_frame);
    g_signal_emit_by_name(webView, "window-object-cleared", m_frame, context, windowObject);

    // TODO: Re-attach debug clients if present.
    // The Win port has an example of how we might do this.
}

void FrameLoaderClient::didPerformFirstNavigation() const
{
}

void FrameLoaderClient::registerForIconNotification(bool)
{
    notImplemented();
}

void FrameLoaderClient::setMainFrameDocumentReady(bool)
{
    // this is only interesting once we provide an external API for the DOM
}

bool FrameLoaderClient::hasWebView() const
{
    notImplemented();
    return true;
}

void FrameLoaderClient::dispatchDidFinishLoad()
{
    g_signal_emit_by_name(m_frame, "load-done", true);
}

void FrameLoaderClient::frameLoadCompleted()
{
    notImplemented();
}

void FrameLoaderClient::saveViewStateToItem(HistoryItem*)
{
    notImplemented();
}

void FrameLoaderClient::restoreViewState()
{
    notImplemented();
}

bool FrameLoaderClient::shouldGoToHistoryItem(HistoryItem* item) const
{
    // FIXME: This is a very simple implementation. More sophisticated
    // implementation would delegate the decision to a PolicyDelegate.
    // See mac implementation for example.
    return item != 0;
}

void FrameLoaderClient::makeRepresentation(DocumentLoader*)
{
    notImplemented();
}

void FrameLoaderClient::forceLayout()
{
    core(m_frame)->forceLayout(true);
}

void FrameLoaderClient::forceLayoutForNonHTML()
{
    notImplemented();
}

void FrameLoaderClient::setCopiesOnScroll()
{
    notImplemented();
}

void FrameLoaderClient::detachedFromParent2()
{
    notImplemented();
}

void FrameLoaderClient::detachedFromParent3()
{
    notImplemented();
}

void FrameLoaderClient::loadedFromCachedPage()
{
    notImplemented();
}

void FrameLoaderClient::dispatchDidHandleOnloadEvents()
{
    notImplemented();
}

void FrameLoaderClient::dispatchDidReceiveServerRedirectForProvisionalLoad()
{
    notImplemented();
}

void FrameLoaderClient::dispatchDidCancelClientRedirect()
{
    notImplemented();
}

void FrameLoaderClient::dispatchWillPerformClientRedirect(const KURL&, double, double)
{
    notImplemented();
}

void FrameLoaderClient::dispatchDidChangeLocationWithinPage()
{
    notImplemented();
}

void FrameLoaderClient::dispatchWillClose()
{
    notImplemented();
}

void FrameLoaderClient::dispatchDidReceiveIcon()
{
    WebKitWebView* webView = getViewFromFrame(m_frame);

    g_signal_emit_by_name(webView, "icon-loaded", m_frame);
}

void FrameLoaderClient::dispatchDidStartProvisionalLoad()
{
}

void FrameLoaderClient::dispatchDidReceiveTitle(const String& title)
{
    g_signal_emit_by_name(m_frame, "title-changed", title.utf8().data());

    WebKitWebView* webView = getViewFromFrame(m_frame);
    if (m_frame == webkit_web_view_get_main_frame(webView))
        g_signal_emit_by_name(webView, "title-changed", m_frame, title.utf8().data());
}

void FrameLoaderClient::dispatchDidCommitLoad()
{
    /* Update the URI once first data has been received.
     * This means the URI is valid and successfully identify the page that's going to be loaded.
     */
    WebKitWebFramePrivate* frameData = WEBKIT_WEB_FRAME_GET_PRIVATE(m_frame);
    g_free(frameData->uri);
    frameData->uri = g_strdup(core(m_frame)->loader()->url().prettyURL().utf8().data());

    g_signal_emit_by_name(m_frame, "load-committed");

    WebKitWebView* webView = getViewFromFrame(m_frame);
    if (m_frame == webkit_web_view_get_main_frame(webView))
        g_signal_emit_by_name(webView, "load-committed", m_frame);
}

void FrameLoaderClient::dispatchDidFinishDocumentLoad()
{
    notImplemented();
}

void FrameLoaderClient::dispatchDidFirstLayout()
{
    notImplemented();
}

void FrameLoaderClient::dispatchShow()
{
    WebKitWebView* webView = getViewFromFrame(m_frame);
    webkit_web_view_notify_ready(webView);
}

void FrameLoaderClient::cancelPolicyCheck()
{
    notImplemented();
}

void FrameLoaderClient::dispatchDidLoadMainResource(DocumentLoader*)
{
    notImplemented();
}

void FrameLoaderClient::revertToProvisionalState(DocumentLoader*)
{
    notImplemented();
}

void FrameLoaderClient::willChangeTitle(DocumentLoader*)
{
    notImplemented();
}

void FrameLoaderClient::didChangeTitle(DocumentLoader *l)
{
    setTitle(l->title(), l->url());
}

bool FrameLoaderClient::canHandleRequest(const ResourceRequest&) const
{
    notImplemented();
    return true;
}

bool FrameLoaderClient::canShowMIMEType(const String&) const
{
    notImplemented();
    return true;
}

bool FrameLoaderClient::representationExistsForURLScheme(const String&) const
{
    notImplemented();
    return false;
}

String FrameLoaderClient::generatedMIMETypeForURLScheme(const String&) const
{
    notImplemented();
    return String();
}

void FrameLoaderClient::finishedLoading(DocumentLoader* documentLoader)
{
    if (!m_pluginView)
        committedLoad(documentLoader, 0, 0);
    else {
        m_pluginView->didFinishLoading();
        m_pluginView = 0;
        m_hasSentResponseToPlugin = false;
    }
}


void FrameLoaderClient::provisionalLoadStarted()
{
    notImplemented();
}

void FrameLoaderClient::didFinishLoad() {
    notImplemented();
}

void FrameLoaderClient::prepareForDataSourceReplacement() { notImplemented(); }

void FrameLoaderClient::setTitle(const String& title, const KURL& url)
{
    WebKitWebFramePrivate* frameData = WEBKIT_WEB_FRAME_GET_PRIVATE(m_frame);
    g_free(frameData->title);
    frameData->title = g_strdup(title.utf8().data());
}

void FrameLoaderClient::dispatchDidReceiveContentLength(DocumentLoader*, unsigned long identifier, int lengthReceived)
{
    notImplemented();
}

void FrameLoaderClient::dispatchDidFinishLoading(DocumentLoader*, unsigned long identifier)
{
    notImplemented();
}

void FrameLoaderClient::dispatchDidFailLoading(DocumentLoader*, unsigned long identifier, const ResourceError&)
{
    notImplemented();
}

bool FrameLoaderClient::dispatchDidLoadResourceFromMemoryCache(DocumentLoader*, const ResourceRequest&, const ResourceResponse&, int length)
{
    notImplemented();
    return false;
}

void FrameLoaderClient::dispatchDidFailProvisionalLoad(const ResourceError&)
{
    g_signal_emit_by_name(m_frame, "load-done", false);
}

void FrameLoaderClient::dispatchDidFailLoad(const ResourceError&)
{
    g_signal_emit_by_name(m_frame, "load-done", false);
}

void FrameLoaderClient::download(ResourceHandle*, const ResourceRequest&, const ResourceRequest&, const ResourceResponse&)
{
    notImplemented();
}

ResourceError FrameLoaderClient::cancelledError(const ResourceRequest&)
{
    notImplemented();
    return ResourceError();
}

ResourceError FrameLoaderClient::blockedError(const ResourceRequest&)
{
    notImplemented();
    return ResourceError();
}

ResourceError FrameLoaderClient::cannotShowURLError(const ResourceRequest&)
{
    notImplemented();
    return ResourceError();
}

ResourceError FrameLoaderClient::interruptForPolicyChangeError(const ResourceRequest&)
{
    notImplemented();
    return ResourceError();
}

ResourceError FrameLoaderClient::cannotShowMIMETypeError(const ResourceResponse&)
{
    notImplemented();
    return ResourceError();
}

ResourceError FrameLoaderClient::fileDoesNotExistError(const ResourceResponse&)
{
    notImplemented();
    return ResourceError();
}

ResourceError FrameLoaderClient::pluginWillHandleLoadError(const ResourceResponse&)
{
    notImplemented();
    return ResourceError();
}

bool FrameLoaderClient::shouldFallBack(const ResourceError&)
{
    notImplemented();
    return false;
}

bool FrameLoaderClient::canCachePage() const
{
    return true;
}

Frame* FrameLoaderClient::dispatchCreatePage()
{
    WebKitWebView* webView = getViewFromFrame(m_frame);
    WebKitWebView* newWebView = 0;

    g_signal_emit_by_name(webView, "create-web-view", m_frame, &newWebView);

    if (!newWebView)
        return 0;

    WebKitWebViewPrivate* privateData = WEBKIT_WEB_VIEW_GET_PRIVATE(newWebView);
    return core(privateData->mainFrame);
}

void FrameLoaderClient::dispatchUnableToImplementPolicy(const ResourceError&)
{
    notImplemented();
}

void FrameLoaderClient::setMainDocumentError(DocumentLoader*, const ResourceError& error)
{
    if (m_pluginView) {
        m_pluginView->didFail(error);
        m_pluginView = 0;
        m_hasSentResponseToPlugin = false;
    }
}

void FrameLoaderClient::startDownload(const ResourceRequest&)
{
    notImplemented();
}

void FrameLoaderClient::updateGlobalHistory()
{
    notImplemented();
}

void FrameLoaderClient::savePlatformDataToCachedPage(CachedPage*)
{
}

void FrameLoaderClient::transitionToCommittedFromCachedPage(CachedPage*)
{
}

void FrameLoaderClient::transitionToCommittedForNewPage()
{
    WebKitWebView* containingWindow = getViewFromFrame(m_frame);
    IntSize size = IntSize(GTK_WIDGET(containingWindow)->allocation.width,
                           GTK_WIDGET(containingWindow)->allocation.height);
    bool transparent = webkit_web_view_get_transparent(containingWindow);
    Color backgroundColor = transparent ? WebCore::Color::transparent : WebCore::Color::white;
    Frame* frame = core(m_frame);
    ASSERT(frame);

    WebCore::FrameLoaderClient::transitionToCommittedForNewPage(frame, size, backgroundColor, transparent);

    // We need to do further manipulation on the FrameView if it was the mainFrame
    if (frame != frame->page()->mainFrame())
        return;

    WebKitWebViewPrivate* priv = WEBKIT_WEB_VIEW_GET_PRIVATE(containingWindow);
    frame->view()->setGtkAdjustments(priv->horizontalAdjustment, priv->verticalAdjustment);
}

}
