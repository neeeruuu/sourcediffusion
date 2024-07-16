#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <vector>

template <typename... T> class CallbackEvent
{
    private:
        class Callback
        {
            public:
                bool isActive;

                void call(T... args)
                {
                    if (isActive)
                        _fn(std::forward<T>(args)...);
                }
                Callback() = delete;
                Callback(std::function<void(T...)> fn, bool active)
                {
                    _fn = fn;
                    isActive = active;
                }

            private:
                std::function<void(T...)> _fn;
        };

    public:
        inline void run(T... args)
        {
            std::lock_guard<std::mutex> lock(_mtx);
            for (auto callback : _callbacks)
                callback->call(std::forward<T>(args)...);
        }
        inline std::shared_ptr<Callback> addListener(std::function<void(T...)> fn, bool active = true)
        {
            std::lock_guard<std::mutex> lock(_mtx);
            auto cb = std::make_shared<Callback>(std::move(fn), active);
            _callbacks.push_back(cb);
            return cb;
        }

    private:
        std::vector<std::shared_ptr<Callback>> _callbacks;
        std::mutex _mtx;
};