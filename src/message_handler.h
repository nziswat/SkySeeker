#pragma once

#include "include/wrapper/cef_message_router.h"

class MessageHandler : public CefMessageRouterBrowserSide::Handler {
public:
    bool OnQuery(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        int64_t query_id,
        const CefString& request,
        bool persistent,
        CefRefPtr<Callback> callback) override {
        if (request == "spawn_aircraft") {
            // Sample aircraft data
            std::string json = R"({
                "ID": "Flight777",
                "lat": 27.95,
                "long": -82.45,
                "head": 90,
                "alt": 32000,
                "speed": 450
            })";


            browser->GetMainFrame()->ExecuteJavaScript(
                "console.log('Hello from C++!');",
                browser->GetMainFrame()->GetURL(),
                0);

            callback->Success(json);
            return true;
        }
        return false;
    }
    //
    //IMPLEMENT_REFCOUNTING(MyHandler);
};