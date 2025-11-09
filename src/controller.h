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

class EnableGamepad : public Napi::AsyncWorker {
    public:
        EnableGamepad(const Napi::Env& env, uint32_t gamepadId, uint32_t maxButton, uint32_t maxAxis);
        Napi::Promise GetPromise();

    protected:
        void Execute();
        void OnOK();
        void OnError(const Napi::Error& e);

    private:
        Napi::Promise::Deferred m_deferred;
        uint32_t m_gamepadId;
        uint32_t m_maxButton;
        uint32_t m_maxAxis;
        bool m_result;

};

class DisableGamepad : public Napi::AsyncWorker {
    public:
        DisableGamepad(const Napi::Env& env, uint32_t gamepadId);
        Napi::Promise GetPromise();

    protected:
        void Execute();
        void OnOK();
        void OnError(const Napi::Error& e);

    private:
        Napi::Promise::Deferred m_deferred;
        uint32_t m_gamepadId;
        bool m_result;
};

class Controller : public Napi::ObjectWrap<Controller> {
    public:
        static Napi::Object Init(Napi::Env env, Napi::Object exports);

        static Napi::Boolean isSupported(const Napi::CallbackInfo& info);
        static Napi::Promise install(const Napi::CallbackInfo& info);
        static Napi::Array list(const Napi::CallbackInfo& info);
        static Napi::Value enable(const Napi::CallbackInfo& info);
        static Napi::Value disable(const Napi::CallbackInfo& info);
        static void buttonDown(const Napi::CallbackInfo& info);
        static void buttonUp(const Napi::CallbackInfo& info);
        static void setAxis(const Napi::CallbackInfo& info);
};

#endif