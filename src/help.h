#define TEXT_DISABLED_WRAPPED(__text__) \
GUI::PushStyleColor(ImGuiCol_Text, { .6f, .6f, .6f, 1 }); \
ImGui::TextWrapped(__text__); \
GUI::PopStyleColor();

void display_help()
{
    GUI::Text("Goal"); 
    GUI::Spacing();
    TEXT_DISABLED_WRAPPED("Read the level's description in the menu bar, and outputs the solution. Run your code by pressing Play, or F5.");
    TEXT_DISABLED_WRAPPED("If the solution matches the desired output, the code will be re-run multiple times with different inputs to make sure your solution works in every situation.");
    GUI::Spacing(); GUI::Separator(); GUI::Spacing();

    GUI::Text("A register (Accumulator)"); 
    GUI::Spacing();
    TEXT_DISABLED_WRAPPED("The A register is a variable that can hold a value. It is called the Accumulator. Every instruction reads and/or writes to the accumulator.");
    GUI::Spacing(); GUI::Separator(); GUI::Spacing();

    GUI::Text("RAM (Random Access Memory)"); 
    GUI::Spacing();
    TEXT_DISABLED_WRAPPED("The memory can be accessed through various instructions directly by writing the address number.");
    GUI::Spacing();
    GUI::TextDisabled("0 LDA 3");
    GUI::Spacing();
    TEXT_DISABLED_WRAPPED("It can be accessed indirectly by surrounding the address in brackets [].");
    GUI::Spacing();
    GUI::TextDisabled("0 LDA [3]");
    GUI::Spacing();
    TEXT_DISABLED_WRAPPED("By clicking on the address in the RAM window, the address can be renamed with text. Then it can be referenced in the code by its text name.");
    GUI::Spacing();
    GUI::TextDisabled("0 LDA TEMP");
    GUI::Spacing();
    GUI::Spacing(); GUI::Separator(); GUI::Spacing();

    GUI::Text("Labels"); 
    GUI::Spacing();
    TEXT_DISABLED_WRAPPED("You can mark your code with labels to work as jump points.");
    GUI::TextDisabled("0 LOOP:    # <----+");
    GUI::TextDisabled("1 INP      #      |");
    GUI::TextDisabled("2 OUT      #      |");
    GUI::TextDisabled("3 JMP LOOP # -----+");
    GUI::Spacing(); GUI::Separator(); GUI::Spacing();

    // Instructions
    GUI::Text("INP");
    GUI::Spacing();
    TEXT_DISABLED_WRAPPED("Move Input into A.");
    GUI::Spacing(); GUI::Separator(); GUI::Spacing();

    GUI::Text("OUT");
    GUI::Spacing();
    TEXT_DISABLED_WRAPPED("Copy A to Output.");
    GUI::Spacing(); GUI::Separator(); GUI::Spacing();

    GUI::Text("SET");
    GUI::Spacing();
    TEXT_DISABLED_WRAPPED("Set value to A. It can be a number from -99 to 99, or a letter from A to Z.");
    GUI::Spacing();
    GUI::TextDisabled("0 SET 16");
    GUI::TextDisabled("1 OUT # OUTPUTS 16");
    GUI::TextDisabled("2 SET H");
    GUI::TextDisabled("3 OUT # OUTPUTS H");
    GUI::Spacing(); GUI::Separator(); GUI::Spacing();

    GUI::Text("LDA");
    GUI::Spacing();
    TEXT_DISABLED_WRAPPED("Copy RAM to A.");
    GUI::Spacing();
    GUI::TextDisabled("0 LDA 3");
    GUI::TextDisabled("1 OUT   # OUTPUTS RAM(3)");
    GUI::Spacing(); GUI::Separator(); GUI::Spacing();

    GUI::Text("STA");
    GUI::Spacing();
    TEXT_DISABLED_WRAPPED("Copy A to RAM.");
    GUI::Spacing();
    GUI::TextDisabled("0 INP   # READ INPUT");
    GUI::TextDisabled("1 STA 3 # WRITE AT ADDR 3");
    GUI::Spacing(); GUI::Separator(); GUI::Spacing();

    GUI::Text("ADD");
    GUI::Spacing();
    TEXT_DISABLED_WRAPPED("Add RAM to A.");
    GUI::Spacing();
    GUI::TextDisabled("0 INP   # READ INPUT");
    GUI::TextDisabled("1 ADD 3 # ADD VALUE OF RAM(3)");
    GUI::Spacing(); GUI::Separator(); GUI::Spacing();

    GUI::Text("ADD");
    GUI::Spacing();
    TEXT_DISABLED_WRAPPED("Add RAM to A.");
    GUI::Spacing();
    GUI::TextDisabled("0 INP   # READ INPUT");
    GUI::TextDisabled("1 ADD 3 # A += RAM(3)");
    GUI::Spacing(); GUI::Separator(); GUI::Spacing();

    GUI::Text("SUB");
    GUI::Spacing();
    TEXT_DISABLED_WRAPPED("Subtract RAM to A.");
    GUI::Spacing();
    GUI::TextDisabled("0 INP   # READ INPUT");
    GUI::TextDisabled("1 SUB 3 # A -= RAM(3)");
    GUI::Spacing(); GUI::Separator(); GUI::Spacing();

    GUI::Text("INC");
    GUI::Spacing();
    TEXT_DISABLED_WRAPPED("Increment RAM and copy to A.");
    GUI::Spacing();
    GUI::TextDisabled("0 SET 0  # A = 0");
    GUI::TextDisabled("1 STA 3  # RAM(3) = A");
    GUI::TextDisabled("2 INC 3  # A = ++RAM(3)");
    GUI::Spacing(); GUI::Separator(); GUI::Spacing();

    GUI::Text("DEC");
    GUI::Spacing();
    TEXT_DISABLED_WRAPPED("Decrement RAM and copy to A.");
    GUI::Spacing();
    GUI::TextDisabled("0 SET 0  # A = 0");
    GUI::TextDisabled("1 STA 3  # RAM(3) = A");
    GUI::TextDisabled("2 INC 3  # A = --RAM(3)");
    GUI::Spacing(); GUI::Separator(); GUI::Spacing();

    GUI::Text("JMP");
    GUI::Spacing();
    TEXT_DISABLED_WRAPPED("Jump to label.");
    GUI::Spacing();
    GUI::TextDisabled("0 LOOP:    # <----+");
    GUI::TextDisabled("1 INP      #      |");
    GUI::TextDisabled("2 OUT      #      |");
    GUI::TextDisabled("3 JMP LOOP # -----+");
    GUI::Spacing(); GUI::Separator(); GUI::Spacing();

    GUI::Text("JPE");
    GUI::Spacing();
    TEXT_DISABLED_WRAPPED("Jump to label if A is equal to zero.");
    GUI::Spacing();
    GUI::TextDisabled("0 LOOP:    # <----+");
    GUI::TextDisabled("1 SET 0    #      |");
    GUI::TextDisabled("3 JPE LOOP # -----+");
    GUI::Spacing(); GUI::Separator(); GUI::Spacing();

    GUI::Text("JPL");
    GUI::Spacing();
    TEXT_DISABLED_WRAPPED("Jump to label if A is negative.");
    GUI::Spacing();
    GUI::TextDisabled("0 LOOP:    # <----+");
    GUI::TextDisabled("1 SET -1   #      |");
    GUI::TextDisabled("3 JPE LOOP # -----+");
    GUI::Spacing(); GUI::Separator(); GUI::Spacing();

    GUI::Text("JPG");
    GUI::Spacing();
    TEXT_DISABLED_WRAPPED("Jump to label if A is greater or equal to zero.");
    GUI::Spacing();
    GUI::TextDisabled("0 LOOP:    # <----+");
    GUI::TextDisabled("1 SET 1    #      |");
    GUI::TextDisabled("3 JPG LOOP # -----+");
    GUI::Spacing(); GUI::Separator(); GUI::Spacing();

    GUI::Text("NOP");
    GUI::Spacing();
    TEXT_DISABLED_WRAPPED("Does nothing.");
    GUI::Spacing();
    GUI::Spacing(); GUI::Separator(); GUI::Spacing();
}
