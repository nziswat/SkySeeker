//This is Sky Seeker's initialization code

#include <windows.h>

#include "include/cef_command_line.h"
#include "include/cef_sandbox_win.h"
#include "src/simple_app.h"
#include "src/renderer.h"
#include "src/RTL_interface.h"
#include <iostream>
#include <filesystem>
namespace fs = std::filesystem;

// When generating projects with CMake the CEF_USE_SANDBOX value will be defined
// automatically if using the required compiler version. Pass -DUSE_SANDBOX=OFF
// to the CMake command-line to disable use of the sandbox.
// Uncomment this line to manually enable sandbox support.
// #define CEF_USE_SANDBOX 1

void copyHTML() {
    fs::path exe_path = fs::current_path();
    fs::path source = exe_path / "../../../../../src/html";
    fs::path destination = exe_path;
    fs::create_directories(destination);


    // Copy the directory
    for (const auto& entry : fs::recursive_directory_iterator(source)) {
        const auto& path = entry.path();
        auto relative_path = fs::relative(path, source);
        auto destination_path = destination / "html" / relative_path;

        if (fs::is_directory(path)) {
            fs::create_directories(destination_path);
        }
        else {
            fs::copy_file(path, destination_path, fs::copy_options::overwrite_existing);
        }
    }

}





#if defined(CEF_USE_SANDBOX)
// The cef_sandbox.lib static library may not link successfully with all VS
// versions.
#pragma comment(lib, "cef_sandbox.lib")
#endif

// Entry point function for all processes.
int APIENTRY wWinMain(HINSTANCE hInstance,
                      HINSTANCE hPrevInstance,
                      LPTSTR lpCmdLine,
                      int nCmdShow) {
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);



  void* sandbox_info = nullptr;

#if defined(CEF_USE_SANDBOX)
  // Manage the life span of the sandbox information object. This is necessary
  // for sandbox support on Windows. See cef_sandbox_win.h for complete details.
  CefScopedSandboxInfo scoped_sandbox;
  sandbox_info = scoped_sandbox.sandbox_info();
#endif

  // Provide CEF with command-line arguments.
  CefMainArgs main_args(hInstance);


  // SimpleApp implements application-level callbacks for the browser process.
// It will create the first browser instance in OnContextInitialized() after
// CEF has initialized.
  CefRefPtr<SimpleApp> app(new SimpleApp);
  CefRefPtr<RendererApp> r_app(new RendererApp);

  // CEF applications have multiple sub-processes (render, GPU, etc) that share
  // the same executable. This function checks the command-line and, if this is
  // a sub-process, executes the appropriate logic.

  /*
  exit_code = CefExecuteProcess(main_args, app, sandbox_info);
  if (exit_code >= 0) {
    // The sub-process has completed so return here.
    return exit_code;
  }
  */
  // Parse command-line arguments for use in this method.
  CefRefPtr<CefCommandLine> command_line = CefCommandLine::CreateCommandLine();
  command_line->InitFromString(::GetCommandLineW());

  // Specify CEF global settings here.
  CefSettings settings;

#if !defined(CEF_USE_SANDBOX)
  settings.no_sandbox = true;
#endif




  // in theory, call the browser that inherits from cefRenderProcessHandler and pass it into here to get CefQuery working 
  CefExecuteProcess(main_args, r_app, sandbox_info);

  // Initialize the CEF browser process. May return false if initialization
  // fails or if early exit is desired (for example, due to process singleton
  // relaunch behavior).
  if (!CefInitialize(main_args, settings, app.get(), sandbox_info)) {
    return CefGetExitCode();
  }

  // Run the CEF message loop. This will block until CefQuitMessageLoop() is
  // called.
#ifdef _DEBUG // automatically move html files over if launched in debug mode
  copyHTML();
#endif

  std::cout << "Message loop about to run" << std::endl;
  CefRunMessageLoop();
  exitDriverThread = true;
  // Shut down CEF.
  CefShutdown();

  return 0;
}
