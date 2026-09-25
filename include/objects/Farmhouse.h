#pragma once

class Farmhouse {
public:
    void render() const;

private:
    void renderBody() const;
    void renderRoof() const;
    void renderDoor() const;
    void renderWindows() const;
    void renderChimney() const;
    void renderPorch() const;
    void drawWindow(float x) const;
    void drawWindowCross(float x, float y, float z) const;
};
