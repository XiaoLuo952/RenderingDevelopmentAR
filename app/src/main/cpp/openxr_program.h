// Copyright (c) 2017-2022, The Khronos Group Inc.
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

struct IOpenXrProgram {
    virtual ~IOpenXrProgram() = default;

    // Create an Instance and other basic instance-level initialization.
    virtual void CreateInstance() = 0;

    // Select a System for the view configuration specified in the Options and initialize the graphics device for the selected
    // system.
    virtual void InitializeSystem() = 0;

    // Create a Session and other basic session-level initialization.
    virtual void InitializeSession() = 0;

    // Create a Swapchain which requires coordinating with the graphics plugin to select the format, getting the system graphics
    // properties, getting the view configuration and grabbing the resulting swapchain images.
    virtual void CreateSwapchains() = 0;

    virtual void InitializeApplication() = 0;

    // Process any events in the event queue.
    virtual void PollEvents(bool* exitRenderLoop, bool* requestRestart) = 0;

    // Manage session lifecycle to track if RenderFrame should be called.
    virtual bool IsSessionRunning() const = 0;

    // Manage session state to track if input should be processed.
    virtual bool IsSessionFocused() const = 0;

    // Sample input actions and generate haptic feedback.
    virtual void PollActions() = 0;

    // Create and submit a frame.
    virtual void RenderFrame() = 0;

    //modify by qi.cheng
    // Virtual function for initializing the Marker functionality (image recognition).
    // This function should be implemented to load Marker-related extension interfaces
    // and enable Marker recognition features.
    virtual void InitializeMarker() = 0;

    // Virtual function for adding images to the Marker database.
    // This function should be implemented to add specific image data to the Marker system for recognition.
    virtual void AddMarkerImages() = 0;

    // Virtual function for processing Marker data.
    // This function should be implemented to handle events related to Marker additions, updates,
    // and removals, and to update the corresponding application logic.
    virtual void ProcessMarkerData() = 0;

    // Virtual function for initializing the Plane Tracking functionality.
    // This function should be implemented to load plane tracking-related extension interfaces
    // and enable plane detection features.
    virtual void InitializePlaneTracking() = 0;

    // Virtual function for processing plane tracking data.
    // This function should be implemented to handle events related to plane additions, updates,
    // and removals, and to update the corresponding application logic.
    virtual void ProcessPlaneTracking() = 0;


};

struct Swapchain {
    XrSwapchain handle;
    int32_t width;
    int32_t height;
};

std::shared_ptr<IOpenXrProgram> CreateOpenXrProgram(const std::shared_ptr<Options>& options,
                                                    const std::shared_ptr<IPlatformPlugin>& platformPlugin,
                                                    const std::shared_ptr<IGraphicsPlugin>& graphicsPlugin);
