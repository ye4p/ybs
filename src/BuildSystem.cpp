#include "../include/BuildSystem.hpp"


inline constexpr std::string BUILD_FILE_NAME = "ybs.txt";
inline constexpr std::string BUILD_DIR_NAME = "bs";
inline constexpr std::string INTERMEDIATE_DIR_NAME = "intermediate";
inline constexpr std::string DEPENDENCY_DIR_NAME = "dep";
inline constexpr std::string FULL_INTERMEDIATE_PATH = BUILD_DIR_NAME + '/' + INTERMEDIATE_DIR_NAME;
inline constexpr std::string FULL_DEPENDENCY_PATH = BUILD_DIR_NAME + '/' + DEPENDENCY_DIR_NAME;

inline constexpr std::string COMPILER_NO_LINKING_FLAG = "-c";
inline constexpr std::string COMPILER_NAME_OBJECT_FILE_FLAG = "-o";
inline constexpr std::string COMPILER_DEPENDENCY_GENERATION_FLAG = "-MMD";
inline constexpr std::string COMPILER_NAME_DEPENDENCY_FILE_FLAG = "-MF";

inline constexpr std::string BACKSLASH = "\\";


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
    // check(); // we need to see if files are changed and only compile those (and files that depend on them)
    create_dependency_graph();
    // build();
    // next(); // if has option such as run, do that.
}

int BuildSystem::init() {
    int read_file_status = read_markup_file();
    if (read_file_status) {
        // handle file read fail
        std::cerr << "FAILED READING MARKUP FILE\n";
        return 1;
    }


    // Check if intermediate folder with build files is already created, if not - create it.
    std::filesystem::path dir_path = BUILD_DIR_NAME;

    bool created = std::filesystem::create_directory(dir_path);

    if (created) {
        int dir_init_status = init_build_dir();
        if (!dir_init_status) {
            std::cerr << "Error creating subdirectories\n";
        }
    }


    return 0;
}

int BuildSystem::init_build_dir() {
    std::filesystem::path inter_path = FULL_INTERMEDIATE_PATH;
    std::filesystem::path dep_path = FULL_DEPENDENCY_PATH;

    bool flag = std::filesystem::create_directory(inter_path);
    flag &= std::filesystem::create_directory(dep_path);

    return flag;
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
    files_ = cs_.get_files();

    size_t size = files_.size();

    // for (size_t i = 0; i < files.size(); ++i) {
    //     std::cout << "file: " << files[i] << std::endl;
    // }
    std::vector<int> is_done(size, 0);
    std::vector<int> process_ids(size, 0); // to see which process compiles which file

    long max_processes = sysconf(_SC_CHILD_MAX);

    while (std::accumulate(is_done.begin(), is_done.end(), 0) != static_cast<int>(size)) {
        size_t launched_processes{};
        for (size_t i = 0; i < static_cast<size_t>(max_processes); ++i) {
            if (i >= size) break; // already have enough processes running.
            if (is_done[i]) continue;

            pid_t pid = fork();

            if (pid == 0) 
            {
                // Child process
                std::string str = std::to_string(i);
                const std::string save_to = FULL_INTERMEDIATE_PATH + '/' + str + ".o";
                const std::string dependency_file_name = FULL_DEPENDENCY_PATH + '/' + str + ".d";
                get_object_file(files_[i], save_to, dependency_file_name);

                // only runs if execvp fails
                perror("execvp");
                std::cout << "ERROR?\n";
                _exit(127);

            }
            else if (pid < 0)
            {
                // Fork failed.
                std::cerr << "Fork fail\n";
            }
            else if (pid > 0)
            {
                // Parent process.
                process_ids[i] = static_cast<int>(pid);
                ++launched_processes;
            }
        }
        std::cout << " Waiting for children to finish\n";


        for (size_t i = 0; i < launched_processes; ++i) {
            if (process_ids[i] == 0) continue;
            pid_t pid = waitpid(process_ids[i], nullptr, 0);
            if (pid > 0) {
                // Process has done its job successfully.
                is_done[i] = 1;
                std::cout << "process " << static_cast<int>(pid) << " did its job successfully\n";
            }
            process_ids[i] = 0;
        }
    }

    std::cout << "COMPILED OBJECT FILES SUCCESSFULLY\n";

    return 0;
}

void BuildSystem::get_object_file(const std::string& file, const std::string& save_to, const std::string& dependency_file_name) {
    std::vector<std::string> command = get_command(file, save_to, dependency_file_name);
    std::vector<char*> args = get_argv(command);

    // for (size_t i = 0; i < command.size(); ++i) {
    //     write(STDOUT_FILENO, command[i], strlen(command[i]));
    //     write(STDOUT_FILENO, "\n", 1);
    // }
    execvp(args[0], args.data());
}

