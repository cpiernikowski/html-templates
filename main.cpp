#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <iostream>
#include <cassert>
#include <vector>

namespace fs = std::filesystem;
using indentation_level_t = unsigned short;

std::string n_whitespaces(indentation_level_t n) {
    std::string out;
    out.reserve(n);

    while (n--) {
        out.push_back(' ');
    }

    return out;
}

void write_as_html_comment(std::string& b, const std::string& s) {
    b.append("<!-- ");
    b.append(s);
    b.append(" -->");
}


void rewrite_with_filled_templates(const fs::path& path, bool should_write_comments, bool _is_on_bottom_of_stack = true) {
    // if _is_on_bottom_of_stack: will create a new file named t.<p>
    // else (only called this way within this function, with template files): creates temporary files with handled
    // templates within them (templates can have other templates inside), to be used in the creation of the main output file

    assert(path.has_filename()); // should be checked by the caller

    std::ifstream ifs(path);
    ifs >> std::noskipws;
    char current_c;
    std::string new_contents;
    const std::string input_file_parent_path_str = path.parent_path().string() + "/"; 
    constexpr auto OUTPUT_FILE_PREFIX = "t.";
    constexpr auto TEMPORARY_PREFIX = "temp.";
    indentation_level_t indentation_level{};
    static std::vector<fs::path> temporaries_to_delete;

    while (ifs >> current_c) {
        if (current_c == '<') {
        lessthan_encountered:
            ifs >> current_c;

            if (current_c == '@') {
                std::string potential_filename;
                constexpr std::uint8_t ITERS_MAX = 255; // max filename length
                std::remove_const_t<decltype(ITERS_MAX)> iter_counter{};
                bool reached_iters_max = false;

                while (ifs >> current_c) {
                    if (current_c == '@') {
                        ifs >> current_c;

                        if (current_c == '>') {
                            break;
                        }
                        else {
                            potential_filename.push_back('@');
                        }
                    }

                    potential_filename.push_back(current_c);
                    ++iter_counter;

                    if (iter_counter == ITERS_MAX) {
                        reached_iters_max = true;
                        break;
                    }
                }

                if (reached_iters_max) {
                    new_contents.append(potential_filename);
                } else /* found the filename */ {
                    // replace the template in the main file with actual contents of said template
                    // todo: handle tempaltes in the template file recursively
                    rewrite_with_filled_templates(input_file_parent_path_str + potential_filename, should_write_comments, false);
                    
                    std::ifstream ifs_template(input_file_parent_path_str + TEMPORARY_PREFIX + potential_filename);

                    if (!ifs_template.good()) {
                        std::cout << "Fatal error - file " << (input_file_parent_path_str + potential_filename) << " doesn't exist, exiting...";     
                        ifs.close();                 
                        std::exit(EXIT_FAILURE);
                    }

                    const std::string indentation_whitespaces = n_whitespaces(indentation_level);
                    std::string line;

                    if (should_write_comments) {
                        // the comment will be indented appropriately
                        write_as_html_comment(new_contents, potential_filename);
                        new_contents.append("\n");
                    } else {
                        // first line has to be handled separately if the comment isn't written, because the already written whitespace isnt used then
                        // so we will use it now, to indent the first line appropriately
                        
                        std::getline(ifs_template, line);
                        new_contents.append(line);
                        new_contents.push_back('\n');
                    }

                    while (std::getline(ifs_template, line)) {
                        new_contents.append(indentation_whitespaces);
                        new_contents.append(line);
                        new_contents.push_back('\n');
                    }

                    if (should_write_comments) {
                        new_contents.append(indentation_whitespaces);
                        write_as_html_comment(new_contents, "END " + potential_filename);
                    }
                }
            }
            else {
                new_contents.push_back('<');
                new_contents.push_back(current_c);
            }
        }
        else if (current_c == '\n') {
            indentation_level = 0;

            while (ifs >> current_c) {
                if (current_c != ' ') {
                    new_contents.push_back('\n');

                    for (decltype(indentation_level) i = 0; i < indentation_level; ++i) {
                        new_contents.push_back(' ');
                    }

                    if (current_c == '<') {
                        goto lessthan_encountered; // should be completely safe :)
                        break;
                    }

                    new_contents.push_back(current_c);
                    break;
                }

                ++indentation_level;
            }
        }
        else {
            new_contents.push_back(current_c);
        }
    }
    
    if (_is_on_bottom_of_stack) {
        std::ofstream(input_file_parent_path_str + OUTPUT_FILE_PREFIX + path.filename().string()) << new_contents;

        for (auto& f : temporaries_to_delete) {
            fs::remove(f);
        }
    } else {
        const std::string filename = input_file_parent_path_str + TEMPORARY_PREFIX + path.filename().string();
        std::ofstream(filename) << new_contents;
        temporaries_to_delete.push_back(filename);
    }
}

int main(int argc, const char* const* argv) {
    (void)argc;
    (void)argv;
    rewrite_with_filled_templates("./test/index_with_template_inside_template.html", true);

    return EXIT_SUCCESS;
}
