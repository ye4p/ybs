#include <iostream>
#include <string>
#include <vector>
#include <fstream>

class BuildSystem {
public:
    BuildSystem(int argc, char** argv);
    void run();
    int read_markup_file();
    int build();
    int compile();
    int link();
private:
    std::vector<std::string> argv_;
    size_t argc_;
};

class CompilationSettings {
public:
    CompilationSettings(const std::string& in);
    std::string parse_word(const std::string& in, int& i);
private:
    std::string compiler_;
    std::vector<std::string> files_;
    std::vector<std::String> compiler_options_;
    std::string exec_name_;
};
