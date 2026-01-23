#pragma once
#include "Node.h"
struct Bus : public Node {
    Bus(std::vector<Node*>* container, Vector2 pos = { 0,0 }, Output_connector* input = nullptr) : Node(container, pos, { 0, 0 }, ColorBrightness(BLUE, -0.4f)) {
        label = "BUS_0";
        inputs.push_back(Input_connector(this, 0, "", input));
        recompute_size();
        find_connections();

    }
    Bus(const Bus* base) : Node(base) {
        find_connections();
    }

    void find_connections() {
        for (Node* node : *container) {
            if (node) {
                Bus* bus = dynamic_cast<Bus*>(node);
                if (bus && bus != this) {
                    if (bus->label == label) {
                        bus_values_has_updated = bus->bus_values_has_updated;
                        bus_values = bus->bus_values;
                        while ((*bus_values).size() < inputs.size()) {
                            (*bus_values).push_back(false);
                        }
                        return;
                    }
                }
            }
        }

        bus_values_has_updated = std::make_shared<bool>();
        bus_values = std::make_shared<std::vector<bool>>();
        while ((*bus_values).size() < inputs.size()) {
            (*bus_values).push_back(false);
        }
    }

    virtual void add_input() override;

    virtual void remove_input() override;

    virtual void change_label(const char* newlabel) override;

    Node* copy() const override { return new Bus(this); }

    virtual json to_JSON() const override;

    virtual void load_extra_JSON(const json& nodeJson) override;
    virtual void load_extra_bin(const uint8_t* node_data_ptr, const uint8_t* save_ptr) override;

    virtual std::string get_label() const override { return std::string(label); }

    virtual Texture get_texture() const override { return{ 0 }; }

    virtual std::string get_type_str() const override { return"Bus"; }
    virtual NodeType get_type() const override { return NodeType::BusNode; }

    virtual std::vector<Input_connector*> connected_inputs(size_t output_idx) {
        std::vector<Input_connector*> conned;

        for (Node* node : *container) {
            Bus* bus = dynamic_cast<Bus*>(node);
            if (bus) {
                if (bus->label == label && bus->inputs.size() > output_idx) {
                    conned.push_back(&bus->inputs[output_idx]);
                }
            }
        }
        return conned;
    }

    size_t bus_values_size() const { return bus_values->size(); }
    std::weak_ptr<const std::vector<bool>> get_bus_values() const { return bus_values; };
private:
    std::shared_ptr<std::vector<bool>> bus_values;
    std::shared_ptr<bool> bus_values_has_updated;
};