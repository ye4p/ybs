#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <numeric>
#include <utility>
#include <unordered_map>

#include <unistd.h> // for posix stuff
#include <sys/wait.h>
#include <sys/stat.h>
#include <cstdlib>
#include <time.h>
#include <cstring>


struct Node {
    std::string val;
    std::vector<size_t> deps;

    // Node() {}
    explicit Node(const std::string& v) : val(v) {}
};

class CompilationSettings {
public:
    CompilationSettings();
    CompilationSettings(const std::string& in);
    std::string parse_word(const std::string& in, size_t& i, bool check_bracket);
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

    int next();
    int check();
    void create_dependency_graph();
    void match_headers();
    void display_dependencies() const;

    std::string normalize_path(const std::string& path);
    std::string parse_token(size_t &i, const std::string& path);
    bool is_header_file(const std::string& path) const;

    void get_object_file(const std::string& file, const std::string& save_to, const std::string& dependency_file_name);
    std::vector<std::string> get_command(const std::string& file, const std::string& save_to, const std::string& dependency_file_name);
    std::vector<char*> get_argv(const std::vector<std::string>& v);
    int exec_command(const std::vector<char*>& args);

    // Helpers
    // const std::string save_to get_save_to_path(const std::string
private:
    std::vector<std::string> argv_;
    size_t argc_;

    std::vector<std::string> files_;

    CompilationSettings cs_;

    std::vector<Node> file_deps_;
    std::unordered_map<std::string, size_t> dep_idxs_;
};

