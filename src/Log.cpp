#include "Log.h"

#include <windows.h>
#include <cstdio>
#include <cstdarg>

static bool g_enabled = false;

void InitLogging() {
    char buf[8] = {0};
    DWORD n = GetEnvironmentVariableA("SC4SRP_DEBUG", buf, sizeof(buf));
    g_enabled = (n > 0 && buf[0] != '0' && buf[0] != '\0');
}

bool LoggingEnabled() {
    return g_enabled;
}

static void Write(const char* msg) {
    FILE* f = std::fopen("C:\\sc4srp.log", "a");
    if (!f) return;
    std::fputs(msg, f);
    std::fputc('\n', f);
    std::fclose(f);
}

void Log(const char* msg) {
    if (g_enabled) Write(msg);
}

void Logf(const char* fmt, ...) {
    if (!g_enabled) return;
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    Write(buf);
}
