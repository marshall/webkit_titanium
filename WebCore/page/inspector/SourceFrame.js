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

WebInspector.SourceFrame = function(element, addBreakpointDelegate)
{
    this.messages = [];
    this.breakpoints = [];

    this.addBreakpointDelegate = addBreakpointDelegate;

    this.element = element || document.createElement("iframe");
    this.element.addStyleClass("source-view-frame");
    this.element.setAttribute("viewsource", "true");

    this.element.addEventListener("load", this._loaded.bind(this), false);
}

WebInspector.SourceFrame.prototype = {
    get executionLine()
    {
        return this._executionLine;
    },

    set executionLine(x)
    {
        if (this._executionLine === x)
            return;

        var previousLine = this._executionLine;
        this._executionLine = x;

        this._updateExecutionLine(previousLine);
    },

    sourceRow: function(lineNumber)
    {
        if (!lineNumber || !this.element.contentDocument)
            return;

        var table = this.element.contentDocument.getElementsByTagName("table")[0];
        if (!table)
            return;

        var rows = table.rows;

        // Line numbers are a 1-based index, but the rows collection is 0-based.
        --lineNumber;
        if (lineNumber >= rows.length)
            lineNumber = rows.length - 1;

        return rows[lineNumber];
    },

    lineNumberForSourceRow: function(sourceRow)
    {
        // Line numbers are a 1-based index, but the rows collection is 0-based.
        var lineNumber = 0;
        while (sourceRow) {
            ++lineNumber;
            sourceRow = sourceRow.previousSibling;
        }

        return lineNumber;
    },

    revealLine: function(lineNumber)
    {
        var row = this.sourceRow(lineNumber);
        if (row)
            row.scrollIntoViewIfNeeded(true);
    },

    addBreakpoint: function(breakpoint)
    {
        this.breakpoints.push(breakpoint);
        breakpoint.addEventListener("enabled", this._breakpointEnableChanged, this);
        breakpoint.addEventListener("disabled", this._breakpointEnableChanged, this);
        this._addBreakpointToSource(breakpoint);
    },

    removeBreakpoint: function(breakpoint)
    {
        this.breakpoints.remove(breakpoint);
        breakpoint.removeEventListener("enabled", null, this);
        breakpoint.removeEventListener("disabled", null, this);
        this._removeBreakpointFromSource(breakpoint);
    },

    addMessage: function(msg)
    {
        this.messages.push(msg);
        this._addMessageToSource(msg);
    },

    clearMessages: function()
    {
        this.messages = [];

        if (!this.element.contentDocument)
            return;

        var bubbles = this.element.contentDocument.querySelectorAll(".webkit-html-message-bubble");
        if (!bubbles)
            return;

        for (var i = 0; i < bubbles.length; ++i) {
            var bubble = bubbles[i];
            bubble.parentNode.removeChild(bubble);
        }
    },

    _loaded: function()
    {
        WebInspector.addMainEventListeners(this.element.contentDocument);
        this.element.contentDocument.addEventListener("mousedown", this._documentMouseDown.bind(this), true);

        var headElement = this.element.contentDocument.getElementsByTagName("head")[0];
        if (!headElement) {
            headElement = this.element.contentDocument.createElement("head");
            this.element.contentDocument.documentElement.insertBefore(headElement, this.element.contentDocument.documentElement.firstChild);
        }

        var styleElement = this.element.contentDocument.createElement("style");
        headElement.appendChild(styleElement);

        // Add these style rules here since they are specific to the Inspector. They also behave oddly and not
        // all properties apply if added to view-source.css (becuase it is a user agent sheet.)
        var styleText = ".webkit-line-number { background-repeat: no-repeat; background-position: right 1px; }\n";
        styleText += ".webkit-breakpoint .webkit-line-number { color: white; background-image: -webkit-canvas(breakpoint); }\n";
        styleText += ".webkit-breakpoint-disabled .webkit-line-number { color: white; background-image: -webkit-canvas(breakpoint-disabled); }\n";
        styleText += ".webkit-execution-line .webkit-line-number { color: transparent; background-image: -webkit-canvas(program-counter); }\n";
        styleText += ".webkit-breakpoint.webkit-execution-line .webkit-line-number { color: transparent; background-image: -webkit-canvas(breakpoint-program-counter); }\n";
        styleText += ".webkit-breakpoint-disabled.webkit-execution-line .webkit-line-number { color: transparent; background-image: -webkit-canvas(breakpoint-disabled-program-counter); }\n";
        styleText += ".webkit-execution-line .webkit-line-content { background-color: rgb(171, 191, 254); outline: 1px solid rgb(64, 115, 244); }\n";

        styleElement.textContent = styleText;

        this._needsProgramCounterImage = true;
        this._needsBreakpointImages = true;

        this.element.contentWindow.Element.prototype.addStyleClass = Element.prototype.addStyleClass;
        this.element.contentWindow.Element.prototype.removeStyleClass = Element.prototype.removeStyleClass;
        this.element.contentWindow.Element.prototype.hasStyleClass = Element.prototype.hasStyleClass;
        this.element.contentWindow.Node.prototype.enclosingNodeOrSelfWithNodeName = Node.prototype.enclosingNodeOrSelfWithNodeName;

        this._addExistingMessagesToSource();
        this._addExistingBreakpointsToSource();
        this._updateExecutionLine();
    },

    _documentMouseDown: function(event)
    {
        if (!event.target.hasStyleClass("webkit-line-number"))
            return;

        var sourceRow = event.target.enclosingNodeOrSelfWithNodeName("tr");
        if (sourceRow._breakpointObject)
            sourceRow._breakpointObject.enabled = !sourceRow._breakpointObject.enabled;
        else if (this.addBreakpointDelegate)
            this.addBreakpointDelegate(this.lineNumberForSourceRow(sourceRow));
    },

    _breakpointEnableChanged: function(event)
    {
        var breakpoint = event.target;
        var sourceRow = this.sourceRow(breakpoint.line);
        if (!sourceRow)
            return;

        sourceRow.addStyleClass("webkit-breakpoint");

        if (breakpoint.enabled)
            sourceRow.removeStyleClass("webkit-breakpoint-disabled");
        else
            sourceRow.addStyleClass("webkit-breakpoint-disabled");
    },

    _updateExecutionLine: function(previousLine)
    {
        if (previousLine) {
            var sourceRow = this.sourceRow(previousLine);
            if (sourceRow)
                sourceRow.removeStyleClass("webkit-execution-line");
        }

        if (!this._executionLine)
            return;

        this._drawProgramCounterImageIfNeeded();

        var sourceRow = this.sourceRow(this._executionLine);
        if (sourceRow)
            sourceRow.addStyleClass("webkit-execution-line");
    },

    _addExistingBreakpointsToSource: function()
    {
        var length = this.breakpoints.length;
        for (var i = 0; i < length; ++i)
            this._addBreakpointToSource(this.breakpoints[i]);
    },

    _addBreakpointToSource: function(breakpoint)
    {
        var sourceRow = this.sourceRow(breakpoint.line);
        if (!sourceRow)
            return;

        this._drawBreakpointImagesIfNeeded();

        sourceRow._breakpointObject = breakpoint;

        sourceRow.addStyleClass("webkit-breakpoint");
        if (!breakpoint.enabled)
            sourceRow.addStyleClass("webkit-breakpoint-disabled");
    },

    _removeBreakpointFromSource: function(breakpoint)
    {
        var sourceRow = this.sourceRow(breakpoint.line);
        if (!sourceRow)
            return;

        delete sourceRow._breakpointObject;

        sourceRow.removeStyleClass("webkit-breakpoint");
        sourceRow.removeStyleClass("webkit-breakpoint-disabled");
    },

    _addExistingMessagesToSource: function()
    {
        var length = this.messages.length;
        for (var i = 0; i < length; ++i)
            this._addMessageToSource(this.messages[i]);
    },

    _addMessageToSource: function(msg)
    {
        var row = this.sourceRow(msg.line);
        if (!row)
            return;

        var cell = row.getElementsByTagName("td")[1];
        if (!cell)
            return;

        var errorDiv = cell.lastChild;
        if (!errorDiv || errorDiv.nodeName.toLowerCase() !== "div" || !errorDiv.hasStyleClass("webkit-html-message-bubble")) {
            errorDiv = this.element.contentDocument.createElement("div");
            errorDiv.className = "webkit-html-message-bubble";
            cell.appendChild(errorDiv);
        }

        var imageURL;
        switch (msg.level) {
            case WebInspector.ConsoleMessage.MessageLevel.Error:
                errorDiv.addStyleClass("webkit-html-error-message");
                imageURL = "Images/errorIcon.png";
                break;
            case WebInspector.ConsoleMessage.MessageLevel.Warning:
                errorDiv.addStyleClass("webkit-html-warning-message");
                imageURL = "Images/warningIcon.png";
                break;
        }

        var lineDiv = this.element.contentDocument.createElement("div");
        lineDiv.className = "webkit-html-message-line";
        errorDiv.appendChild(lineDiv);

        // Create the image element in the Inspector's document so we can use relative image URLs.
        var image = document.createElement("img");
        image.src = imageURL;
        image.className = "webkit-html-message-icon";

        // Adopt the image element since it wasn't created in element's contentDocument.
        image = this.element.contentDocument.adoptNode(image);
        lineDiv.appendChild(image);

        lineDiv.appendChild(this.element.contentDocument.createTextNode(msg.message));
    },

    _drawProgramCounterInContext: function(ctx, glow)
    {
        if (glow)
            ctx.save();

        ctx.beginPath();
        ctx.moveTo(17, 2);
        ctx.lineTo(19, 2);
        ctx.lineTo(19, 0);
        ctx.lineTo(21, 0);
        ctx.lineTo(26, 5.5);
        ctx.lineTo(21, 11);
        ctx.lineTo(19, 11);
        ctx.lineTo(19, 9);
        ctx.lineTo(17, 9);
        ctx.closePath();
        ctx.fillStyle = "rgb(142, 5, 4)";

        if (glow) {
            ctx.shadowBlur = 4;
            ctx.shadowColor = "rgb(255, 255, 255)";
            ctx.shadowOffsetX = -1;
            ctx.shadowOffsetY = 0;
        }

        ctx.fill();
        ctx.fill(); // Fill twice to get a good shadow and darker anti-aliased pixels.

        if (glow)
            ctx.restore();
    },

    _drawProgramCounterImageIfNeeded: function()
    {
        if (!this._needsProgramCounterImage || !this.element.contentDocument)
            return;

        var ctx = this.element.contentDocument.getCSSCanvasContext("2d", "program-counter", 26, 11);
        ctx.clearRect(0, 0, 26, 11);
        this._drawProgramCounterInContext(ctx, true);

        delete this._needsProgramCounterImage;
    },

    _drawBreakpointImagesIfNeeded: function()
    {
        if (!this._needsBreakpointImages || !this.element.contentDocument)
            return;

        function drawBreakpoint(ctx, disabled)
        {
            ctx.beginPath();
            ctx.moveTo(0, 2);
            ctx.lineTo(2, 0);
            ctx.lineTo(21, 0);
            ctx.lineTo(26, 5.5);
            ctx.lineTo(21, 11);
            ctx.lineTo(2, 11);
            ctx.lineTo(0, 9);
            ctx.closePath();
            ctx.fillStyle = "rgb(1, 142, 217)";
            ctx.strokeStyle = "rgb(0, 103, 205)";
            ctx.lineWidth = 3;
            ctx.fill();
            ctx.save();
            ctx.clip();
            ctx.stroke();
            ctx.restore();

            if (!disabled)
                return;

            ctx.save();
            ctx.globalCompositeOperation = "destination-out";
            ctx.fillStyle = "rgba(0, 0, 0, 0.5)";
            ctx.fillRect(0, 0, 26, 11);
            ctx.restore();
        }

        var ctx = this.element.contentDocument.getCSSCanvasContext("2d", "breakpoint", 26, 11);
        ctx.clearRect(0, 0, 26, 11);
        drawBreakpoint(ctx);

        var ctx = this.element.contentDocument.getCSSCanvasContext("2d", "breakpoint-program-counter", 26, 11);
        ctx.clearRect(0, 0, 26, 11);
        drawBreakpoint(ctx);
        ctx.clearRect(20, 0, 6, 11);
        this._drawProgramCounterInContext(ctx, true);

        var ctx = this.element.contentDocument.getCSSCanvasContext("2d", "breakpoint-disabled", 26, 11);
        ctx.clearRect(0, 0, 26, 11);
        drawBreakpoint(ctx, true);

        var ctx = this.element.contentDocument.getCSSCanvasContext("2d", "breakpoint-disabled-program-counter", 26, 11);
        ctx.clearRect(0, 0, 26, 11);
        drawBreakpoint(ctx, true);
        ctx.clearRect(20, 0, 6, 11);
        this._drawProgramCounterInContext(ctx, true);

        delete this._needsBreakpointImages;
    }
}