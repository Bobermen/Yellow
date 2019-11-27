//
// Created by Олег Бобров on 15.11.2019.
//

#ifndef WG_LINE_H
#define WG_LINE_H


#include "Behaviour.h"
#include "../core-components/renderers/LineRenderer.h"

class Line : public Behaviour {
private:
    LineRenderer *lineRenderer = nullptr;
public:
    int length;
    Point* points[2];

    explicit Line(int idx);

    void update() override;
    void applyLayer0(const nlohmann::json &json) override;
};

#endif //WG_LINE_H