#include "bind.h"

/*
    Bind implementation
*/
Bind::~Bind() { _controller->unregisterBind(this); }

void Bind::onKeyUp()
{
    switch (_type)
    {
        case BIND_BOOL:
            *reinterpret_cast<bool*>(_data) = false;
    }
}

void Bind::onKeyDown()
{
    switch (_type)
    {
        case BIND_CALLBACK:
            reinterpret_cast<tCallbackFunction>(_data)();
            break;
        case BIND_INCREASE:
            (*reinterpret_cast<int*>(_data))++;
            break;
        case BIND_DECREASE:
            (*reinterpret_cast<int*>(_data))--;
            break;
        case BIND_TOGGLE:
            *reinterpret_cast<bool*>(_data) = !(*reinterpret_cast<bool*>(_data));
            break;
        case BIND_BOOL:
            *reinterpret_cast<bool*>(_data) = true;
    }
}

void Bind::initialize(BindType type, int keyCode, void* data, bool active, BindController* controller)
{
    _type = type;
    key = keyCode;
    _data = data;
    isActive = active;
    _controller = controller ? controller : BindController::instance();
    _controller->registerBind(this);
}

/*
    BindController implementation
*/
BindController* BindController::instance()
{
    if (!_instance)
        _instance = new BindController();
    return _instance;
}

void BindController::onKey(unsigned __int64 key, bool isDown)
{
    runDeletionQueue();
    for (auto bind : _binds)
    {
        if (!bind->isActive)
            continue;

        if (bind->_type == BIND_READER)
            reinterpret_cast<tInputReader>(bind->_data)(bind, key, isDown);
        else if (key == bind->key)
            isDown ? bind->onKeyDown() : bind->onKeyUp();
    }
}

void BindController::runDeletionQueue()
{
    std::lock_guard<std::mutex> lock(_mtx);
    if (_deletionQueue.empty())
        return;

    for (auto bind : _deletionQueue)
    {
        auto iter = std::find(_binds.begin(), _binds.end(), bind);
        if (iter != _binds.end())
            _binds.erase(iter);
    }
    _deletionQueue.clear();
}

void BindController::registerBind(Bind* bind) { _binds.push_back(bind); }

void BindController::unregisterBind(Bind* bind)
{
    std::lock_guard<std::mutex> lock(_mtx);

    bind->isActive = false;
    _deletionQueue.push_back(bind);
}
