#include "config.h"
#include "StringHash.h"
#include <wtf/Vector.h>
#include "webkitwebscriptelement.h"
#include "webkitwebscriptevaluator.h"
#include "ScriptSourceCode.h"
#include "ScriptEvaluator.h"
#include "ScriptElement.h"
#include "CString.h"

EvaluatorAdapter::EvaluatorAdapter(WebKitWebScriptEvaluator *evaluator)
	: evaluator(evaluator)
{

}

bool EvaluatorAdapter::matchesMimeType(const WebCore::String &mimeType) {
	return evaluator->matchesMimeType(mimeType.utf8().data());
}

void EvaluatorAdapter::evaluate(const WebCore::String &mimeType, const WebCore::ScriptSourceCode& sourceCode, void *context)
{
	evaluator->evaluate(mimeType.utf8().data(), sourceCode.jsSourceCode().toString().ascii());
}

WEBKIT_API void addScriptEvaluator(WebKitWebScriptEvaluator *evaluator) {
	WebCore::ScriptElement::addScriptEvaluator(new EvaluatorAdapter(evaluator));
}
