#include "../include/BuildSystem.hpp"


inline constexpr std::string BUILD_FILE_NAME = "ybs.txt";


BuildSystem::BuildSystem(int argc, char** argv) : argc_(argc) {
    for (char** it = argv; *it != nullptr; ++it) {
        argv_.emplace_back(*it);
    }

    int init_status = init();
    if (init_status) {
        // handle failed initialization
    }

    std::cout << "initialized successfully" << std::endl;
}

void BuildSystem::run() {

}

int BuildSystem::init() {
    int read_file_status = read_markup_file();
    if (read_file_status) {
        // handle file read fail
        return 1;
    }

    return 0;
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

    cs_ = CompilationSettings(line);
    cs_.print();

    return 0;
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

int BuildSystem::compile() {

    return 0;
}

int BuildSystem::link() {

    return 0;
}



//
//      Compilation Settings class
//

CompilationSettings::CompilationSettings() {}

CompilationSettings::CompilationSettings(const std::string& in) {
    int i = 0;

    compiler_ = parse_word(in, i, false);


    print();
    while (in[i++] != '[') {}

    while(in[i] != ']') {
        files_.emplace_back(parse_word(in, i, true));
        print();
    }
    print();

    while (in[i++] != '[') {}

    while (in[i] != ']') {
        compiler_options_.emplace_back(parse_word(in, i, true));
        ++i;
    }
    print();

    exec_name_ = parse_word(in, i, false);
}

std::string CompilationSettings::parse_word(const std::string& in, int& i, bool check_bracket) {
    int beg = i;
    while (in[i] != ' ' && (check_bracket ? in[i] != ']' : true)) { ++i; }

    return in.substr(beg, i++ - beg);
}

void CompilationSettings::print() {
    std::cout << "Using compiler " << compiler_ << '\n';
    std::cout << "Compiling files: ";
    for (const auto& f : files_) {
        std::cout << f << ", ";
    }
    std::cout << "\nWith compiler options: ";
    for (const auto& co : compiler_options_) {
        std::cout << co << ", ";
    }

    std::cout << "\nInto an executable named " << exec_name_ << '\n';
}
