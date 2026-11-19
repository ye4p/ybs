#include "../include/BuildSystem.hpp"


inline constexpr std::string_view BUILD_FILE_NAME = "ybs.txt";


BuildSystem::BuildSystem(int argc, char** argv) : argc_(argc) {
    for (char* it = *argv; it != nullptr; ++it) {
        argv_.emplace_back(it);
    }
}

int BuildSystem::read_markup_file() {
    std::ifstream in(BUILD_FILE_NAME);

    if (!in.is_open()) {
        // handle file not open
        perror("Couldn't open a file");
        return 1;
    }

    std::string line;

    std::getline(in, line);

    
}

int BuildSystem::build() {
    int compile_status = compile();

    if (compile_status) {
        // Handle an error.

    }

    int linking_status = link();

    if (linking_status) {
        // Handle a linking error.

    }

    return 0;
}



//
//      Compilation Settings class
//

CompilationSettings::CompilationSettings(const std::string& in) {
    int i = 0;
    int beg = 0;
    int end = 0;

    compiler_ = parse_word(in, i);


    while (i < in.size()) {
        
    }
}

std::string parse_word(const std::string& in, int& i) {
    int beg = i;
    while (in[i] != ' ') { ++i; }

    return in.substr(beg, i++ - beg + 1);
}
