#include "axle/core/window/AX_IWindow.hpp"

namespace axle::core {

SharedState::SharedState(WindowSpec spec, uint32_t maxEventsQueue)
    : m_MaxEventQueueCapacity(maxEventsQueue),
      m_Width(spec.width), m_Height(spec.height),
      m_IsResizable(spec.resizable), m_Title(spec.title) {}

bool SharedState::IsRunning() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_IsRunning;
}

bool SharedState::IsResizable() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_IsResizable;
}

bool SharedState::IsQuitting() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_ShouldQuit;
}

WndCursorMode SharedState::GetCursorMode() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_CursorMode;
}

uint32_t SharedState::GetWidth() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Width;
}

uint32_t SharedState::GetHeight() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Height;
}

uint32_t SharedState::GetMaxEventQueueCapacity() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_MaxEventQueueCapacity;
}

std::string SharedState::GetTitle() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Title;
}

void SharedState::PushEvent(const WndEvent& event) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (event.type == WndEventType::Void) return;
    if (m_SharedEventsQ.size() >= m_MaxEventQueueCapacity)
        m_SharedEventsQ.pop_front();
    m_SharedEventsQ.push_back(event);
}

std::deque<WndEvent> SharedState::TakeEvents() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    std::deque<WndEvent> events;
    events.swap(m_SharedEventsQ);
    return events;
}

void SharedState::SetRunning(bool running) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_IsRunning = running;
}

void SharedState::SetResizable(bool resizable) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_IsResizable = resizable;
}

void SharedState::SetTitle(const std::string& title) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Title = title;
}

void SharedState::SetSize(uint32_t width, uint32_t height) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Width = width;
    m_Height = height;
}

void SharedState::SetCursorMode(WndCursorMode mode) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_CursorMode = mode;
}

void SharedState::RequestQuit() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_ShouldQuit = true;
}

}