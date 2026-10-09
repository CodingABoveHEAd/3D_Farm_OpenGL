#pragma once

class Farmhouse
{
public:
    void render() const;
    void setPowerAmount(float amount) { powerAmount_ = amount; }

private:
    void renderBody() const;
    void renderFoundation() const;
    void renderRoof() const;
    void renderDoor() const;
    void renderWindows() const;
    void drawWindow(float x) const;
    void drawWindowCross(float x, float y, float z) const;
    void renderWindowShutters() const;
    void renderChimney() const;
    void renderPorch() const;
    void renderPorchSteps() const;
    void renderPorchRailings() const;
    void renderWallTrim() const;
    void renderAtticWindow() const;
    float powerAmount_ = 1.0f;
};