std::vector<std::string> BuildSystem::get_command(const std::string& file, const std::string& save_to, const std::string& dependency_file_name) {
    std::vector<std::string> command;
    command.push_back(cs_.get_compiler_name());

    // Generate dependency file
    command.push_back(COMPILER_DEPENDENCY_GENERATION_FLAG);
    command.push_back(COMPILER_NAME_DEPENDENCY_FILE_FLAG);
    command.push_back(dependency_file_name);

    command.push_back(file);

    const std::vector<std::string>& options = cs_.get_compiler_options();
    command.insert(command.end(), options.begin(), options.end());

    command.push_back(COMPILER_NO_LINKING_FLAG);

    command.push_back(COMPILER_NAME_OBJECT_FILE_FLAG);

    // const std::string save_to = BUILD_DIR_NAME + '/' + INTERMEDIATE_DIR_NAME + '/' + pure_name + ".o";
    // std::cout << "save_to : " << save_to;
    command.push_back(save_to);


    std::cout<< "COMMAND: ";
    for (size_t i = 0; i < command.size(); ++i) {
        std::cout << command[i] << ", ";
    }
    std::cout << std::endl;

    return command;
}

std::vector<char*> BuildSystem::get_argv(const std::vector<std::string>& v) {
    std::vector<char*> command(v.size());

    for (size_t i = 0; i < command.size(); ++i) {
        command[i] = const_cast<char*>(v[i].data());
    }
    command.push_back(nullptr);

    return command;
}


int BuildSystem::link() {
    std::filesystem::path p = FULL_INTERMEDIATE_PATH;
    std::filesystem::directory_iterator it(p);

    std::vector<std::string> object_files;
    while (it != std::filesystem::end(it)) {
        object_files.push_back((*it).path().string());
        ++it;
    }

    for (size_t i = 0; i < object_files.size(); ++i) {
        std::cout << object_files[i] << ", ";
    }

    std::vector<std::string> command;
    command.push_back(cs_.get_compiler_name());

    command.insert(command.end(), object_files.begin(), object_files.end());

    command.push_back(COMPILER_NAME_OBJECT_FILE_FLAG);

    command.push_back(cs_.get_exec_name());

    exec_command(get_argv(command));

    return 0;
}

int BuildSystem::exec_command(const std::vector<char*>& args) {

    pid_t pid = fork();
    if (pid == 0) {
        // child
        execvp(args[0], args.data());

        // if runs to this point then an error.
    } else if (pid < 0) {
        perror("fork");
    } else {
        int status;
        wait(&status);
        return WIFEXITED(status) && WEXITSTATUS(status);
    }

    return 0;
}

int BuildSystem::next() {
    if (argc_ == 1 || argv_[1] == "build") {
        return 0;
    }

    if (argv_[1] == "run") {
        std::vector<std::string> command;
        std::string executable = "./" + cs_.get_exec_name();

        command.push_back(executable);

        auto it = argv_.begin();
        ++it;
        ++it;

        if (argc_ > 2) {
            command.insert(command.end(), it,  argv_.end());
        }

        int status = exec_command(get_argv(command));

        return status;
    }

    throw std::logic_error("Not Implemented");
}

int BuildSystem::check() {
    const std::vector<std::string>& files = cs_.get_files();

    // First check if executable exists at all. If it doesn't, keep files as they are.

    std::filesystem::path exec_path = cs_.get_exec_name();
    bool exec_exists = std::filesystem::exists(exec_path);

    if (!exec_exists) {
        files_ = files;
        return 0;
    }

    // Path exists.

    // Now need to loop over all the .cpp files and see if they have been changed after last executable was created.

    struct stat exec_stat{};
    int exec_stat_status = stat(const_cast<char*>(cs_.get_exec_name().data()), &exec_stat);
    (void)exec_stat_status;

    time_t exec_modified_time = exec_stat.st_mtime;


    struct stat file_stat{};
    int count = 0;
    for (size_t i = 0; i < files.size(); ++i) {
        int s = stat(const_cast<char*>(files[i].data()), &file_stat);
        (void)s;
        if (file_stat.st_mtime > exec_modified_time) {
            // was changed, add it.
            files_.push_back(files[i]);
            ++count;
        }
    }

    if (!count) {
        // No changes were made to any of the files.
        std::cout << "No changed detected! No reason to compile\n";
        return 0;
    }

    // Now need to go through dependencies.

    return 0;
}

