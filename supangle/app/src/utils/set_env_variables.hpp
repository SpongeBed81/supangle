extern char **environ;

static void SetEnviromentVariables(v8::Isolate* isolate, const v8::Local<v8::ObjectTemplate> &templated)
{
    char **s = environ;

    for (int i = 0; environ[i] != nullptr; ++i) {
        std::string envVar(environ[i]);

        size_t equalPos = envVar.find('=');

        if (equalPos != std::string::npos) {
            std::string name = envVar.substr(0, equalPos);
            std::string value = envVar.substr(equalPos + 1);

            templated->Set(isolate, name.c_str(), v8_str(value.c_str()));
        }
    }
}