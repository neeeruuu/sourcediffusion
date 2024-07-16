#include "hooks.h"
#include "util/log.h"

HookManager* HookManager::_instance = nullptr;

HookManager* HookManager::get()
{
    if (!_instance)
        _instance = new HookManager();
    return _instance;
}

bool HookManager::applyAll()
{
    for (auto hookInfo : _hooks)
    {
        if (!hookInfo->apply())
        {
            Log::error("failed to apply {} hook(s)", hookInfo->getName());
            removeAll();
            return false;
        }
        Log::verbose("applied {} hook(s)", hookInfo->getName());
    }

    return true;
}

bool HookManager::removeAll()
{
    bool hadError = false;
    for (auto hookInfo : _hooks)
    {
        if (!hookInfo->remove())
            hadError = true;
    }

    return hadError;
}

void HookManager::registerHook(HookInfo* hook)
{
    std::lock_guard<std::mutex> lock(_mtx);
    _hooks.push_back(hook);
}

void HookManager::unregisterHook(HookInfo* hook)
{
    std::lock_guard<std::mutex> lock(_mtx);
    auto it = std::find(_hooks.begin(), _hooks.end(), hook);
    if (it == _hooks.end())
        return;
    _hooks.erase(it);
}

/*
    HookInfo implementation
*/
HookInfo::HookInfo(std::string name, tHookCallback apply, tHookCallback remove)
{
    _name = name;
    _apply = apply;
    _remove = remove;

    HookManager::get()->registerHook(this);
}

HookInfo::~HookInfo() { HookManager::get()->unregisterHook(this); }