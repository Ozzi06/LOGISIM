#pragma once
#include "Node.h"
#include <math.h>
#include "game.h"
class ROMNode : public Node {
public:
    //constructors and destructors
    ROMNode(std::vector<Node*>* container, Vector2 pos = { 0, 0 }) : Node(container, pos, { 0, 0 }, ColorBrightness(GREEN, -0.6f)) { 
        label = "ROM";
        while (outputs.size() < 8) {
            add_input();
        }
        recompute_size();
    }
    
    ROMNode(const ROMNode* base) : Node(base) {}
    ~ROMNode() {}

    virtual bool show_node_editor() override;

    void grab_new_data();

    Node* copy() const override { return new ROMNode(this); }

    //virtual void draw() override;
    //virtual bool show_node_editor() override;

    virtual void add_input() override { // adds an output connector
        Game& game = Game::getInstance();
        if (outputs.size() == outputs.capacity()) return;
        outputs.push_back(Output_connector(this, outputs.size(), "")); 
        recompute_input_connector_count();
        recompute_size();
        game.network_change();
    }

    virtual void remove_input() override {// removes an output connector
        if (outputs.size() > 1) {
            Game& game = Game::getInstance();
            for (Node* node : game.nodes) {
                for (auto& conn : node->inputs) {
                    if (conn.target == &outputs.back()) conn.target = nullptr;
                }
            }
            outputs.pop_back();
            recompute_input_connector_count();
            recompute_size();
            game.network_change();
        }
    }

    void recompute_input_connector_count() {
        Game& game = Game::getInstance();

        // Total number of bits to address, considering word size (outputs.size())
        size_t total_bits = data_save.size() * 8;
        size_t word_size = outputs.size();  // Word size based on the number of outputs
        size_t required_words = std::ceil(static_cast<double>(total_bits) / word_size);

        

        // Calculate the required address bits to address each word
        size_t required_bits = std::ceil(std::log2(required_words));
        if (required_words == 0) required_bits = 0;

        // Add input connectors until we have enough bits to address the whole range
        while (inputs.size() < required_bits) {
            inputs.push_back(Input_connector(this, inputs.size(), ""));
        }

        // Remove excess input connectors if we have more than needed
        while (inputs.size() > required_bits) {
            inputs.pop_back();
        }

        recompute_size();
        game.network_change();
    }

    virtual std::string get_type_str() const override { return"ROMNode"; }
    virtual NodeType get_type() const override { return NodeType::ROMNode; }
    virtual std::string get_label() const override { return std::string(label); }
    virtual Texture get_texture() const override { return { 0 }; }
    virtual const std::vector<Node*>* get_children() const override { return &nodes; }


    //virtual void load_extra_bin(const uint8_t* node_data_ptr, const uint8_t* save_ptr) override;
    //virtual void recompute_size() override;


    std::vector<Node*> nodes;
    std::vector<Node*> input_targs;
    std::vector<Node*> output_targs;
    void set_data_save(const std::vector<uint8_t>& data);
    const std::vector<uint8_t>* get_data_save() { return &data_save; }
    bool has_data_save() { return data_save.size() > 0; }
    //void allocate_data_save(const uint8_t* node);
private:
    std::vector<uint8_t> data_save;
};