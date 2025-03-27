#pragma once

#include "include/wrapper/cef_message_router.h"
#include "src/structs.h"
#include <string>

class MessageHandler : public CefMessageRouterBrowserSide::Handler, public CefBaseRefCounted {
public:

    CefRefPtr<CefBrowser> browser_ref_m; //should keep a reference to the browser at all times.

    //// Previous Debug Code
    /*
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
    }*/

        //sends a string
        void sendDebug(std::string debug) {
            browser_ref_m->GetMainFrame()->ExecuteJavaScript(
                "debugCall(\"" + debug + " \")",
                browser_ref_m->GetMainFrame()->GetURL(),
                0);
        }
        

        void MessageHandler::sendToJS(const std::string& functionName, const std::string& jsonMessage) {
            std::string script = functionName + "(" + jsonMessage + ");";
            // Execute JavaScript in the main frame
            browser_ref_m->GetMainFrame()->ExecuteJavaScript(script, browser_ref_m->GetMainFrame()->GetURL(), 0);
        }



private:
    IMPLEMENT_REFCOUNTING(MessageHandler);
};

class MyCefTask : public CefTask {
public:
    MyCefTask(MessageHandler* handler, const std::string& message)
        : handler_(handler), message_(message) {}

    void Execute() override {
        if (handler_) {
            handler_->sendToJS("updateAircraftData", message_);
        }
    }

private:
    MessageHandler* handler_;
    std::string message_;
    IMPLEMENT_REFCOUNTING(MyCefTask);
};
