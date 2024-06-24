#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <iostream>
#include <cassert>
#include <vector>
#include <unordered_map>

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

void exit_if_file_doesnt_exist(const std::string& filename) noexcept {
    std::FILE* file = std::fopen(filename.c_str(), "r");
    if (file == nullptr) { // faster than using fstream
        std::cout << "Fatal error - file \"" << filename << "\" doesn't exist, exiting...";
        std::exit(EXIT_FAILURE);
    }
    std::fclose(file);
}

static std::unordered_map<std::string, std::string> temporaries;

template <bool _is_on_bottom_of_stack = true>
void rewrite_with_filled_templates(const fs::path& path, bool should_write_comments) {
    // if _is_on_bottom_of_stack: will create a new file named t.<p>
    // else (only called this way within this function, with template files): creates temporary buffers with handled
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
                    const std::string potential_filename_with_dir = input_file_parent_path_str + potential_filename;
                    exit_if_file_doesnt_exist(potential_filename_with_dir);

                    // handle templates recursively - if _is_on_bottom_of_stack, the new contents will be written to a new file,
                    // else - they will be written to the temporaries map
                    // ifs at the bottom of this function handle this behaviour
                    rewrite_with_filled_templates<false>(potential_filename_with_dir, should_write_comments);
                    
                    std::istringstream stream_template_contents(temporaries.at(input_file_parent_path_str + TEMPORARY_PREFIX + potential_filename));
                    const std::string indentation_whitespaces = n_whitespaces(indentation_level);
                    std::string line;

                    if (should_write_comments) {
                        // the comment will be indented appropriately
                        write_as_html_comment(new_contents, potential_filename);
                        new_contents.append("\n");
                    } else {
                        // first line has to be handled separately if the comment isn't written, because the already written whitespace isnt used then
                        // so we will use it now, to indent the first line appropriately
                        std::getline(stream_template_contents, line);
                        new_contents.append(line);
                        new_contents.push_back('\n');
                    }

                    while (std::getline(stream_template_contents, line)) {
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
    
    if constexpr (_is_on_bottom_of_stack) {
        // create the main file
        std::ofstream(input_file_parent_path_str + OUTPUT_FILE_PREFIX + path.filename().string()) << new_contents;
    } else {
        // create buffers for templates inside templates, which are not meant to be written to the disk,
        // we only want to write the main file to the disk
        temporaries[input_file_parent_path_str + TEMPORARY_PREFIX + path.filename().string()] = new_contents;
    }
}

int main(int argc, const char* const* argv) {
    (void)argc;
    (void)argv;
    std::cout << argc;
    rewrite_with_filled_templates("./test/index_with_template_inside_template.html", true);

    return EXIT_SUCCESS;
}
