var wmlNS = "http://www.wapforum.org/DTD/wml_1.1.xml";
var xhtmlNS = "http://www.w3.org/1999/xhtml";

var testDocument;
var iframeElement;
var bodyElement;

function createWMLElement(name) {
    return testDocument.createElementNS(wmlNS, "wml:" + name);
}

function createWMLTestCase(desc, substituteVariables) {
    if (substituteVariables == null)
        substituteVariables = true;

    description(desc);
    bodyElement = document.getElementsByTagName("body")[0];

    // Clear variable state & history
    document.resetWMLPageState();

    if (window.layoutTestController) {
        layoutTestController.dumpChildFramesAsText();
        layoutTestController.waitUntilDone();
    }

    iframeElement = document.createElementNS(xhtmlNS, "iframe");
    iframeElement.src = "resources/test-document.wml";

    var loaded = false;
    var executed = false;

    iframeElement.onload = function() {
        if (executed)
            return;

        // External deck jumps
        if (testDocument != null && !substituteVariables) {
            executeTest();
            return;
        }

        testDocument = iframeElement.contentDocument;
        setupTestDocument();

        // Variable refresh
        if (loaded && substituteVariables) {
            executeTest();
            return;
        }

        loaded = true;
        prepareTest();

        // Internal deck jumps
        if (!substituteVariables) {
            executed = true;
            executeTest();
        }
    }

    bodyElement.insertBefore(iframeElement, document.getElementById("description"));
}

function triggerUpdate(x, y) {
    // Translation due to HTML content above the WML document in the iframe
    x = x + iframeElement.offsetLeft;
    y = y + iframeElement.offsetTop;

    if (window.eventSender) {
        eventSender.mouseMoveTo(x, y);
        eventSender.mouseDown();
        eventSender.mouseUp();
    }
}

function startTest(x, y) {
    // Assure first layout finished
    window.setTimeout("triggerUpdate(" + x + ", " + y + ")", 0);
}

function completeTest() {
    var script = document.createElementNS(xhtmlNS, "script");

    script.onload = function() {
        if (window.layoutTestController)
            window.setTimeout("layoutTestController.notifyDone()", 0);
    };

    script.src = "../fast/js/resources/js-test-post.js";
    successfullyParsed = true;

    bodyElement.appendChild(script);
}
