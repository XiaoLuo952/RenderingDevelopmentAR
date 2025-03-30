// Copyright (c) 2017-2022, The Khronos Group Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "pch.h"//预编译头文件
#include "common.h"
#include "options.h"//配置选项（图形后端选择）
#include "platformdata.h"
#include "platformplugin.h"
#include "graphicsplugin.h"//图形API抽象层
#include "openxr_program.h"//openxr程序主逻辑
#include "demos/utils.h"


namespace {

void ShowHelp() {
    Log::Write(Log::Level::Info, "adb shell setprop debug.xr.graphicsPlugin OpenGLES|Vulkan");
    Log::Write(Log::Level::Info, "adb shell setprop debug.xr.formFactor Hmd|Handheld");
    Log::Write(Log::Level::Info, "adb shell setprop debug.xr.viewConfiguration Stereo|Mono");
    Log::Write(Log::Level::Info, "adb shell setprop debug.xr.blendMode Opaque|Additive|AlphaBlend");
    Log::Write(Log::Level::Info, "adb shell setprop persist.log.tag V");
}

bool UpdateOptionsFromSystemProperties(Options& options) {
    char value[PROP_VALUE_MAX] = {};
    if (__system_property_get("debug.xr.graphicsPlugin", value) != 0) {
        options.GraphicsPlugin = value;
    }

    // Check for required parameters.
    if (options.GraphicsPlugin.empty()) {
        Log::Write(Log::Level::Warning, __FILE__, __LINE__, "GraphicsPlugin Default OpenGLES");
        options.GraphicsPlugin = "OpenGLES";
    }
    return true;
}
}  // namespace


struct AndroidAppState {
    ANativeWindow* NativeWindow = nullptr;//用于渲染的Surface窗口
    bool Resumed = false;//应用是否在前台（Resumed-恢复）
    std::shared_ptr<IOpenXrProgram> program;
};

/**
 * Process the next main command.
 */
static void app_handle_cmd(struct android_app* app, int32_t cmd) {
    AndroidAppState* appState = (AndroidAppState*)app->userData;

    //根据cmd的不同来选择不同的相应模式
    switch (cmd) {
        // There is no APP_CMD_CREATE. The ANativeActivity creates the
        // application thread from onCreate(). The application thread
        // then calls android_main().无需创建命令的意思大概
        case APP_CMD_START: {
            Log::Write(Log::Level::Info, __FILE__, __LINE__, "onStart()");
            break;
        }
        //控制渲染的启动和停止
        case APP_CMD_RESUME: {
            Log::Write(Log::Level::Info, __FILE__, __LINE__, "onResume()");
            appState->Resumed = true;
            if (appState->program.get()) {
            }
            break;
        }
        case APP_CMD_PAUSE: {
            Log::Write(Log::Level::Info, __FILE__, __LINE__, "onPause()");
            appState->Resumed = false;
            if (appState->program.get()) {
            }
            break;
        }
        case APP_CMD_STOP: {
            Log::Write(Log::Level::Info, __FILE__, __LINE__, "onStop()");
            break;
        }
        case APP_CMD_DESTROY: {
            Log::Write(Log::Level::Info, __FILE__, __LINE__, "onDestroy()");
            appState->NativeWindow = NULL;
            break;
        }
        //获取窗口来渲染
        case APP_CMD_INIT_WINDOW: {
            Log::Write(Log::Level::Info, __FILE__, __LINE__, "surfaceCreated()");
            appState->NativeWindow = app->window;
            break;
        }
        case APP_CMD_TERM_WINDOW: {
            Log::Write(Log::Level::Info, __FILE__, __LINE__, "surfaceDestroyed()");
            appState->NativeWindow = NULL;
            break;
        }
    }
}
//处理输入事件
static int32_t onInputEvent(struct android_app* app, AInputEvent* event){
    int type = AInputEvent_getType(event);
    if(type == AINPUT_EVENT_TYPE_KEY){
        int32_t action = AKeyEvent_getAction(event);
        int32_t code   = AKeyEvent_getKeyCode(event);
        Log::Write(Log::Level::Info, __FILE__, __LINE__, Fmt("onInputEvent:%d %d\n", code, action));
    }//根据输入事件的不同进行不同的响应
    return 0;
}

