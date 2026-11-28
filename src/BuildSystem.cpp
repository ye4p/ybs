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

inline constexpr std::string BUILD_DATA_FILE_PATH = "bs/temp.txt";


BuildSystem::BuildSystem(int argc, char** argv) : argc_(argc) {
    for (char** it = argv; *it != nullptr; ++it) {
        argv_.emplace_back(*it);
    }

    int init_status = init();
    if (init_status) {
        std::runtime_error("Failed initialization\n");
    }

    std::cout << "initialized successfully" << std::endl;
}

void BuildSystem::run() {
    //auto start = std::chrono::steady_clock::now();

    check();
    //std::thread worker(&BuildSystem::check, this); // we need to see if files are changed and only compile those (and files that depend on them)
    create_dependency_graph();
    // worker.join();


    // now we need to look at both dependencies vector and at the files_ to see which files need to be recompiled.
    fill_dependencies();

    display_files_to_compile();

    //auto end = std::chrono::steady_clock::now();
    //std::chrono::duration<double> elapsed = end - start;
    //std::cout << elapsed.count() << " seconds\n";
    build();
    next(); // if has option such as run, do that.
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
    if (files_.empty()) {
        return 0;
    }
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
    const std::vector<std::string> all_src_files = cs_.get_files();
    size_t size = files_.size();

    std::vector<int> is_done(size, 0);
    std::vector<int> process_ids(size, 0); // to see which process compiles which file

    long max_processes = std::thread::hardware_concurrency();
    std::cout << "maximum processes: " << max_processes << std::endl;
    // UP NEXT:
    // make smarter compilation system, so that all available processes remain busy at all times.

    std::queue<size_t> q;

    auto sum = [&](){
        return std::accumulate(is_done.begin(), is_done.end(), 0);
    };

    auto old_idx = [&](size_t i__) {
        for (size_t j = 0; j < all_src_files.size(); ++j) {
            if (files_[i__] == all_src_files[j]) {
                return j;
            }
        }
        return ULONG_MAX;
    };
    size_t done_counter = 0;

    while (done_counter != size) {
        std::cout << "done " << sum() << " out of " << size << std::endl;
        size_t launched_processes{};
        size_t i = 0;

        auto launch_process = [&](){
            pid_t pid = fork();

            if (pid == 0) 
            {
                // Child process
                // need to get index of a file of how it would be from cs_.get_files() so that enumeration of object files is consistent
                std::string str = std::to_string(old_idx(i));
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
                std::cout << "started " << i  << " with id << " << pid << std::endl;
                ++launched_processes;
                q.push(i);
                ++i;
            }
        };

        while (i < size && launched_processes < static_cast<size_t>(max_processes)) {
            if (i >= size) break; // already have enough processes running.
            if (is_done[i] || process_ids[i]) { // this proces is running or already executed
                ++i;
                continue;
            }

            launch_process();
        }


        while (launched_processes) {
        // for (size_t i = 0; i < launched_processes; ++i) {
            size_t idx = q.front();
            q.pop();
            if (process_ids[idx] == 0) continue;
            pid_t pid = waitpid(process_ids[idx], nullptr, 0);
            if (pid > 0) {
                // Process has done its job successfully.
                is_done[idx] = 1;
                ++done_counter;
                std::cout << "idx " << i << " process " << static_cast<int>(pid) << " did its job successfully\n";
            }
            process_ids[idx] = 0;

            --launched_processes;
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

void BuildSystem::rebuild_all() {
    files_ = cs_.get_files();
}

int BuildSystem::check() {
    const std::vector<std::string>& files = cs_.get_files();

    // First check if executable exists at all. If it doesn't, keep files as they are.

    std::filesystem::path exec_path = cs_.get_exec_name();
    bool exec_exists = std::filesystem::exists(exec_path);

    if (!exec_exists) {
        rebuild_all();
        return 0;
    }

    // Path exists.

    // Now check build_data_file

    int read_status = read_build_data_file();

    if (read_status) {
        rebuild_all();
        std::cout << "REBUILDING DATA FILE\n";
        update_build_data_file();
        return 0;
    }

    // Now need to loop over all the .cpp files and see if they have been changed after last executable was created.

    struct stat exec_stat{};
    stat(const_cast<char*>(cs_.get_exec_name().data()), &exec_stat);

    time_t exec_modified_time = exec_stat.st_mtime;


    struct stat file_stat{};
    int count = 0;
    for (size_t i = 0; i < files.size(); ++i) {
        int s = stat(const_cast<char*>(files[i].data()), &file_stat);

        if (s) {
            rebuild_all();
            return 0;
        }

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

int BuildSystem::read_build_data_file() {
    std::ifstream in(BUILD_DATA_FILE_PATH);

    if (!in.is_open()) {
        return 1;
    }

    std::vector<std::string> old_flags;
    std::string str;
    while (in >> str) {
        old_flags.push_back(str);
    }

    return old_flags != cs_.get_compiler_options();
}

void BuildSystem::update_build_data_file() {
    std::ofstream out(BUILD_DATA_FILE_PATH);

    if (!out.is_open()) {
        std::cerr << "Error creating build data file\n";
        return;
    }

    std::string res;
    for (const std::string& s : cs_.get_compiler_options()) {
        res += s;
        res += ' ';
    }

    out << res;
}

void BuildSystem::create_dependency_graph() {
    if (files_.empty()) return;
    const std::vector<std::string>& files = cs_.get_files();

    for (size_t i = 0; i < files.size(); ++i) {
        file_deps_.emplace_back(Node(files[i]));
        std::cout << "file_deps_.back().val: " << file_deps_.back().val << std::endl;
        dep_idxs_.insert({file_deps_.back().val, i});
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
        auto itself_it = dep_idxs_.find(itself);
        if (itself_it == dep_idxs_.end()) {
            std::cerr << "Didn't find file with the name <" << itself << '>' << std::endl;
            return;
        }
        size_t ind = itself_it->second;
        std::cout << "FOR THE FILE: " << itself << " and its index " << ind << std::endl;
        std::string w;
        while (in >> w) {
            if (w == BACKSLASH) {
                continue; 
            }
            std::cout << "depends on file " << w << std::endl;
            std::string normalized = normalize_path(w);
            std::cout << "file normalized " << normalized << std::endl;
            auto it = dep_idxs_.find(normalized);
            if (it == dep_idxs_.end()) {
                std::cout << "NOT FOUND " << normalized << std::endl;
                file_deps_.emplace_back(normalized);
                dep_idxs_.insert({file_deps_.back().val, file_deps_.size() - 1});
            }
            auto new_it = dep_idxs_.find(normalized);

            if (new_it == dep_idxs_.end()) {
                std::cerr << "ERROR: didn't find the file " << normalized << std::endl;
                return;
            }
            // Dangling pointer. file_deps_ could have realocated, but pointer still points there.

            file_deps_[ind].deps.push_back(new_it->second);
        }

    }

    display_dependencies();

    match_headers();

    display_dependencies();
}

void BuildSystem::match_headers() {
    std::unordered_map<size_t, size_t> header_to_source;

    for (size_t i = 0; i < file_deps_.size(); ++i) {
        const std::string& header = file_deps_[i].val;
        if (!is_header_file(header)) continue;

        auto header_it = dep_idxs_.find(header);

        std::string source = header_to_source_string(header);

        auto it = dep_idxs_.find(source);
        if (it == dep_idxs_.end()) continue;

        header_to_source.insert({header_it->second, it->second});
    }

    for (const auto& [header_idx, source_idx] : header_to_source) {
        for (size_t i = 0; i < file_deps_.size(); ++i) {
            for (size_t j = 0; j < file_deps_[i].deps.size(); ++j) {
                if (file_deps_[i].deps[j] != header_idx) continue;

                file_deps_[i].deps.push_back(source_idx);
                std::cout << "pushed " << file_deps_[source_idx].val << " into " << file_deps_[i].val << std::endl;
            }
        }
    }
}

void BuildSystem::display_dependencies() const {
    for (size_t i = 0; i < file_deps_.size(); ++i) {
        std::cout << "File " << file_deps_[i].val << " depends on: ";
        for (size_t j = 0; j < file_deps_[i].deps.size(); ++j) {
            std::cout << file_deps_[file_deps_[i].deps[j]].val << ", ";
        }
        std::cout << '\n';
    }
}

void BuildSystem::display_files_to_compile() const {
    std::cout << "FILES TO COMPILE: \n";
    for (size_t i = 0; i < files_.size(); ++i) {
        std::cout << files_[i] << ", ";
    }
    std::cout << "DONE\n";
}

std::string BuildSystem::normalize_path(const std::string& path) {
    std::vector<std::string> s;
    size_t i = 0;

    while (i < path.size()) {
        std::string curr = parse_token(i, path);
        ++i;
        if (curr == "..") {
            s.pop_back();
            continue;
        } else if (curr == ".") {
            continue;
        } else {
            s.push_back(std::move(curr));
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

std::string BuildSystem::header_to_source_string(const std::string& path) const {
    int forward_slash_idx = -1;
    for (int i = path.size() - 1; i > 0; --i) {
        if (path[i] == '/') {
            forward_slash_idx = i;
            break;
        }
    }

    if (forward_slash_idx <= 0) {
        return change_ending(path);
    } else {
        std::string ending = change_ending(path.substr(forward_slash_idx));
        for (size_t i = 0; i < file_deps_.size(); ++i) {
            if (file_deps_[i].val.ends_with(ending)) {
                return file_deps_[i].val;
            }
        }

        std::string err = "ERROR: no matching source file was found from a header file " + path;
        throw std::runtime_error(err);
    }
}

std::string BuildSystem::change_ending(const std::string& path) const {
    const std::string ending = cs_.get_compiler_name().ends_with("++") ? "cpp" : "c";

    size_t dot_idx = 0;

    for (size_t i = path.size() - 1; i > 1; --i) {
        if (path[i] == '.') {
            dot_idx = i;
            break;
        }
    }

    if (dot_idx == 0) {
        throw std::runtime_error("File extension not found > " + path + '\n');;
    }

    return path.substr(0, dot_idx + 1) + ending;
}


void BuildSystem::fill_dependencies() {
    size_t changed_files = files_.size();

    for (size_t i = 0; i < changed_files; ++i) {
        add_files_that_depend_on(files_[i]);
    }
}

void BuildSystem::add_files_that_depend_on(const std::string path) {
    for (size_t i = 0; i < file_deps_.size(); ++i) {
        for (size_t j = 0; j < file_deps_[i].deps.size(); ++j) {
            if (is_header_file(file_deps_[i].val)) break;

            const size_t ind = file_deps_[i].deps[j];

            if (file_deps_[ind].val == path && !is_in_files(file_deps_[i].val)) {
                files_.push_back(file_deps_[i].val);
                break;
            }
        }
    }
}

bool BuildSystem::is_in_files(const std::string& path) const {
    const std::vector<std::string>& fs = files_;
    return std::find(fs.begin(), fs.end(), path) != fs.end();
}



//
//      Compilation Settings class
//

CompilationSettings::CompilationSettings() {}

CompilationSettings::CompilationSettings(const std::string& in) {
    size_t i = 0;

    compiler_ = parse_word(in, i, false);

    while (in[i] != '[') { ++i; }

    ++i;
    do {
        files_.emplace_back(parse_word(in, i, true));
        skip_whitespace(in, i);
    } while (in[i] != ']');

    while (in[i] != '[') { ++i; }

    ++i;
    if (in[i] != ']') {
        do {
            compiler_options_.emplace_back(parse_word(in, i, true));
            skip_whitespace(in, i);
        } while (in[i] != ']');
    }

    ++i;
    while (in[i] == ' ') { ++i; }

    exec_name_ = parse_word(in, i, false);

    clean_names();
}

void CompilationSettings::skip_whitespace(const std::string& in, size_t &i) {
    while (in[i] == ' ') { ++i; }
}

std::string CompilationSettings::parse_word(const std::string& in, size_t& i, bool check_bracket) {
    int beg = i;
    while (i < in.size() && in[i] != ' ' && (check_bracket ? in[i] != ']' : true)) { ++i; }

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
