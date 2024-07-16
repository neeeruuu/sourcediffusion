#pragma once

#include <mutex>
#include <vector>

#include "keycodes.h"

class Bind;
class BindController;

typedef void (*tCallbackFunction)(void);
typedef void (*tInputReader)(Bind* bind, unsigned __int64 keyCode, bool down);

enum BindType : unsigned char
{
    BIND_CALLBACK,
    BIND_INCREASE,
    BIND_DECREASE,
    BIND_TOGGLE,
    BIND_BOOL,
    BIND_READER
};

class Bind
{
    public:
        inline Bind(int keyCode, tCallbackFunction callback, bool active = true, BindController* controller = nullptr)
        {
            initialize(BIND_CALLBACK, keyCode, callback, active, controller);
        }

        inline Bind(BindType type, int keyCode, bool* value, bool active = true, BindController* controller = nullptr)
        {
            if (type != BIND_BOOL && type != BIND_TOGGLE)
                throw;
            initialize(type, keyCode, value, active, controller);
        }

        inline Bind(BindType type, int keyCode, int* value, bool active = true, BindController* controller = nullptr)
        {
            if (type != BIND_INCREASE && type != BIND_DECREASE)
                throw;
            initialize(type, keyCode, value, active, controller);
        }

        ~Bind();

        bool isActive;
        unsigned int key;

        void onKeyUp();
        void onKeyDown();

    protected:
        void initialize(BindType type, int keyCode, void* data, bool active, BindController* controller);

        Bind() = delete;

        void* _data;
        BindType _type;
        BindController* _controller;

        friend class BindController;
};

class BindController
{
    public:
        static BindController* instance();

        void onKey(unsigned __int64 key, bool isDown);

    private:
        BindController(){};
        ~BindController(){};

        void runDeletionQueue();

        std::vector<Bind*> _binds{};
        std::vector<Bind*> _deletionQueue{};

        std::mutex _mtx;

        inline static BindController* _instance;

    protected:
        friend class Bind;
        void registerBind(Bind* bind);
        void unregisterBind(Bind* bind);
};