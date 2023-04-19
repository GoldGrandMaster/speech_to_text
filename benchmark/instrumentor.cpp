#include "instrumentor.hpp"

#include <iostream>

Instrumentor::Instrumentor()
    : m_CurrentSession(nullptr), m_ProfileCount(0)
{
}

void Instrumentor::BeginSession(const std::string &name, const std::string &filepath = "results.json")
{
    m_OutputStream.open(filepath);
    WriteHeader();
    m_CurrentSession = new InstrumentationSession{name};
}

void Instrumentor::EndSession()
{
    WriteFooter();
    m_OutputStream.close();
    delete m_CurrentSession;
    m_CurrentSession = nullptr;
    m_ProfileCount = 0;
}

void Instrumentor::WriteProfile(const ProfileResult &result)
{
    if (m_ProfileCount++ > 0)
        m_OutputStream << ",";

    std::string name = result.Name;
    std::replace(name.begin(), name.end(), '"', '\'');

    m_OutputStream << "{";
    m_OutputStream << "\"cat\":\"function\",";
    m_OutputStream << "\"dur\":" << (result.End - result.Start) << ',';
    m_OutputStream << "\"name\":\"" << name << "\",";
    m_OutputStream << "\"ph\":\"X\",";
    m_OutputStream << "\"pid\":0,";
    m_OutputStream << "\"tid\":" << result.ThreadID << ",";
    m_OutputStream << "\"ts\":" << result.Start;
    m_OutputStream << "}";

    m_OutputStream.flush();
}

void Instrumentor::WriteHeader()
{
    m_OutputStream << "{\"otherData\": {},\"traceEvents\":[";
    m_OutputStream.flush();
}

void Instrumentor::WriteFooter()
{
    m_OutputStream << "]}";
    m_OutputStream.flush();
}

Instrumentor &Instrumentor::Get()
{
    static Instrumentor instance;
    return instance;
}

InstrumentationTimer::InstrumentationTimer(const char *name) : m_Name(name), m_Stopped(false)
{
    m_StartTimepoint = std::chrono::high_resolution_clock::now();
}

InstrumentationTimer::~InstrumentationTimer()
{
    if (!m_Stopped)
        Stop();
}

void InstrumentationTimer::Stop()
{
    auto endTimepoint = std::chrono::high_resolution_clock::now();

    long long start =
        std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimepoint).time_since_epoch().count();
    long long end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().count();

    uint32_t threadID = std::hash<std::thread::id>{}(std::this_thread::get_id());
    Instrumentor::Get().WriteProfile({m_Name, start, end, threadID});

    m_Stopped = true;
}

Timer::Timer(char *name)
{
    m_Name = name;
    m_StartTimePoint = std::chrono::high_resolution_clock::now();
    //std::cout << "started timer " << name
    //          << " \n";
}

void Timer::Stop()
{
    stopped = true;
    auto endTimePoint = std::chrono::high_resolution_clock::now();

    auto start = std::chrono::time_point_cast<std::chrono::milliseconds>(m_StartTimePoint).time_since_epoch().count();
    auto end = std::chrono::time_point_cast<std::chrono::milliseconds>(endTimePoint).time_since_epoch().count();

    auto duration = end - start;
    std::cout << m_Name << ": " << duration << "ms \n";
}


Timer::~Timer()
{
    if (!stopped)
    {
        Timer::Stop();
    }
}
