#include <iostream>
#include <string>
#include <vector>
#include <fstream>


class CompilationSettings {
public:
    CompilationSettings();
    CompilationSettings(const std::string& in);
    std::string parse_word(const std::string& in, int& i, bool check_bracket);
    void print();
private:
    std::string compiler_;
    std::vector<std::string> files_;
    std::vector<std::string> compiler_options_;
    std::string exec_name_;
};


class BuildSystem {
public:
    BuildSystem(int argc, char** argv);
    void run();
    int init();
    int read_markup_file();
    int build();
    int compile();
    int link();
private:
    std::vector<std::string> argv_;
    size_t argc_;

    CompilationSettings cs_;
};

