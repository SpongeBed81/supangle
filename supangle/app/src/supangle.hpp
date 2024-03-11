#include <libplatform/libplatform.h>
#include <uv.h>
#include "v8.h"
#include <string>
#include <iostream>
#include <vector>
#include <cassert>

#include "./fs.hpp"
#include "./timer.hpp"

#include "./utils/to_c_string.hpp"
#include "./utils/report_exception.hpp"
#include "./utils/set_env_variables.hpp"
#include "globals.h"

using namespace std;
using namespace v8;

int decreaseLineValueBy = 0;

int getLineCount(const string &str)
{
    int lineCount = 1; // At least one line if the string is not empty

    for (char c : str)
    {
        if (c == '\n')
        {
            lineCount++;
        }
    }

    return lineCount;
}

uv_loop_t *DEFAULT_LOOP = uv_default_loop();

Timer timer;

class Supangle
{
private:
    Isolate *isolate;
    Local<Context> context;
    unique_ptr<Platform> *platform;
    Isolate::CreateParams create_params;

    void WaitForEvents()
    {
        uv_run(DEFAULT_LOOP, UV_RUN_DEFAULT);
    }

    static bool ExecuteString(Isolate *isolate, Local<String> source, Local<Value> name, bool print_result, bool report_exceptions)
    {

        HandleScope handle_scope(isolate);
        TryCatch try_catch(isolate);
        ScriptOrigin origin(isolate, name);
        Local<Context> context(isolate->GetCurrentContext());
        Local<Script> script;

        if (!Script::Compile(context, source, &origin).ToLocal(&script))
        {
            if (report_exceptions)
            {
                ReportException(isolate, &try_catch);
            }
            return false;
        }
        else
        {
            Local<Value> result;
            if (!script->Run(context).ToLocal(&result))
            {
                assert(try_catch.HasCaught());

                if (report_exceptions)
                {
                    ReportException(isolate, &try_catch);
                }

                return false;
            }
            else
            {
                assert(!try_catch.HasCaught());
                if (print_result && !result->IsUndefined())
                {
                    String::Utf8Value str(isolate, result);
                    const char *cstr = ToCString(str);
                    printf("%s\n", cstr);
                }
                return true;
            }
        }
    }

    static void Require(const FunctionCallbackInfo<Value> &info)
    {
        for (int i = 0; i < info.Length(); i++)
        {
            HandleScope handle_scope(info.GetIsolate());
            String::Utf8Value file(info.GetIsolate(), info[i]);
            if (*file == nullptr)
            {
                info.GetIsolate()->ThrowError("Error loading file");
                return;
            }

            // to convert String::Utf8Value to const char*
            string fileContent = Fs::readFile(*file);

            Local<String> source = String::NewFromUtf8(info.GetIsolate(), fileContent.c_str(), NewStringType::kNormal).ToLocalChecked();

            if (!ExecuteString(info.GetIsolate(), source, info[i], false, true))
            {
                info.GetIsolate()->ThrowError("Error executing file");
                return;
            }
        }
    }

    void ExecuteScriptAndWaitForEvents(char *filename)
    {
        Context::Scope context_scope(this->context);
        {

            string fileContent = Fs::readFile(filename);

            vector<string> files = Fs::getFilesInDirectory("interfaces");

            string interfacesLookup = "";

            for (const auto &file : files)
            {
                string interfaceContent = Fs::readFile(file.c_str());

                interfacesLookup += interfaceContent + "\n\n";
            }

            decreaseLineValueBy = getLineCount(interfacesLookup);

            string compileThis = interfacesLookup + fileContent;

            Local<String> source = v8_str(compileThis.c_str());

            ExecuteString(isolate, source, v8_str(filename), true, true);

            WaitForEvents();
        }
    }

    static void Print(const FunctionCallbackInfo<Value> &args)
    {
        String::Utf8Value str(args.GetIsolate(), args[0]);
        printf("%s", *str);
        fflush(stdout);
    }

public:
    unique_ptr<Platform> initializeV8(int argc, char *argv[])
    {
        unique_ptr<Platform> platform = platform::NewDefaultPlatform();
        V8::InitializePlatform(platform.get());
        V8::Initialize();

        this->platform = &platform;
        return platform;
    }

    void initializeVM()
    {
        // Create a new Isolate and make it the current one.
        Isolate::CreateParams create_params;
        create_params.array_buffer_allocator =
            ArrayBuffer::Allocator::NewDefaultAllocator();
        this->isolate = Isolate::New(create_params);
        this->create_params = create_params;
    }

    void InitializeProgram(char *filename)
    {

        // I definitely need to clear these lines somehow...

        Isolate::Scope isolate_scope(this->isolate);

        // Create a stack-allocated handle scope.
        HandleScope handle_scope(this->isolate);

        // Create a template for the global object.
        Local<ObjectTemplate> global = ObjectTemplate::New(isolate);

        global->Set(isolate, "require", FunctionTemplate::New(isolate, Require));

        // console template
        Local<ObjectTemplate> console = ObjectTemplate::New(isolate);

        global->Set(isolate, "console", console);

        // process template
        Local<ObjectTemplate> process = ObjectTemplate::New(isolate);

        Local<ObjectTemplate> process_stdout = ObjectTemplate::New(isolate);

        Local<ObjectTemplate> process_env = ObjectTemplate::New(isolate);

        SetEnviromentVariables(isolate, process_env);

        process_stdout->Set(isolate, "write", FunctionTemplate::New(isolate, Print));

        process->Set(isolate, "stdout", process_stdout);
        process->Set(isolate, "env", process_env);
        process->Set(isolate, "timeout", FunctionTemplate::New(isolate, timer.Timeout));

        global->Set(isolate, "process", process);

        // timeout
        timer.Initialize(DEFAULT_LOOP);

        // currently both can be used to clear timeout and interval ¯\_(ツ)_/¯
        global->Set(isolate, "clearTimeout", FunctionTemplate::New(isolate, timer.StopTimerByWorkerId));
        global->Set(isolate, "clearInterval", FunctionTemplate::New(isolate, timer.StopTimerByWorkerId));

        this->context = Context::New(this->isolate, NULL, global);

        ExecuteScriptAndWaitForEvents(filename);
    }

    void Shutdown()
    {
        this->isolate->Dispose();
        V8::Dispose();
        V8::DisposePlatform();
        Timer::Cleanup();
        delete this->create_params.array_buffer_allocator;
    }
};
