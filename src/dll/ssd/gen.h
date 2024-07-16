#pragma once

// from: minwindef.h
#define MAX_PATH 260

struct GeneratorConfig
{
        bool enabled = false;
        bool drawSource = true;

        char mdlPath[MAX_PATH];
        char vaePath[MAX_PATH];
        char taesdPath[MAX_PATH];
        char cnetPath[MAX_PATH];
        char loraPath[MAX_PATH];

        char positivePrompt[1024];
        char negativePrompt[1024];

        float cfg = 2.f;
        int steps = 4;
        __int64 seed = -1;
        float strength = .5f;

        int sampler = 7;
        int rng = 1;
        int schedule = 2;
        int type = 1;

        bool overrideResolution = true;
        int width = 512;
        int height = 512;
};

enum GeneratorState
{
    GENSTATE_INIT,
    GENSTATE_LOADING,
    GENSTATE_IDLE,
    GENSTATE_GENERATING
};
inline const char* genStateStr[] = {"Init (Awaiting model)", "Loading model", "Idle", "Generating"};

namespace generator
{
    GeneratorConfig* getConfig();
    GeneratorState getState();

    void loadModelWithSettings();

    void loadConfig();
    void saveConfig();
}