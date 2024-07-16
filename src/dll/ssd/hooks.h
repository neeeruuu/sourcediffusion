#pragma once

#include <mutex>
#include <string>
#include <vector>

#define HOOK(inlineInstance, addr, dest)                                                                               \
    if (!(addr))                                                                                                         \
        return false;                                                                                                  \
    inlineInstance = safetyhook::create_inline(addr, dest);                                                             \
    if (!inlineInstance.enable())                                                                                      \
        return false;

#define HOOK_INFO(name, apply, remove) HookInfo* _##name##Hook = new HookInfo(#name, apply, remove);

typedef bool (*tHookCallback)();

class HookInfo;

class HookManager
{
        friend HookInfo;

    public:
        static HookManager* get();

        bool applyAll();
        bool removeAll();

    protected:
        void registerHook(HookInfo* hook);
        void unregisterHook(HookInfo* hook);

    private:
        std::vector<HookInfo*> _hooks;
        static HookManager* _instance;
        mutable std::mutex _mtx;
};

class HookInfo
{
        friend HookManager;

    public:
        HookInfo(std::string name, tHookCallback apply, tHookCallback remove);
        ~HookInfo();

    protected:
        __forceinline bool apply() const { return _apply(); }
        __forceinline bool remove() const { return _remove(); }
        __forceinline const std::string getName() const { return _name; }

    private:
        HookInfo() = delete;

        std::string _name;
        tHookCallback _apply;
        tHookCallback _remove;
};