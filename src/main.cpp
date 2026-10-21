#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace {

constexpr const char* kDataFile = "tasks.txt";
constexpr std::size_t kMaxTitle = 120;

struct Task {
    std::string title;
    bool done = false;
};

std::string trim(std::string text) {
    const auto notSpace = [](unsigned char ch) { return !std::isspace(ch); };
    text.erase(text.begin(), std::find_if(text.begin(), text.end(), notSpace));
    text.erase(std::find_if(text.rbegin(), text.rend(), notSpace).base(), text.end());
    return text;
}

std::optional<std::string> ask(const std::string& prompt) {
    std::cout << prompt << std::flush;
    std::string line;
    if (!std::getline(std::cin, line)) {
        std::cout << '\n';
        return std::nullopt;
    }
    return trim(std::move(line));
}

bool toNumber(const std::string& text, int& value) {
    const char* begin = text.data();
    const char* end = begin + text.size();
    const auto result = std::from_chars(begin, end, value);
    return result.ec == std::errc() && result.ptr == end;
}

std::vector<Task> loadTasks() {
    std::vector<Task> tasks;
    std::ifstream input(kDataFile);
    if (!input) {
        return tasks;
    }

    std::string line;
    while (std::getline(input, line)) {
        line = trim(std::move(line));
        if (line.empty()) {
            continue;
        }
        if (line.size() < 2 || (line[0] != '0' && line[0] != '1') || line[1] != '|') {
            std::cout << "Ignored invalid line: " << line << '\n';
            continue;
        }

        std::string title = trim(line.substr(2));
        if (title.empty()) {
            std::cout << "Ignored invalid line: " << line << '\n';
            continue;
        }
        tasks.push_back({std::move(title), line[0] == '1'});
    }
    return tasks;
}

bool saveTasks(const std::vector<Task>& tasks) {
    std::ofstream output(kDataFile, std::ios::trunc);
    if (!output) {
        std::cerr << "Could not open " << kDataFile << " for writing.\n";
        return false;
    }

    for (const Task& task : tasks) {
        output << (task.done ? '1' : '0') << '|' << task.title << '\n';
    }
    if (!output) {
        std::cerr << "Failed to save tasks.\n";
        return false;
    }
    return true;
}

void printMenu() {
    std::cout
        << "================================\n"
        << "       SMART TASK MANAGER\n"
        << "================================\n"
        << "1. Add task\n"
        << "2. List tasks\n"
        << "3. Complete task\n"
        << "4. Delete task\n"
        << "5. Save and exit\n"
        << "================================\n";
}

void addTask(std::vector<Task>& tasks) {
    const auto title = ask("Task title: ");
    if (!title) {
        std::cout << "Input closed. Task not added.\n";
        return;
    }
    if (title->empty()) {
        std::cout << "Task title cannot be empty.\n";
        return;
    }
    if (title->size() > kMaxTitle) {
        std::cout << "Task title is too long (maximum " << kMaxTitle << " characters).\n";
        return;
    }

    tasks.push_back({*title, false});
    std::cout << "Task added.\n";
}

void listTasks(const std::vector<Task>& tasks) {
    if (tasks.empty()) {
        std::cout << "No tasks yet. Add one with option 1.\n";
        return;
    }

    for (std::size_t i = 0; i < tasks.size(); ++i) {
        std::cout << ' ' << (i + 1) << ". [" << (tasks[i].done ? 'x' : ' ') << "] " << tasks[i].title << '\n';
    }

    const auto done = std::count_if(tasks.begin(), tasks.end(), [](const Task& task) { return task.done; });
    std::cout << "Completed: " << done << " of " << tasks.size() << '\n';
}

bool pickTask(const std::vector<Task>& tasks, std::size_t& index) {
    const auto line = ask("Task number: ");
    if (!line) {
        std::cout << "Input closed.\n";
        return false;
    }

    int number = 0;
    if (!toNumber(*line, number) || number < 1 || static_cast<std::size_t>(number) > tasks.size()) {
        std::cout << "Invalid task number.\n";
        return false;
    }

    index = static_cast<std::size_t>(number - 1);
    return true;
}

void completeTask(std::vector<Task>& tasks) {
    if (tasks.empty()) {
        std::cout << "No tasks to complete.\n";
        return;
    }

    std::size_t index = 0;
    if (!pickTask(tasks, index)) {
        return;
    }

    Task& task = tasks[index];
    if (task.done) {
        std::cout << '"' << task.title << "\" is already completed.\n";
        return;
    }

    task.done = true;
    std::cout << "Completed \"" << task.title << "\".\n";
}

void deleteTask(std::vector<Task>& tasks) {
    if (tasks.empty()) {
        std::cout << "No tasks to delete.\n";
        return;
    }

    std::size_t index = 0;
    if (!pickTask(tasks, index)) {
        return;
    }

    std::cout << "Deleted \"" << tasks[index].title << "\".\n";
    tasks.erase(tasks.begin() + static_cast<std::ptrdiff_t>(index));
}

}

int main() {
    std::vector<Task> tasks = loadTasks();
    bool running = true;

    while (running && std::cin) {
        printMenu();

        const auto line = ask("Select an option: ");
        if (!line) {
            std::cout << "Input closed.\n";
            break;
        }

        int choice = 0;
        if (!toNumber(*line, choice) || choice < 1 || choice > 5) {
            std::cout << "Invalid option. Enter a number from 1 to 5.\n\n";
            continue;
        }

        switch (choice) {
        case 1:
            addTask(tasks);
            break;
        case 2:
            listTasks(tasks);
            break;
        case 3:
            completeTask(tasks);
            break;
        case 4:
            deleteTask(tasks);
            break;
        case 5:
            running = false;
            break;
        }
        std::cout << '\n';
    }

    if (!saveTasks(tasks)) {
        return 1;
    }
    std::cout << "Tasks saved to " << kDataFile << ".\n";
    return 0;
}
