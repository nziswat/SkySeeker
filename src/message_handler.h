#pragma once

#include "include/wrapper/cef_message_router.h"
#include "src/structs.h"
#include <string>
#include <thread>
#include "skyseekerTSV.h"
//#include "database.h"

class MessageHandler : public CefMessageRouterBrowserSide::Handler, public CefBaseRefCounted {
public:

    CefRefPtr<CefBrowser> browser_ref_m; //should keep a reference to the browser at all times.
    std::atomic<bool> driverStatus = 0; //initial state of the driver


    //Query code, which is used for when the browser calls back to CEF
    bool OnQuery(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        int64_t query_id,
        const CefString& request,
        bool persistent,
        CefRefPtr<Callback> callback) override {
        if (request == "startStopDriver") {
            driverStatus = !driverStatus;


            // Send response back to JS
            callback->Success(driverStatus ? "Driver Stopped" : "Driver Started");
            return true;
        }

        //sliced requests past this point
        //first 11 chars are checked for what function it is

        std::string fullReq = request.ToString();
        std::string firsthalf = fullReq.substr(0,11);
        
        if (firsthalf == "getICAOData") {// check first 10 chars
            std::string icao = fullReq.substr(11, 6);
			//std::string icao = "LALALA";
            icaoData data{};
            TSV::getDataForICAO(icao, data);
			callback->Success(icao);
			return true;
		}
        if (firsthalf == "savAircraft") {// check first 10 chars
            //Database& db = Database::getInstance("collection.db");

            std::string icao = fullReq.substr(11, 6); // icao
            std::string lat = fullReq.substr(17, 6); //lat
            std::string lon = fullReq.substr(23, 6); // long;


            callback->Success(icao);
            return true;
        }

        return false;  // wrong query
    }

        void sendPacket(aircraftPacket packet,
            CefRefPtr<CefBrowser> browser){

            browser->GetMainFrame()->ExecuteJavaScript(
                "console.log('Hello from C++!');",
                browser->GetMainFrame()->GetURL(),
                0);
    }



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

//task for sending packet to aircraft object in js
class AirPacketCefTask : public CefTask {
public:
    AirPacketCefTask(MessageHandler* handler, const std::string& message)
        : handler_(handler), message_(message) {}

    void Execute() override {
        if (handler_) {
            handler_->sendToJS("updateAircraftData", message_);
        }
    }

private:
    MessageHandler* handler_;
    std::string message_;
    IMPLEMENT_REFCOUNTING(AirPacketCefTask);
};


//task for sending data from .TSV to aircraft object in js (might not be used)
class getDataCefTask : public CefTask {
public:
    getDataCefTask(MessageHandler* handler, const std::string& message)
        : handler_(handler), message_(message) {
    }

    void Execute() override {
        if (handler_) {
            handler_->sendToJS("getICAOData", message_);
        }
    }

private:
    MessageHandler* handler_;
    std::string message_;
    IMPLEMENT_REFCOUNTING(getDataCefTask);
};

