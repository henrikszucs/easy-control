#pragma once
#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <napi.h>
#include <vector>

class InstallDriver : public Napi::AsyncWorker {
    public:
        InstallDriver(const Napi::Env& env);
        Napi::Promise GetPromise();

    protected:
        void Execute();
        void OnOK();
        void OnError(const Napi::Error& e);

    private:
        Napi::Promise::Deferred m_deferred;
        bool m_result;

};
    

class Controller : public Napi::ObjectWrap<Controller> {
    public:
        static Napi::Object Init(Napi::Env env, Napi::Object exports);
        static Napi::Boolean isSupported(const Napi::CallbackInfo& info);
        static Napi::Promise install(const Napi::CallbackInfo& info);
        static Napi::Array list(const Napi::CallbackInfo& info);
        static Napi::Object create(const Napi::CallbackInfo& info);
        static Napi::Object NewInstance(Napi::Env env);
        Controller(const Napi::CallbackInfo& info);
        ~Controller();

    private:
        static std::vector<Napi::ObjectReference>& controllers();
        Napi::Value isActive(const Napi::CallbackInfo& info);
        void buttonDown(const Napi::CallbackInfo& info);
        void buttonUp(const Napi::CallbackInfo& info);
        void setAxis(const Napi::CallbackInfo& info);
        void disconnect(const Napi::CallbackInfo& info);
        int m_id;
        bool m_active;

    
};

#endif