void BuildSystem::create_dependency_graph() {
    const std::vector<std::string>& files = cs_.get_files();
    std::unordered_map<std::string, std::pair<Node*, size_t>> seen;

    for (size_t i = 0; i < files.size(); ++i) {
        file_deps_.emplace_back(Node(files[i]));
        seen.insert({file_deps_.back().val, {&file_deps_.back(), i}});
    }

    std::cout << "finished filling in file_deps_" << std::endl;

    for (size_t i = 0; i < files.size(); ++i) {
        std::string dep_path = FULL_DEPENDENCY_PATH + '/' + std::to_string(i) + ".d";

        std::ifstream in(dep_path);

        if (!in.is_open()) {
            std::cerr << "error opening a file " << dep_path << std::endl;
            return;
        }
        // Need to normalize paths for them, so that src/../include doesn't happen.
        std::string itself;
        in >> itself; // object file
        in >> itself; // itself
        auto itself_it = seen.find(itself);
        if (itself_it == seen.end()) {
            std::cerr << "Didn't find file with the name <" << itself << '>' << std::endl;
            return;
        }
        size_t ind = itself_it->second.second;

        std::cout << "FOR THE FILE: " << itself << std::endl;
        std::string w;
        while (in >> w) {
            if (w == BACKSLASH) {
                continue;
            }
            std::cout << "depends on file " << w << std::endl;
            std::string normalized = normalize_path(w); // check if moving vs nrvo better
            std::cout << "file normalized " << normalized << std::endl;
            auto it = seen.find(normalized);
            if (it == seen.end()) {
                file_deps_.emplace_back(Node(normalized));
                seen[file_deps_.back().val] = {&file_deps_.back(), file_deps_.size() - 1};
            }
            Node* ptr = &file_deps_.back();

            file_deps_[ind].deps.push_back(ptr);
        }

    }

    for (size_t i = 0; i < file_deps_.size(); ++i) {
        std::cout << "File " << file_deps_[i].val << " depends on: ";
        for (size_t j = 0; j < file_deps_[i].deps.size(); ++i) {
            std::cout << file_deps_[i].deps[j]->val << ", ";
        }
        std::cout << '\n';
    }
}

std::string BuildSystem::normalize_path(const std::string& path) {
    std::vector<std::string> s;
    size_t i = 0;
    std::string curr;
    while (i < path.size()) {
        curr = parse_token(i, path); // check if move vs nrvo is better
        ++i;
        if (curr == "..") {
            s.pop_back();
            continue;
        } else if (curr == ".") {
            continue;
        } else {
            s.push_back(curr);
        }
    }

    std::string res;
    for (size_t i = 0; i < s.size(); ++i) {
        res += s[i];
        res += '/';
    }
    res.pop_back();

    return res;
}

std::string BuildSystem::parse_token(size_t& i, const std::string& path) {
    size_t beg = i;
    while (i < path.size() && path[i] != '/') { ++i; }

    return path.substr(beg, i - beg);
}

bool BuildSystem::is_header_file(const std::string& path) const {
    return path.ends_with(".h") || path.ends_with(".hpp");
}

//
//      Compilation Settings class
//

CompilationSettings::CompilationSettings() {}

CompilationSettings::CompilationSettings(const std::string& in) {
    int i = 0;

    compiler_ = parse_word(in, i, false);


    while (in[i] != '[') { ++i; }

    do {
        ++i;
        files_.emplace_back(parse_word(in, i, true));
    } while (in[i] != ']');

    while (in[i] != '[') { ++i; }

    do {
        ++i;
        compiler_options_.emplace_back(parse_word(in, i, true));
    } while (in[i] != ']');

    ++i;
    while (in[i] == ' ') { ++i; }

    exec_name_ = parse_word(in, i, false);

    clean_names();
}

std::string CompilationSettings::parse_word(const std::string& in, int& i, bool check_bracket) {
    int beg = i;
    while (in[i] != ' ' && (check_bracket ? in[i] != ']' : true)) { ++i; }

    return in.substr(beg, i - beg);
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

const std::vector<std::string>& CompilationSettings::get_files() const {
    return files_;
}

const std::string& CompilationSettings::get_compiler_name() const {
    return compiler_;
}

const std::string& CompilationSettings::get_exec_name() const {
    return exec_name_;
}

const std::vector<std::string>& CompilationSettings::get_compiler_options() const {
    return compiler_options_;
}

const std::vector<std::string>& CompilationSettings::get_pure_file_names() const {
    return pure_file_names_;
}

// Helper methods.

const std::string CompilationSettings::get_pure_file_name(const std::string& file) const {
    size_t last_slash = 0;
    size_t last_dot = 0;
    for (size_t i = 0; i < file.size(); ++i) {
        if (file[i] == '/') {
            last_slash = i;
            continue;
        }

        if (file[i] == '.') {
            last_dot = i;
            continue;
        }
    }

    return file.substr(last_slash + 1, last_dot - last_slash - 1);
}

void CompilationSettings::clean_names() {
    for (size_t i = 0; i < files_.size(); ++i) {
        pure_file_names_.emplace_back(get_pure_file_name(files_[i]));
    }
}
