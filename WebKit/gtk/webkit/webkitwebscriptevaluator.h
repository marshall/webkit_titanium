#ifndef WebScriptEvaluator_h
#define WebScriptEvaluator_h

class WebKitWebScriptEvaluator
{
public:
	virtual bool matchesMimeType(const gchar * mimeType) = 0;
	virtual void evaluate(const gchar *mimeType, const gchar *sourceCode) = 0;
};

#endif
