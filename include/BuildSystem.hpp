#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <numeric>

#include <unistd.h> // for posix stuff
#include <sys/wait.h>
#include <cstring>


class CompilationSettings {
public:
    CompilationSettings();
    CompilationSettings(const std::string& in);
    std::string parse_word(const std::string& in, int& i, bool check_bracket);
    void print();

    const std::string& get_compiler_name() const;
    const std::vector<std::string>& get_files() const;
    const std::vector<std::string>& get_compiler_options() const;
    const std::string& get_exec_name() const;
 
    const std::vector<std::string>& get_pure_file_names() const;
    // helpers
    const std::string get_pure_file_name(const std::string& file) const;
    void clean_names();
private:
    std::string compiler_;
    std::vector<std::string> files_;
    std::vector<std::string> compiler_options_;
    std::string exec_name_;

    std::vector<std::string> pure_file_names_;
};


class BuildSystem {
public:
    BuildSystem(int argc, char** argv);
    void run();
    int init();
    int init_build_dir();
    int read_markup_file();
    int build();
    int compile();
    int link();

    void get_object_file(const std::string& file, const std::string& save_to);
    std::vector<char*> get_command(const std::string& file, const std::string& save_to);

    // Helpers
    // const std::string save_to get_save_to_path(const std::string
private:
    std::vector<std::string> argv_;
    size_t argc_;

    CompilationSettings cs_;
};

