#pragma once

// Tiny logger writing to C:\sc4srp.log inside the Wine prefix. Silent by
// default. Set the SC4SRP_DEBUG env var to "1" (in Steam launch options) to
// enable logging. The env var is read once at PostAppInit via InitLogging().

void InitLogging();

bool LoggingEnabled();

void Log(const char* msg);
void Logf(const char* fmt, ...);
