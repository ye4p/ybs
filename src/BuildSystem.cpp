#include "../include/BuildSystem.hpp"


inline constexpr std::string BUILD_FILE_NAME = "ybs.txt";
inline constexpr std::string BUILD_DIR_NAME = "bs";
inline constexpr std::string INTERMEDIATE_DIR_NAME = "intermediate";
inline constexpr std::string FULL_INTERMEDIATE_PATH = BUILD_DIR_NAME + '/' + INTERMEDIATE_DIR_NAME;

inline constexpr std::string GET_OBJECT_TAG = "-c";
inline constexpr std::string RENAME_TAG = "-o";


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
    build();
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

    bool exists = std::filesystem::create_directory(dir_path);

    if (!exists) {
        init_build_dir();
    }


    return 0;
}

int BuildSystem::init_build_dir() {
    std::filesystem::path dir_path = FULL_INTERMEDIATE_PATH;
    bool flag = std::filesystem::create_directory(dir_path);

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
    const std::vector<std::string>& files = cs_.get_files();
    // for (size_t i = 0; i < files.size(); ++i) {
    //     std::cout << "file: " << files[i] << std::endl;
    // }
    std::vector<int> is_done(files.size(), 0);
    std::vector<int> process_ids(files.size(), 0); // to see which process compiles which file

    long max_processes = sysconf(_SC_CHILD_MAX);
    (void)max_processes;

    while (std::accumulate(is_done.begin(), is_done.end(), 0) != static_cast<int>(files.size())) {
        for (size_t i = 0; i < files.size(); ++i) {
            if (is_done[i]) continue;

            pid_t pid = fork();

            if (pid == 0) 
            {
                // Child process
                std::string str = std::to_string(i);
                const std::string save_to = BUILD_DIR_NAME + '/' + INTERMEDIATE_DIR_NAME + '/' + str + ".o";
                get_object_file(files[i], save_to);

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
            }
        }
        std::cout << " Waiting for children to finish\n";


        for (size_t i = 0; i < files.size(); ++i) {
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

void BuildSystem::get_object_file(const std::string& file, const std::string& save_to) {
    std::vector<std::string> command = get_command(file, save_to);
    std::vector<char*> args = get_argv(command);

    // for (size_t i = 0; i < command.size(); ++i) {
    //     write(STDOUT_FILENO, command[i], strlen(command[i]));
    //     write(STDOUT_FILENO, "\n", 1);
    // }
    execvp(args[0], args.data());
}

std::vector<std::string> BuildSystem::get_command(const std::string& file, const std::string& save_to) {
    std::vector<std::string> command;
    command.push_back(cs_.get_compiler_name());

    command.push_back(file);

    const std::vector<std::string>& options = cs_.get_compiler_options();
    command.insert(command.end(), options.begin(), options.end());

    command.push_back(GET_OBJECT_TAG);

    command.push_back(RENAME_TAG);

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

    command.push_back(RENAME_TAG);

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
