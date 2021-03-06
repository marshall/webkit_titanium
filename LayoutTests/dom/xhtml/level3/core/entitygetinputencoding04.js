
/*
Copyright Â© 2001-2004 World Wide Web Consortium, 
(Massachusetts Institute of Technology, European Research Consortium 
for Informatics and Mathematics, Keio University). All 
Rights Reserved. This work is distributed under the W3CÂ® Software License [1] in the 
hope that it will be useful, but WITHOUT ANY WARRANTY; without even 
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 

[1] http://www.w3.org/Consortium/Legal/2002/copyright-software-20021231
*/



   /**
    *  Gets URI that identifies the test.
    *  @return uri identifier of test
    */
function getTargetURI() {
      return "http://www.w3.org/2001/DOM-Test-Suite/level3/core/entitygetinputencoding04";
   }

var docsLoaded = -1000000;
var builder = null;

//
//   This function is called by the testing framework before
//      running the test suite.
//
//   If there are no configuration exceptions, asynchronous
//        document loading is started.  Otherwise, the status
//        is set to complete and the exception is immediately
//        raised when entering the body of the test.
//
function setUpPage() {
   setUpPageStatus = 'running';
   try {
     //
     //   creates test document builder, may throw exception
     //
     builder = createConfiguredBuilder();
       setImplementationAttribute("validating", true);

      docsLoaded = 0;
      
      var docRef = null;
      if (typeof(this.doc) != 'undefined') {
        docRef = this.doc;
      }
      docsLoaded += preload(docRef, "doc", "external_barfoo");
        
       if (docsLoaded == 1) {
          setUpPage = 'complete';
       }
    } catch(ex) {
    	catchInitializationError(builder, ex);
        setUpPage = 'complete';
    }
}



//
//   This method is called on the completion of 
//      each asychronous load started in setUpTests.
//
//   When every synchronous loaded document has completed,
//      the page status is changed which allows the
//      body of the test to be executed.
function loadComplete() {
    if (++docsLoaded == 1) {
        setUpPageStatus = 'complete';
    }
}


/**
* 
Check the value of Entity.inputEncoding on an UTF-8 external entity 
is UTF-8.

* @author IBM
* @author Neil Delima
* @author Curt Arnold
* @see http://www.w3.org/TR/2004/REC-DOM-Level-3-Core-20040407/core#Entity3-inputEncoding
* @see http://lists.w3.org/Archives/Public/www-dom-ts/2003Dec/0045.html
*/
function entitygetinputencoding04() {
   var success;
    if(checkInitialization(builder, "entitygetinputencoding04") != null) return;
    var doc;
      var docType;
      var entitiesMap;
      var entity;
      var encodingName;
      
      var docRef = null;
      if (typeof(this.doc) != 'undefined') {
        docRef = this.doc;
      }
      doc = load(docRef, "doc", "external_barfoo");
      docType = doc.doctype;

      entitiesMap = docType.entities;

      entity = entitiesMap.getNamedItem("ent2");
      encodingName = entity.inputEncoding;

      assertEquals("entityIsUTF8","UTF-8".toLowerCase(),encodingName.toLowerCase());
       encodingName = doc.inputEncoding;

      assertEquals("documentIsUTF8","UTF-8".toLowerCase(),encodingName.toLowerCase());
       
}




function runTest() {
   entitygetinputencoding04();
}
