// This is skyseeker's renderer browser file
// In theory, this is where JS execution occurs


#pragma once

#include "include/wrapper/cef_message_router.h"
#include "include/cef_render_process_handler.h"
#include "include/cef_app.h"

// Implementation of CefApp for the renderer process.
class RendererApp : public CefApp, public CefRenderProcessHandler {
public:
    RendererApp();

    // CefApp methods:
    CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override;

    // CefRenderProcessHandler methods:
    void OnWebKitInitialized() override;
    void OnContextCreated(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefV8Context> context) override;
    void OnContextReleased(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefV8Context> context) override;
    bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefProcessId source_process,
        CefRefPtr<CefProcessMessage> message) override;

private:
    // Handles the renderer side of query routing.
    CefRefPtr<CefMessageRouterRendererSide> message_router_;

    IMPLEMENT_REFCOUNTING(RendererApp);
};
