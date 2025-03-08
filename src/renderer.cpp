// This is Skyseeker's renderer browser file
// In theory, this is where JS execution occurs.

#include "src/renderer.h"
#include "include/wrapper/cef_message_router.h"
#include "include/cef_render_process_handler.h"
#include "include/cef_app.h"

RendererApp::RendererApp() = default;

CefRefPtr<CefRenderProcessHandler> RendererApp::GetRenderProcessHandler() {
    return this;
}

void RendererApp::OnWebKitInitialized() {
    // Create the renderer-side router for query handling.
    CefMessageRouterConfig config;
    message_router_ = CefMessageRouterRendererSide::Create(config);
}

void RendererApp::OnContextCreated(CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefRefPtr<CefV8Context> context) {
    if (message_router_) {
        message_router_->OnContextCreated(browser, frame, context);
    }
}

void RendererApp::OnContextReleased(CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefRefPtr<CefV8Context> context) {
    if (message_router_) {
        message_router_->OnContextReleased(browser, frame, context);
    }
}

bool RendererApp::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefProcessId source_process,
    CefRefPtr<CefProcessMessage> message) {
    return message_router_ && message_router_->OnProcessMessageReceived(browser, frame, source_process, message);
}
