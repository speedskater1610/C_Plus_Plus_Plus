// preprocess.cpp
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>
#include <cstdlib>

std::vector<std::string> preprocess(const std::vector<std::string>& lines) {
    std::vector<std::string> output;
    std::regex func_header_re(R"(^\s*(auto\s+([A-Za-z_]\w*)\s*\([^)]*\))\s*$)");
    const std::string end_template = R"(^\s*end\s+%s\s*$)";

    bool in_func = false;
    std::regex expected_end_re;
    std::string current_func;

    for (const auto& line : lines) {
        if (!in_func) {
            std::smatch m;
            if (std::regex_match(line, m, func_header_re)) {
                // start of function: emit header with opening brace
                std::string header = m[1].str(); // the "auto name(...)" part
                output.push_back(header + " {\n");
                current_func = m[2].str();
                // build expected end regex for this function name
                std::string pattern = "^\\s*end\\s+" + std::regex_replace(current_func, std::regex(R"([.^$|()\\[\]{}*+?])"), R"(\\$&)") + "\\s*$";
                expected_end_re = std::regex(pattern);
                in_func = true;
            } else {
                output.push_back(line);
            }
        } else {
            if (std::regex_match(line, expected_end_re)) {
                output.push_back("}\n");
                in_func = false;
                current_func.clear();
            } else {
                output.push_back(line);
            }
        }
    }

    if (in_func) {
        throw std::runtime_error("Unclosed function: missing matching end " + current_func);
    }
    return output;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: preprocess in.c out.c\n";
        return 1;
    }

    const char* in_path = argv[1];
    const char* out_path = argv[2];

    std::ifstream in(in_path);
    if (!in) {
        std::cerr << "Error: cannot open input file '" << in_path << "'\n";
        return 1;
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        lines.push_back(line + "\n"); // getline strips newline
    }
    in.close();

    std::vector<std::string> out_lines;
    try {
        out_lines = preprocess(lines);
    } catch (const std::exception& e) {
        std::cerr << "Preprocessing error: " << e.what() << "\n";
        return 1;
    }

    std::ofstream out(out_path);
    if (!out) {
        std::cerr << "Error: cannot open output file '" << out_path << "' for writing\n";
        return 1;
    }
    for (const auto& out_line : out_lines) {
        out << out_line;
    }
    out.close();

    return 0;
}
