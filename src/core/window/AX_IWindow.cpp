#include "axle/core/window/AX_IWindow.hpp"

#include <iostream>

namespace axle::core {

DiscreteState::DiscreteState(WindowSpec spec, uint32_t maxEventsQueue)
    : m_MaxEventQueueCapacity(maxEventsQueue),
      m_Width(spec.width), m_Height(spec.height),
      m_IsResizable(spec.resizable), m_Title(spec.title),
      m_WaitForNextEvent(spec.waitForNextEvent)
{
    for (std::size_t i{0}; i < std::size_t(WndMB::__End__); i++) m_MouseButtonsDown[i] = 0;
    for (std::size_t i{0}; i < std::size_t(WndKey::__End__); i++) m_KeysDown[i] = 0;
}

float DiscreteState::GetMouseX() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_MouseX;
}

float DiscreteState::GetMouseY() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_MouseY;
}

float DiscreteState::PeekMouseDX() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    float temp = m_MouseDX;
    m_MouseDX = 0.0f;
    return temp;
}

float DiscreteState::PeekMouseDY() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    float temp = m_MouseDY;
    m_MouseDY = 0.0f;
    return temp;
}

float DiscreteState::PeekMouseDWheel() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    float temp = m_MouseDWheel;
    m_MouseDWheel = 0.0f;
    return temp;
}

bool DiscreteState::IsKeyPressed(WndKey key) const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_KeysDown[static_cast<uint32_t>(key)] != 0;
}

bool DiscreteState::IsMouseButtonPressed(WndMB mb) const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_MouseButtonsDown[static_cast<uint32_t>(mb)] != 0;
}

bool DiscreteState::IsRunning() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_IsRunning;
}

bool DiscreteState::IsResizable() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_IsResizable;
}

bool DiscreteState::IsQuitting() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_ShouldQuit;
}

WndCursorMode DiscreteState::GetCursorMode() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_CursorMode;
}

float DiscreteState::GetAlpha() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Alpha;
}

uint32_t DiscreteState::GetWidth() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Width;
}

uint32_t DiscreteState::GetHeight() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Height;
}

uint32_t DiscreteState::GetMaxEventQueueCapacity() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_MaxEventQueueCapacity;
}

std::string DiscreteState::GetTitle() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Title;
}

void DiscreteState::PushEvent(const WndEvent& event) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (event.type == WndEventType::Void) return;
    if (m_SharedEventsQ.size() >= m_MaxEventQueueCapacity)
        m_SharedEventsQ.pop_front();
    m_SharedEventsQ.push_back(event);
}

std::deque<WndEvent> DiscreteState::TakeEvents() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    std::deque<WndEvent> events;
    events.swap(m_SharedEventsQ);
    return events;
}

void DiscreteState::SetRunning(bool running) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_IsRunning = running;
}

void DiscreteState::SetResizable(bool resizable) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_IsResizable = resizable;
}

void DiscreteState::SetTitle(const std::string& title) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Title = title;
}

void DiscreteState::SetSize(uint32_t width, uint32_t height) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Width = width;
    m_Height = height;
}

void DiscreteState::SetCursorMode(WndCursorMode mode) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_CursorMode = mode;
}

void DiscreteState::SetAlpha(float alpha) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Alpha = alpha;
}

void DiscreteState::SetMouseX(float mouseX) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_MouseX = mouseX;
}

void DiscreteState::SetMouseY(float mouseY) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_MouseY = mouseY;
}

void DiscreteState::AddMouseDX(float mouseDX) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_MouseDX += mouseDX;
}

void DiscreteState::AddMouseDY(float mouseDY) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_MouseDY += mouseDY;
}

void DiscreteState::AddMouseDWheel(float mouseDWheel) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_MouseDWheel += mouseDWheel;
}

void DiscreteState::SetMouseButtonState(WndMB mb, bool pressed) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_MouseButtonsDown[static_cast<uint32_t>(mb)] = (pressed ? 1 : 0);
}

void DiscreteState::SetKeyState(WndKey key, bool pressed) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_KeysDown[static_cast<uint32_t>(key)] = (pressed ? 1 : 0);
}

void DiscreteState::RequestQuit() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_ShouldQuit = true;
}

}