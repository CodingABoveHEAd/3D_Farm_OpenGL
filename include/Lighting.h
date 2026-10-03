#pragma once

class Lighting {
public:
    void toggleNight();
    void apply() const;
    bool isNight() const;

private:
    bool night_ = false;
};
