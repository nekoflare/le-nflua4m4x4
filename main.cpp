#include <iostream>
#include <fstream>
#include <cstdint>
#include <vector>

enum gate_type_t
{
    PSTH = 0,   // Pass-through
    AND = 1,
    OR = 2,
    NOT = 3
};

enum io_gate_t
{
    A = 0,
    B = 1,
    C = 2,
    D = 3,
};

struct instruction_t
{
    uint8_t sel_col : 2;    // Column.
    uint8_t sel_row : 2;    // Row.
    uint8_t rst : 1;        // Resets the chip if ALU = SEL.
    uint8_t set_gate_type : 1; // Set the gate type
    uint8_t sel_gate_type : 2; // Gate type selector.
    uint8_t abio : 1; // Make this I/O input connected to A (0) or B (1) internal bus.
    uint8_t perf : 1; // Perform I/O config or confirm changes.
    uint8_t mout : 1; // Make this I/O port an output.
    uint8_t min : 1; // Make this I/O port an input.
    uint8_t iosel : 2; // The selected I/O port.
    uint8_t unused : 2;
};
static_assert(sizeof(instruction_t) == 2, "size of instruction_t must be 2.");

// Function to reset LU by address
void reset_lu_by_address(instruction_t &instr, uint8_t col, uint8_t row)
{
    instr.sel_col = col & 0x03;  // Select the column (only lower 2 bits)
    instr.sel_row = row & 0x03;  // Select the row (only lower 2 bits)
    instr.rst = 1;               // Set reset flag
}

// Function to set I/O configuration (input or output), with LU address
void set_io_config(instruction_t &instr, uint8_t col, uint8_t row, uint8_t io_port, bool is_input)
{
    instr.sel_col = col & 0x03;  // Set the column address
    instr.sel_row = row & 0x03;  // Set the row address
    instr.iosel = io_port & 0x03;   // Select I/O port (only lower 2 bits)
    instr.perf = 1;                 // Perform I/O configuration
    if (is_input)
    {
        instr.min = 1;  // Set as input
        instr.mout = 0; // Clear output flag
    }
    else
    {
        instr.mout = 1; // Set as output
        instr.min = 0;  // Clear input flag
    }
}

// Function to set gate type, with LU address
void set_gate_type(instruction_t &instr, uint8_t col, uint8_t row, gate_type_t gate_type)
{
    instr.sel_col = col & 0x03;  // Set the column address
    instr.sel_row = row & 0x03;  // Set the row address
    instr.set_gate_type = 1;     // Enable gate type setup
    instr.sel_gate_type = gate_type; // Select the gate type (2-bit)
}

// Updated function to set internal bus connection (ABIO) with LU address and I/O selection
void set_internal_bus_connection(instruction_t &instr, uint8_t col, uint8_t row, uint8_t io_port, bool connect_to_bus_b)
{
    instr.sel_col = col & 0x03;  // Set the column address (only lower 2 bits)
    instr.sel_row = row & 0x03;  // Set the row address (only lower 2 bits)
    instr.iosel = io_port & 0x03; // Select the I/O port (only lower 2 bits)
    instr.abio = connect_to_bus_b ? 1 : 0; // Set ABIO: 0 for A-bus, 1 for B-bus
    instr.perf = 1;                        // Confirm changes (set PERF to 1)
}


// Function to save a vector of instructions to a binary file
void save_instructions_to_binary(const std::vector<instruction_t> &instructions, const std::string &filename)
{
    std::ofstream file(filename, std::ios::binary);
    if (file.is_open())
    {
        for (const auto &instr : instructions)
        {
            // Write each instruction as raw binary data
            file.write(reinterpret_cast<const char*>(&instr), sizeof(instruction_t));
        }
        file.close();
        std::cout << "Instructions saved to " << filename << " as binary." << std::endl;
    }
    else
    {
        std::cerr << "Failed to open file: " << filename << std::endl;
    }
}


int main()
{
    std::vector<instruction_t> instructions;

    for (int i = 0; 4 > i; i++)
    {
        for (int j = 0; 4 > j; j++)
        {
            instruction_t reset_lu {0};
            reset_lu_by_address(reset_lu, i, j);
            instructions.push_back(reset_lu);
        }
    }

    // Set A0B0 to be an AND gate.
    instruction_t a0b0_set_gate {0};
    set_gate_type(a0b0_set_gate, 0, 0, AND);
    instructions.push_back(a0b0_set_gate);

    // Make input D to bus A and A to bus B.
    instruction_t a0b0_set_d2a_A_input {0};
    set_io_config(a0b0_set_d2a_A_input, 0, 0, A, true);

    instruction_t a0b0_set_a2b_B_input {0};
    set_io_config(a0b0_set_a2b_B_input, 0, 0, B, true);

    instruction_t a0b0_set_d2a_Q_output {0};
    set_io_config(a0b0_set_d2a_Q_output, 0, 0, D, false);

    instructions.push_back(a0b0_set_d2a_A_input);
    instructions.push_back(a0b0_set_a2b_B_input);
    instructions.push_back(a0b0_set_d2a_Q_output);

    // Make A to A and B to B.
    instruction_t a0b0_a2a {0};
    instruction_t a0b0_b2b {0};

    set_internal_bus_connection(a0b0_a2a, 0, 0, A, false);
    set_internal_bus_connection(a0b0_b2b, 0, 0, B, true);

    instructions.push_back(a0b0_a2a);
    instructions.push_back(a0b0_b2b);

    // Make (0, 0), C input.
    instruction_t a0b0_c0i {0};
    set_io_config(a0b0_c0i, 0, 0, C, true);
    instructions.push_back(a0b0_c0i);

    // Now. connect (0, 1) A (IN) -> D (OUT)
    instruction_t a1b0_ai_do {0};
    set_gate_type(a1b0_ai_do, 0, 1, PSTH);
    instructions.push_back(a1b0_ai_do);
    a1b0_ai_do = {0};

    set_io_config(a1b0_ai_do, 0, 1, A, true);
    instructions.push_back(a1b0_ai_do);
    a1b0_ai_do = {0};

    set_io_config(a1b0_ai_do, 0, 1, C, false);
    instructions.push_back(a1b0_ai_do);
    a1b0_ai_do = {0};

    save_instructions_to_binary(instructions, "nflua4m4x4-default.hex");
    return 0;
}
