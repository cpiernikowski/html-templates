#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <iostream>
#include <cassert>

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

void write_as_html_comment_newline(std::string& b, const std::string& s) {
    b.append("<!-- ");
    b.append(s);
    b.append(" -->\n");
}


void rewrite_with_filled_templates(const fs::path& path, bool should_write_comments, bool _is_on_bottom_of_stack = true) {
    // if _is_on_bottom_of_stack: will create a new file named t.<p>
    // else (only called this way within this function, with template files): creates temporary files with handled
    // templates within them (templates can have other templates inside), to be used in the creation of the main output file

    assert(path.has_filename()); // should be checked by the caller
    (void)_is_on_bottom_of_stack;

    std::ifstream ifs(path);
    ifs >> std::noskipws;
    char current_c;
    std::string new_contents;
    const std::string input_file_parent_path_str = path.parent_path().string() + "/"; 
    constexpr auto OUTPUT_FILE_PREFIX = "t.";
    indentation_level_t indentation_level{};

    while (ifs >> current_c) {
        if (current_c == '<') {
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
                    std::ifstream ifs_template(input_file_parent_path_str + potential_filename);
                    const std::string indentation_whitespaces = n_whitespaces(indentation_level);
                    std::string line;

                    if (should_write_comments) {
                        // the comment will be indented appropriately
                        write_as_html_comment_newline(new_contents, potential_filename);
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
                        write_as_html_comment_newline(new_contents, "END " + potential_filename);
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

    std::ofstream ofs(input_file_parent_path_str + OUTPUT_FILE_PREFIX + path.filename().string());
    ofs << new_contents;
}

int main(int argc, const char* const* argv) {
    (void)argc;
    (void)argv;
    rewrite_with_filled_templates("./test/index.html", true);

    return EXIT_SUCCESS;
}