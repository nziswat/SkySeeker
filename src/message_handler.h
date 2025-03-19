#pragma once

#include "include/wrapper/cef_message_router.h"
#include "src/structs.h"
#include <string>

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


            sendDebug("asdf", browser);
            return true;
        }
        return false;
    }

        void sendPacket(aircraftPacket packet,
            CefRefPtr<CefBrowser> browser){

            browser->GetMainFrame()->ExecuteJavaScript(
                "console.log('Hello from C++!');",
                browser->GetMainFrame()->GetURL(),
                0);
    }
        void sendDebug(std::string debug,
            CefRefPtr<CefBrowser> browser) {

            browser->GetMainFrame()->ExecuteJavaScript(
                "debugCall(\"" + debug + " \")",
                browser->GetMainFrame()->GetURL(),
                0);
        }

    //
    //IMPLEMENT_REFCOUNTING(MyHandler);
};