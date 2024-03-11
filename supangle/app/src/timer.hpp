#include "./utils/v8_str.hpp"
#include <cstdlib>
#include <memory>
#include <string>
#include <uv.h>
#include <v8.h>
#include <vector>

using namespace std;
using namespace v8;

struct TimerWrap
{
    uv_timer_t uvTimer;
    string workerId;
    Isolate *isolate;
    Global<Function> callback;
    bool isInterval;
};

uv_loop_t *loop;
vector<unique_ptr<TimerWrap>> timers;

class Timer
{
public:
    static void Initialize(uv_loop_t *evloop) { loop = evloop; }

    static void Timeout(const FunctionCallbackInfo<Value> &args)
    {
        Isolate *isolate = args.GetIsolate();
        Local<Context> context = isolate->GetCurrentContext();

        if (!args[0]->IsFunction())
        {
            isolate->ThrowError("First argument is not a function.");
            return;
        }

        Local<Function> callback =
            Local<Function>::New(isolate, args[0].As<Function>());

        string workerId = to_string(rand());

        int delay = args[1]->IntegerValue(context).ToChecked();
        int repeat = args[2]->IntegerValue(context).ToChecked();

        auto timerWrap = make_unique<TimerWrap>();
        timerWrap->callback.Reset(isolate, callback.As<Function>());
        timerWrap->uvTimer.data = (void *)timerWrap.get();
        timerWrap->isolate = isolate;
        timerWrap->workerId = workerId;

        if (repeat > 0)
        {
            timerWrap->isInterval = true;
        }

        timers.push_back(move(timerWrap));

        uv_timer_init(loop, &timers.back()->uvTimer);

        if (uv_timer_start(&timers.back()->uvTimer, onTimerCallback, delay,
                           repeat) != 0)
        {
            isolate->ThrowError("libuv crapped itself.");
        }

        Local<Object> result = Object::New(isolate);

        if (!result->Set(context, v8_str("workerId"), v8_str(workerId.c_str()))
                 .FromJust())
        {
            isolate->ThrowError("Failed to set property on result object.");
            return;
        }

        args.GetReturnValue().Set(result);
    }

    static void
    StopTimerByWorkerId(const FunctionCallbackInfo<Value> &args)
    {
        Isolate *isolate = args.GetIsolate();
        Local<Context> context = isolate->GetCurrentContext();

        if (args[0]->IsObject())
        {
            Handle<Object> object = Handle<Object>::Cast(args[0]);

            Handle<Value> workerIdValue =
                object->Get(context, v8_str("workerId")).ToLocalChecked();

            Handle<String> workerIdHandle =
                Handle<String>::Cast(workerIdValue);
            String::Utf8Value workerId(isolate, workerIdHandle);
            string workerIdStr = *workerId;

            auto it = remove_if(
                timers.begin(), timers.end(),
                [workerIdStr](const unique_ptr<TimerWrap> &timerWrap)
                {
                    return timerWrap->workerId == workerIdStr;
                });

            if (it != timers.end())
            {
                uv_timer_stop(&((*it)->uvTimer));
                timers.erase(it, timers.end());
            }
            else
            {
                isolate->ThrowException(
                    v8_str("No Timer matches with the provided workerId."));
            }
        }
        else
        {
            isolate->ThrowException(
                v8_str("Not a valid object provided to stop the timer."));
        }
    }

    static void onTimerCallback(uv_timer_t *handle)
    {
        TimerWrap *timerWrap = static_cast<TimerWrap *>(handle->data);
        Isolate *isolate = timerWrap->isolate;

        // No need to check isolate liveness here

        Local<Value> result;
        Local<Function> callback =
            Local<Function>::New(isolate, timerWrap->callback);

        MaybeLocal<Value> maybeResult = callback->Call(
            isolate->GetCurrentContext(), Undefined(isolate), 0, nullptr);
        if (maybeResult.IsEmpty())
        {
            isolate->ThrowException(
                v8_str("Callback failed to convert to Local<Value>"));
        }

        if (timerWrap->isInterval != true)
        {

            // Cleanup after the callback
            auto it =
                remove_if(timers.begin(), timers.end(),
                          [timerWrap](const unique_ptr<TimerWrap> &wrap)
                          {
                              return wrap.get() == timerWrap;
                          });

            if (it != timers.end())
            {
                timers.erase(it, timers.end());
            }
        }
    }

    // Add a cleanup function to be called on program exit
    static void Cleanup()
    {
        for (auto &timerWrap : timers)
        {
            uv_timer_stop(&(timerWrap->uvTimer));
        }
        timers.clear();
    }
};
