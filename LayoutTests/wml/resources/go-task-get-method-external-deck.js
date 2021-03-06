// [Name] go-task-get-method-external-deck.js

createWMLTestCase("Tests GET method of <go> element - jump to external deck", false);

function setupTestDocument() {
    var cardElement = testDocument.documentElement.firstChild;

    var anchorElement = createWMLElement("anchor");
    anchorElement.appendChild(testDocument.createTextNode("Start test"));
    cardElement.appendChild(anchorElement);

    var goElement = createWMLElement("go");
    goElement.setAttribute("href", "external-deck.wml#externalCard1");
    anchorElement.appendChild(goElement);
}

function prepareTest() {
    startTest(25, 15);
}

function executeTest() {
    completeTest();
}

var successfullyParsed = true;
