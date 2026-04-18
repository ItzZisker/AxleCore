#pragma once

class fwResTimer
{
public:
    explicit fwResTimer(unsigned int resolution = 1);
    ~fwResTimer();

    fwResTimer(const fwResTimer&) = delete;
    fwResTimer& operator=(const fwResTimer&) = delete;

    fwResTimer(fwResTimer&& other) noexcept;
private:
    unsigned int _resolution;
    bool _active;
};