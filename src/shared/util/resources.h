#pragma once

class Resource
{
    public:
        Resource(void* module, const char* name, const char* type);
        ~Resource();

        unsigned long size() const { return _size; }
        void* data() { return _buff; }

    private:
        Resource() = delete;

        unsigned long _size;
        void* _buff;
};