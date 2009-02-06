#ifndef WebScriptElement_h
#define WebScriptElement_h

#include "webkit.h"
#include "ScriptEvaluator.h"

namespace WebCore {
	class String;
	class ScriptSourceCode;
}

class WebKitWebScriptEvaluator;

class EvaluatorAdapter : public WebCore::ScriptEvaluator {
public:
	EvaluatorAdapter(WebKitWebScriptEvaluator *evaluator);

	virtual bool matchesMimeType(const WebCore::String &mimeType);

	virtual void evaluate(const WebCore::String &mimeType, const WebCore::ScriptSourceCode& sourceCode);

protected:
	WebKitWebScriptEvaluator *evaluator;
};

WEBKIT_API void addScriptEvaluator(WebKitWebScriptEvaluator *evaluator);

#endif
