#ifndef WebScriptElement_h
#define WebScriptElement_h

#include "WebKit.h"
#include <WebCore/ScriptEvaluator.h>
#include <WebCore/ScriptElement.h>
#include <WebCore/ScriptSourceCode.h>
#include <WebCore/BString.h>

class EvaluatorAdapter : public WebCore::ScriptEvaluator {
public:
	EvaluatorAdapter(IWebScriptEvaluator *evaluator) : evaluator(evaluator) {
	}

	virtual bool matchesMimeType(const WebCore::String &mimeType) {
		BOOL returnVal;

		HRESULT hr = evaluator->matchesMimeType(WebCore::BString(mimeType), &returnVal);
		if (SUCCEEDED(hr)) {
			return returnVal;
		}
		return false;
	}

	virtual void evaluate(const WebCore::String &mimeType, const WebCore::ScriptSourceCode& sourceCode) {
		evaluator->evaluate(WebCore::BString(mimeType), WebCore::BString(sourceCode.jsSourceCode().data()));
	}

protected:
	IWebScriptEvaluator *evaluator;
};

void STDMETHODCALLTYPE addScriptEvaluator(IWebScriptEvaluator *evaluator);

#endif