#include "../src/borderBoundary.h"
#include <cassert>
#include <iostream>

int main() {
    borderBoundary node;
    node.setup();
    auto vec = [&](const std::string& name) -> ofParameter<std::vector<float>>& {
        return node.parameter<std::vector<float>>(name);
    };
    int count = 0;
    std::vector<char> order;
    auto yEvent = vec("Out.Y").newListener([&](std::vector<float>&) { order.push_back('Y'); });
    auto xEvent = vec("Out.X").newListener([&](std::vector<float>&) {
        ++count; order.push_back('X');
        assert(vec("Out.X").get().size() == vec("Out.Y").get().size());
    });
    vec("Y") = {.1f, .9f, -1.f};
    node.parameter<float>("Border distance") = .2f;
    node.parameter<float>("Falloff") = 0.f;
    node.parameter<float>("Roundness") = 0.f;
    node.parameter<float>("Curve") = 2.f;
    ofEventArgs event; node.update(event);
    assert(count == 0); // Only X has a listener, including control changes.
    bool coherent = false;
    auto checkPair = vec("Out.Y").newListener([&](std::vector<float>&) {
        assert(std::abs(vec("Out.X").get()[0] - .2f) < 1e-6f);
        assert(std::abs(vec("Out.Y").get()[1] - .8f) < 1e-6f);
        coherent = true;
    });
    vec("X") = {.1f, .9f, -1.f};
    assert(count == 1 && coherent && order == std::vector<char>({'Y', 'X'}));
    assert(vec("Out.X").get()[2] == -1 && vec("Out.Y").get()[2] == -1);
    checkPair.reset(); order.clear();
    const auto oldX = vec("Out.X").get(), oldY = vec("Out.Y").get();
    vec("X") = {.5f}; // mismatched: no work, publication or partial output
    assert(count == 1 && order.empty() && vec("Out.X").get() == oldX && vec("Out.Y").get() == oldY);
    vec("Y") = {.5f}; assert(count == 1);
    vec("X") = {.5f}; assert(count == 2); // OF vector events also fire for repeated X
    assert(vec("Out.X").get() == std::vector<float>({.5f}));
    // Even if X output stays unchanged, downstream X listeners must see new Y.
    vec("Y") = {.7f}; vec("X") = {.5f};
    assert(count == 3 && vec("Out.Y").get() == std::vector<float>({.7f}));
    vec("Y") = {}; vec("X") = {};
    assert(count == 4 && vec("Out.X").get().empty() && vec("Out.Y").get().empty());
    std::cout << "Border Boundary node: X-only trigger, mismatched-size skip, staged Y-before-X events and empty input passed.\n";
}
