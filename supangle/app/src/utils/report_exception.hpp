#include "v8.h"
#include "../globals.h"

using namespace v8;

// Copied from https://github.com/v8/v8/blob/master/samples/shell.cc
static void ReportException(Isolate* isolate, TryCatch* try_catch) {
        HandleScope handle_scope(isolate);
        String::Utf8Value exception(isolate, try_catch->Exception());
        const char* exception_string = ToCString(exception);
        Local<Message> message = try_catch->Message();

        if (message.IsEmpty()) {
            // V8 didn't provide any extra information about this error; just
            // print the exception.
            fprintf(stderr, "%s\n", exception_string);
        } else {
            // Print (filename):(line number): (message).
            String::Utf8Value filename(isolate, message->GetScriptOrigin().ResourceName());
            Local<Context> context(isolate->GetCurrentContext());
            const char* filename_string = ToCString(filename);
            int linenum = message->GetLineNumber(context).FromJust() - decreaseLineValueBy;
            // Print line of source code.

            String::Utf8Value sourceline(isolate, message->GetSourceLine(context).ToLocalChecked());
            const char* sourceline_string = ToCString(sourceline);
            fprintf(stderr, "%s\n", sourceline_string);
            // Print wavy underline (GetUnderline is deprecated).
            int start = message->GetStartColumn(context).FromJust();

            for (int i = 0; i < start; i++) {
                fprintf(stderr, " ");
            }

            int end = message->GetEndColumn(context).FromJust();

            for (int i = start; i < end; i++) {
                fprintf(stderr, "^");
            }

            fprintf(stderr, "\n");
            fprintf(stderr, "%s\n", exception_string);
            fprintf(stderr, "    at %s:%i\n", filename_string, linenum);

        }
}