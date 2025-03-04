#include "../../include/neoneo/terminal/terminal.hpp"
#include <iostream>
#include <mutex>

namespace neoneo {
namespace terminal {

// ANSI color code constants
constexpr const char* RESET_COLOR = "\033[0m";
constexpr const char* BLACK_COLOR = "\033[30m";
constexpr const char* RED_COLOR = "\033[31m";
constexpr const char* GREEN_COLOR = "\033[32m";
constexpr const char* YELLOW_COLOR = "\033[33m";
constexpr const char* BLUE_COLOR = "\033[34m";
constexpr const char* MAGENTA_COLOR = "\033[35m";
constexpr const char* CYAN_COLOR = "\033[36m";
constexpr const char* WHITE_COLOR = "\033[37m";
constexpr const char* BOLD_TEXT = "\033[1m";
constexpr const char* DIM_TEXT = "\033[2m";
constexpr const char* UNDERLINE_TEXT = "\033[4m";

// RAII class for terminal raw mode
TerminalRawMode::TerminalRawMode() {
    if (tcgetattr(STDIN_FILENO, &old_tio) == 0) {
        struct termios new_tio = old_tio;
        new_tio.c_lflag &= ~(ICANON | ECHO);
        if (tcsetattr(STDIN_FILENO, TCSANOW, &new_tio) == 0) {
            active = true;
        }
    }
}

TerminalRawMode::~TerminalRawMode() {
    if (active) {
        tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
    }
}

// Get a single keypress
char get_keypress() {
    TerminalRawMode raw_mode;
    char c = 0;
    if (read(STDIN_FILENO, &c, 1) <= 0) {
        return 0;
    }
    return c;
}

// Get color code for given color
std::string get_color_code(Color color) {
    switch (color) {
        case Color::RESET: return RESET_COLOR;
        case Color::BLACK: return BLACK_COLOR;
        case Color::RED: return RED_COLOR;
        case Color::GREEN: return GREEN_COLOR;
        case Color::YELLOW: return YELLOW_COLOR;
        case Color::BLUE: return BLUE_COLOR;
        case Color::MAGENTA: return MAGENTA_COLOR;
        case Color::CYAN: return CYAN_COLOR;
        case Color::WHITE: return WHITE_COLOR;
        case Color::BOLD: return BOLD_TEXT;
        case Color::DIM: return DIM_TEXT;
        case Color::UNDERLINE: return UNDERLINE_TEXT;
        default: return RESET_COLOR;
    }
}

// Get color for message type
std::string get_message_color(MessageType type) {
    switch (type) {
        case MessageType::USER: return BLUE_COLOR;
        case MessageType::SYSTEM: return YELLOW_COLOR;
        case MessageType::ERROR: return RED_COLOR;
        case MessageType::SUCCESS: return GREEN_COLOR;
        case MessageType::TOOL: return CYAN_COLOR;
        case MessageType::MODEL: return WHITE_COLOR;
        case MessageType::WARNING: return std::string(YELLOW_COLOR) + BOLD_TEXT;
        case MessageType::HEADER: return std::string(MAGENTA_COLOR) + BOLD_TEXT;
        case MessageType::NORMAL:
        default: return RESET_COLOR;
    }
}

// Colorize text with specified color
std::string colorize(const std::string& text, Color color) {
    return get_color_code(color) + text + RESET_COLOR;
}

// Colorize text with message type color
std::string colorize(const std::string& text, MessageType type) {
    return get_message_color(type) + text + RESET_COLOR;
}

// Print text with color
void print(const std::string& text, Color color, bool newline) {
    std::cout << get_color_code(color) << text << RESET_COLOR;
    if (newline) {
        std::cout << std::endl << std::flush;
    } else {
        std::cout << std::flush;
    }
}

// Print text with message type color
void print(const std::string& text, MessageType type, bool newline) {
    std::cout << get_message_color(type) << text << RESET_COLOR;
    if (newline) {
        std::cout << std::endl << std::flush;
    } else {
        std::cout << std::flush;
    }
}

// Unified confirmation dialog
bool confirm_dialog(
    ConfirmType type, 
    std::string_view title, 
    std::string_view message,
    std::string_view details,
    std::string_view tip
) {
    // Header line differs based on dialog type
    std::string header_line;
    MessageType header_type;
    switch (type) {
        case ConfirmType::SHELL_COMMAND:
            header_line = "----------- SHELL COMMAND CONFIRMATION -----------";
            header_type = MessageType::WARNING;
            break;
        case ConfirmType::FILE_OPERATION:
            header_line = "----------- FILE OPERATION CONFIRMATION -----------";
            header_type = MessageType::WARNING;
            break;
        case ConfirmType::CALCULATION:
            header_line = "----------- CALCULATION SAFETY WARNING -----------";
            header_type = MessageType::WARNING;
            break;
    }
    
    // Display the dialog
    std::cout << "\n";
    print(header_line, header_type);
    print(std::string(title), MessageType::HEADER);
    print("  " + std::string(message), MessageType::NORMAL);
    
    // Display details if provided
    if (!details.empty()) {
        std::cout << "\n";
        print("Details:", MessageType::HEADER);
        print(std::string(details), MessageType::NORMAL);
    }
    
    std::cout << "\n";
    print("Press Enter to confirm, or ESC to cancel: ", MessageType::SYSTEM, false);
    
    // Display tip if provided
    if (!tip.empty()) {
        std::cout << "\n";
        print(std::string(tip), Color::DIM);
    }
    
    // Get user response
    char c = get_keypress();
    bool confirmed = (c == 13 || c == 10); // Enter key (CR or LF)
    
    // Print confirmation status
    if (confirmed) {
        print("Confirmed.", MessageType::SUCCESS);
    } else {
        print("Cancelled.", MessageType::ERROR);
    }
    
    // Footer line
    print(std::string(header_line.length(), '-'), MessageType::WARNING);
    
    return confirmed;
}

// Thread-safe streaming of output
void print_streaming_response(const std::string& chunk, MessageType type) {
    static std::mutex output_mutex;
    std::lock_guard<std::mutex> lock(output_mutex);
    
    std::cout << get_message_color(type) << chunk << RESET_COLOR << std::flush;
}

} // namespace terminal
} // namespace neoneo