static void killProcess() {
    pid_t pid = getpid();
    kill(pid, SIGKILL);
}

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(struct android_app* app) {
    app_dummy();
    try {
        JNIEnv* Env;
        app->activity->vm->AttachCurrentThread(&Env, nullptr);

        setJNIEnv(Env);

        AndroidAppState appState = {};

        app->userData = &appState;
        app->onAppCmd = app_handle_cmd;
        app->onInputEvent = onInputEvent;

        //初始化应用配置
        std::shared_ptr<Options> options = std::make_shared<Options>();
        if (!UpdateOptionsFromSystemProperties(*options)) {
            return;
        }

        //保存Android上下文供平台插件使用
        std::shared_ptr<PlatformData> data = std::make_shared<PlatformData>();
        data->applicationVM = app->activity->vm;
        data->applicationActivity = app->activity->clazz;

        bool requestRestart = false;
        bool exitRenderLoop = false;

        // Create platform-specific implementation.创建平台插件处理Android特定逻辑
        std::shared_ptr<IPlatformPlugin> platformPlugin = CreatePlatformPlugin(options, data);
        // Create graphics API implementation.创建图形逻辑（OpenGL ES/Vulkan抽象层）
        std::shared_ptr<IGraphicsPlugin> graphicsPlugin = CreateGraphicsPlugin(options, platformPlugin);

        // Initialize the OpenXR program.初始化OpenXR主程序
        std::shared_ptr<IOpenXrProgram> program = CreateOpenXrProgram(options, platformPlugin, graphicsPlugin);

        appState.program = program;

        // Initialize the loader for this platform
        PFN_xrInitializeLoaderKHR initializeLoader = nullptr;
        if (XR_SUCCEEDED(xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR", (PFN_xrVoidFunction*)(&initializeLoader)))) {
            XrLoaderInitInfoAndroidKHR loaderInitInfoAndroid;
            memset(&loaderInitInfoAndroid, 0, sizeof(loaderInitInfoAndroid));
            loaderInitInfoAndroid.type = XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR;
            loaderInitInfoAndroid.next = NULL;
            loaderInitInfoAndroid.applicationVM = app->activity->vm;
            loaderInitInfoAndroid.applicationContext = app->activity->clazz;
            initializeLoader((const XrLoaderInitInfoBaseHeaderKHR*)&loaderInitInfoAndroid);
        }

        program->CreateInstance();//创建实例
        program->InitializeSystem();
        program->InitializeSession();//创建会话
        program->CreateSwapchains();//创建交换链-Q：交换链是什么
        program->InitializeApplication();
        // modify by qi.cheng
        // Marker 识别功能代码调用
        program -> InitializeMarker();//AR标记识别初始化
        program-> AddMarkerImages();//加载标记图片
        program-> InitializePlaneTracking();//AR平面追踪
        while (app->destroyRequested == 0) {//这里是主循环，除非执行破坏请求否则不会退出（每帧循环渲染）
            // Read all pending events.
            for (;;) {
                int events;
                struct android_poll_source* source;
                // If the timeout is zero, returns immediately without blocking.
                // If the timeout is negative, waits indefinitely until an event appears.检测app状态以做出相应选择-在后台的时候先不加载以节省算力
                const int timeoutMilliseconds = (!appState.Resumed && !program->IsSessionRunning() && app->destroyRequested == 0) ? -1 : 0;
                if (ALooper_pollAll(timeoutMilliseconds, nullptr, &events, (void**)&source) < 0) {
                    break;
                }

                // Process this event.
                if (source != nullptr) {
                    source->process(app, source);
                }
            }

            program->PollEvents(&exitRenderLoop, &requestRestart);

            if (exitRenderLoop && !requestRestart) {//用户退出或故障时且不需要重启时杀死进程
                ANativeActivity_finish(app->activity);

                killProcess();
            }

            if (!program->IsSessionRunning()) {
                // Throttle loop since xrWaitFrame won't be called.
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
                continue;
            }

            program->PollActions();
            program->RenderFrame();
            program->ProcessMarkerData(); // 图像识别处理
            //program->ProcessPlaneTracking();//平面识别处理
        }
        app->activity->vm->DetachCurrentThread();
    }
    catch (const std::exception &ex)
    {
        Log::Write(Log::Level::Error, __FILE__, __LINE__, ex.what());
    }
    catch (...)
    {
        Log::Write(Log::Level::Error, __FILE__, __LINE__, "Unknown Error");
    }
}
