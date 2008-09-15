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
#include "ScrollbarThemeComposite.h"

#include "ChromeClient.h"
#include "Frame.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "Page.h"
#include "Scrollbar.h"
#include "ScrollbarClient.h"
#include "Settings.h"

namespace WebCore {

static Page* pageForScrollView(ScrollView* view)
{
    if (!view)
        return 0;
    if (!view->isFrameView())
        return 0;
    FrameView* frameView = static_cast<FrameView*>(view);
    if (!frameView->frame())
        return 0;
    return frameView->frame()->page();
}

bool ScrollbarThemeComposite::paint(Scrollbar* scrollbar, GraphicsContext* graphicsContext, const IntRect& damageRect)
{
    // Create the ScrollbarControlPartMask based on the damageRect
    ScrollbarControlPartMask scrollMask = NoPart;

    IntRect backButtonPaintRect;
    IntRect forwardButtonPaintRect;
    if (hasButtons(scrollbar)) {
        backButtonPaintRect = backButtonRect(scrollbar, true);
        if (damageRect.intersects(backButtonPaintRect))
            scrollMask |= BackButtonPart;
        forwardButtonPaintRect = forwardButtonRect(scrollbar, true);
        if (damageRect.intersects(forwardButtonPaintRect))
            scrollMask |= ForwardButtonPart;
    }

    IntRect startTrackRect;
    IntRect thumbRect;
    IntRect endTrackRect;
    IntRect trackPaintRect = trackRect(scrollbar, true);
    bool thumbPresent = hasThumb(scrollbar);
    if (thumbPresent) {
        IntRect track = trackRect(scrollbar);
        splitTrack(scrollbar, track, startTrackRect, thumbRect, endTrackRect);
        if (damageRect.intersects(thumbRect)) {
            scrollMask |= ThumbPart;
            if (trackIsSinglePiece()) {
                // The track is a single strip that paints under the thumb.
                // Add both components of the track to the mask.
                scrollMask |= BackTrackPart;
                scrollMask |= ForwardTrackPart;
            }
        }
        if (damageRect.intersects(startTrackRect))
            scrollMask |= BackTrackPart;
        if (damageRect.intersects(endTrackRect))
            scrollMask |= ForwardTrackPart;
    } else if (damageRect.intersects(trackPaintRect)) {
        scrollMask |= BackTrackPart;
        scrollMask |= ForwardTrackPart;
    }

    // FIXME: This API makes the assumption that the custom scrollbar's metrics will match
    // the theme's metrics.  This is not a valid assumption.  The ability for a client to paint
    // custom scrollbars should be removed once scrollbars can be styled via CSS.
    if (Page* page = pageForScrollView(scrollbar->parent())) {
        if (page->settings()->shouldPaintCustomScrollbars()) {
            float proportion = static_cast<float>(scrollbar->visibleSize()) / scrollbar->totalSize();
            float value = scrollbar->currentPos() / static_cast<float>(scrollbar->maximum());
            ScrollbarControlState s = 0;
            if (scrollbar->client()->isActive())
                s |= ActiveScrollbarState;
            if (scrollbar->isEnabled())
                s |= EnabledScrollbarState;
            if (scrollbar->pressedPart() != NoPart)
                s |= PressedScrollbarState;
            if (page->chrome()->client()->paintCustomScrollbar(graphicsContext,
                                                               scrollbar->frameGeometry(), 
                                                               scrollbar->controlSize(),
                                                               s, 
                                                               scrollbar->pressedPart(), 
                                                               scrollbar->orientation() == VerticalScrollbar,
                                                               value,
                                                               proportion,
                                                               scrollMask))
                return true;
        }
    }

    // Paint the track.
    if ((scrollMask & ForwardTrackPart) || (scrollMask & BackTrackPart)) {
        if (!thumbPresent || trackIsSinglePiece())
            paintTrack(graphicsContext, scrollbar, trackPaintRect, ForwardTrackPart | BackTrackPart);
        else {
            if (scrollMask & BackTrackPart)
                paintTrack(graphicsContext, scrollbar, startTrackRect, BackTrackPart);
            if (scrollMask & ForwardTrackPart)
                paintTrack(graphicsContext, scrollbar, endTrackRect, ForwardTrackPart);
        }
    }

    // Paint the back and forward buttons.
    if (scrollMask & BackButtonPart)
        paintButton(graphicsContext, scrollbar, backButtonPaintRect, BackButtonPart);
    if (scrollMask & ForwardButtonPart)
        paintButton(graphicsContext, scrollbar, forwardButtonPaintRect, ForwardButtonPart);
    
    // Paint the thumb.
    if (scrollMask & ThumbPart)
        paintThumb(graphicsContext, scrollbar, thumbRect);

    return true;
}

}