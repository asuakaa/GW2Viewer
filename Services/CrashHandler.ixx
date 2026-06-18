export module GW2Viewer.Services.CrashHandler;
import GW2Viewer.Common.Time;
import GW2Viewer.User.Config;
import std;
import <csignal>;
import <Windows.h>;

export namespace GW2Viewer::Services
{

struct CrashHandler
{
    void Start()
    {
        if (IsDebuggerPresent())
            return;

        SetUnhandledExceptionFilter([](_EXCEPTION_POINTERS* pExceptionInfo) -> long __stdcall
        {
            OnCrash();
            return EXCEPTION_CONTINUE_SEARCH;
        });
        std::set_terminate([]
        {
            OnCrash();
            std::set_terminate(nullptr);
            std::terminate();
        });
        auto signalHandler = [](int signal)
        {
            OnCrash();
            std::signal(signal, SIG_DFL);
            raise(signal);
        };
        std::signal(SIGABRT, signalHandler);
        std::signal(SIGFPE, signalHandler);
        std::signal(SIGILL, signalHandler);
        std::signal(SIGSEGV, signalHandler);
        std::signal(SIGTERM, signalHandler);
    }

private:
    static void OnCrash()
    {
        if (static std::atomic_flag handled { }; handled.test_and_set())
            return;

        try { G::Config.Save(true); }
        catch (...) { }

        try { std::ofstream(std::format("crash-{:%F_%H-%M-%S}.log", Time::Now())) << std::stacktrace::current(); }
        catch (...) { }

        MessageBox(nullptr, L"GW2Viewer has crashed. Config and stacktrace were saved.", L"GW2Viewer", MB_OK | MB_ICONERROR);
    }
};

}

export namespace GW2Viewer::G::Services { GW2Viewer::Services::CrashHandler CrashHandler; }
