// #define CPPHTTPLIB_OPENSSL_SUPPORT
#include <fstream>
#include <map>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>
#include <common/matrix_io.h>

#include "httplib.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

const std::string image_dir = "/mnt/shared/data/webp";
const std::string tag_dir = "/mnt/shared/data/tag";
const std::string tag_file = "/mnt/shared/data/all_tags_translated_250722.csv"; // 标签文件路径
const std::string cg_list_file = "/mnt/shared/data/cglist_250722.csv"; // CG 列表文件路径
const int page_size = 20;
const bool cache_cg_info = true; // 是否缓存 CG 信息
Matrix<std::string, 2> cached_cg_list;
Matrix<Matrix<std::string, 2>, 1> cached_tags;
constexpr size_t max_image_count = 10000; // 最大图片数量

Matrix<Matrix<std::string, 2>, 1> load_tags(const Matrix<std::string, 2>& cglist) {
    char splitChar = matrix_impl::getMatrixConfig().splitChar;
    matrix_impl::getMatrixConfig().splitChar = ' ';
    Matrix<Matrix<std::string, 2>, 1> tags(cglist.extent(0));
    for (size_t i = 0; i < cglist.extent(0); ++i) {
        if (i % 50000 == 0) {
            std::cout << "Tag loading progress: " << (i * 100.0 / cglist.extent(0)) << "%" << std::endl;
        }
        const auto& row = cglist[i];
        std::string tag_path = tag_dir + "/" + row[4] + "/image_" + row[5] + ".txt";
        if (!fs::exists(tag_path)) {
            continue;
        }
        std::ifstream fin(tag_path);
        if (!fin) {
            std::cerr << "Failed to open tag file: " << tag_path << std::endl;
            continue;
        }
        fin >> tags(i);
    }
    matrix_impl::getMatrixConfig().splitChar = splitChar;
    return tags;
}


// 加载标签列表, 返回Nx2的矩阵,第一列为英文标签，第二列为日文标签
Matrix<std::string, 2> load_tags(const std::string& filepath) {
    Matrix<std::string, 2> tags;
    std::ifstream fin(filepath);
    if (!fin) {
        std::cerr << "Error: Unable to open tag file." << std::endl;
        return tags;
    }
    fin >> tags;
    return tags;
}

std::map<std::string, std::string> load_id_title_map(const std::string& filepath) {
    Matrix<std::string, 2> cg_list;
    std::map<std::string, std::string> id_title_map;
    std::ifstream fin(filepath);
    if (!fin) {
        std::cerr << "Error: Unable to open CG list file." << std::endl;
        return {};
    }
    fin >> cg_list;
    for (size_t i = 0; i < cg_list.extent(0); ++i) {
        std::string id = cg_list(i, 4);
        std::string title = cg_list(i, 1);
        if (!id.empty() && !title.empty()) {
            if (id_title_map.find(id) == id_title_map.end()) {
                id_title_map[id] = title;
            }
        }
    }
    return id_title_map;
}

// 读取整个文件内容到字符串
std::string read_file(const std::string& filepath) {
    std::ifstream fin(filepath);
    if (!fin) return "Error: HTML file not found.";

    std::ostringstream ss;
    ss << fin.rdbuf();
    return ss.str();
}

// 将标签列表过滤成 JSON
std::string filter_tags(const Matrix<std::string, 2>& all_tags, const std::string& keyword) {
    std::ostringstream oss;
    oss << "[";
    bool first = true;
    for (size_t i = 0; i < all_tags.extent(0); ++i) {
        // 获取英文标签
        const auto& tag = all_tags(i, 0);
        if (tag.find(keyword) != std::string::npos) {
            if (!first) oss << ",";
            oss << "\"" << tag << "\"";
            first = false;
        }
    }
    oss << "]";
    return oss.str();
}

std::vector<std::string> split_tags(const std::string& tags, const char delimiter = ',') {
    std::vector<std::string> result;
    std::istringstream ss(tags);
    std::string tag;
    while (std::getline(ss, tag, delimiter)) {
        tag.erase(std::remove_if(tag.begin(), tag.end(), ::isspace), tag.end());
        if (!tag.empty()) result.push_back(tag);
    }
    return result;
}

std::vector<std::string> get_image_files_by_tags(const std::vector<std::string>& tags) {
    std::vector<std::string> images;
    int count = 0;
    for (const auto& entry : fs::recursive_directory_iterator(tag_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt") {
            std::string filename = entry.path().filename().string();
            std::ifstream fin(entry.path());
            Matrix<std::string, 2> tag_matrix;
            if (!fin) continue;
            fin >> tag_matrix;
            if (++count % 50000 == 0) {
                std::cout << "Processed file " << count << std::endl;
                std::cout << "tag_matrix size: " << tag_matrix.extent(0) << "x" << tag_matrix.extent(1) << std::endl;
            }
            bool match = true;
            // 检查每个标签是否在矩阵中
            for (const auto& tag : tags) {
                if (tag[0] == '-') {
                    // 如果标签以 '-' 开头，表示排除该标签
                    std::string exclude_tag = tag.substr(1);
                    bool found = false;
                    for (size_t i = 0; i < tag_matrix.extent(0); ++i) {
                        if (tag_matrix(i, 0) == exclude_tag) {
                            found = true;
                            break;
                        }
                    }
                    if (found) {
                        match = false;
                        break;
                    }
                } else {
                    // 正常匹配标签
                    bool found = false;
                    for (size_t i = 0; i < tag_matrix.extent(0); ++i) {
                        if (tag_matrix(i, 0) == tag) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        match = false;
                        break;
                    }
                }
            }
            if (match) {
                // 如果匹配，添加对应的图片文件名
                fs::path relative_path = fs::relative(entry.path(), tag_dir);
                relative_path.replace_extension(".webp");
                fs::path image_file = image_dir / relative_path;
                // 检查图片文件是否存在
                if (fs::exists(image_file)) {
                    if (images.size() < max_image_count)
                    images.push_back(relative_path.string()); // 只保存图片文件名
                }
            }
        }
    }
    return images;
}

std::vector<std::string> get_image_files_by_tags(const std::vector<std::string>& tags,
    const Matrix<std::string, 2>& cached_cg_list, const Matrix<Matrix<std::string, 2>, 1>& cached_tags) {
    std::vector<std::string> images;
    for (size_t i = 0; i < cached_tags.extent(0); ++i) {
        // if (i % 50000 == 0) {
        //     std::cout << "Processed tag " << i << " of " << cached_tags.extent(0) << std::endl;
        // }
        Matrix<std::string, 2> tag_matrix = cached_tags(i);
        bool match = true;
        // 检查每个标签是否在矩阵中
        for (const auto& tag : tags) {
            if (tag[0] == '-') {
                // 如果标签以 '-' 开头，表示排除该标签
                std::string exclude_tag = tag.substr(1);
                bool found = false;
                for (size_t j = 0; j < tag_matrix.extent(0); ++j) {
                    if (tag_matrix(j, 0) == exclude_tag) {
                        found = true;
                        break;
                    }
                }
                if (found) {
                    match = false;
                    break;
                }
            } else {
                // 正常匹配标签
                bool found = false;
                for (size_t j = 0; j < tag_matrix.extent(0); ++j) {
                    if (tag_matrix(j, 0) == tag) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    match = false;
                    break;
                }
            }
        }
        if (match) {
            if (images.size() < max_image_count)
            images.push_back(cached_cg_list(i, 4) + "/image_" + cached_cg_list(i, 5) + ".webp"); // 只保存图片文件名
        }
    }
    return images;
}

int main() {
    // matrix_impl::getMatrixConfig().splitChar = ' ';
    // auto images = get_image_files_by_tags({ "sleeve_cuffs" });
    // std::cout << "Found " << images.size() << " images for the tag 'sleeve_cuffs':\n";
    // for (const auto& img : images) {
    //     std::cout << " - " << img << "\n";
    // }
    // matrix_impl::getMatrixConfig().splitChar = ',';
    // return 0;

    httplib::Server svr;
    Matrix<std::string, 2> all_tags = load_tags(tag_file);
    std::cout << "Loaded " << all_tags.extent(0) << " tags from " << tag_file << std::endl;
    std::map<std::string, std::string> tag_translation_map;
    for (size_t i = 0; i < all_tags.extent(0); ++i) {
        tag_translation_map[all_tags(i, 0)] = all_tags(i, 1);
    }
    std::map<std::string, std::string> id_title_map = load_id_title_map(cg_list_file);
    std::cout << "Loaded " << id_title_map.size() << " CG titles from " << cg_list_file << std::endl;

    if (cache_cg_info) {
        std::ifstream fin(cg_list_file);
        if (!fin) {
            std::cerr << "Error: Unable to open CG list file for caching." << std::endl;
            return 1;
        }
        fin >> cached_cg_list;
        std::cout << "Loaded CG list with " << cached_cg_list.extent(0) << " entries." << std::endl;
        cached_tags = load_tags(cached_cg_list);
        std::cout << "Loaded tags for CG list." << std::endl;
    } else {
        std::cout << "CG info caching is disabled." << std::endl;
    }

    matrix_impl::getMatrixConfig().splitChar = ' '; // 设置分隔符为逗号

    // 页面
    std::string html_cache;
    svr.Get("/", [&](const httplib::Request& req, httplib::Response& res) {
        if (html_cache.empty()) {
            html_cache = read_file("../index.html");
        }
        res.set_content(html_cache, "text/html");
    });

    // 过滤标签 API
    svr.Get("/tags", [&](const httplib::Request& req, httplib::Response& res) {
        auto it = req.get_param_value("filter");
        std::string json = filter_tags(all_tags, it);
        res.set_content(json, "application/json");
    });

    // 验证标签是否合法
    svr.Post("/validate", [&](const httplib::Request& req, httplib::Response& res) {
        std::string body = req.body;
        std::vector<std::string> input_tags;
        std::istringstream ss(body);
        std::string tag;

        // 拆分输入标签（支持逗号或空格）
        while (std::getline(ss, tag, ',')) {
            tag.erase(std::remove_if(tag.begin(), tag.end(), ::isspace), tag.end());
            if (!tag.empty()) input_tags.push_back(tag);
        }

        std::vector<std::string> invalid;
        for (const auto& t : input_tags) {
            if (std::find(all_tags.begin(), all_tags.end(), t) == all_tags.end()) {
                invalid.push_back(t);
            }
        }

        if (!invalid.empty()) {
            std::ostringstream err;
            err << "以下标签不合法：";
            for (const auto& t : invalid) err << t << " ";
            res.set_content(err.str(), "text/plain");
            res.status = 400;
        } else {
            res.set_content("OK", "text/plain");
        }
    });

    // 图片资源
    svr.Get("/sample.png", [&](const httplib::Request& req, httplib::Response& res) {
        std::ifstream in("../sample.png", std::ios::binary);
        if (!in) {
            res.status = 404;
            res.set_content("Not found", "text/plain");
            return;
        }
        std::ostringstream oss;
        oss << in.rdbuf();
        res.set_content(oss.str(), "image/png");
    });

    // // 模拟图片生成接口
    // svr.Post("/generate_image", [&](const httplib::Request& req, httplib::Response& res) {
    //     try {
    //         auto j = json::parse(req.body);
    //         std::string tags = j["tags"];

    //         std::cout << "收到生成请求，标签：" << tags << std::endl;

    //         // 模拟生成耗时
    //         std::this_thread::sleep_for(std::chrono::seconds(2));

    //         // 返回图片（你可以换成实际生成的图片路径）
    //         std::ifstream in("../sample.png", std::ios::binary);
    //         if (!in) {
    //             res.status = 500;
    //             res.set_content("示例图片不存在", "text/plain");
    //             return;
    //         }

    //         std::ostringstream oss;
    //         oss << in.rdbuf();
    //         res.set_content(oss.str(), "image/png");

    //     } catch (const std::exception& e) {
    //         res.status = 400;
    //         res.set_content(std::string("请求解析失败: ") + e.what(), "text/plain");
    //     }
    // });
    std::vector<std::string> search_result_images;
    svr.Post("/search", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = json::parse(req.body);
            std::string tags = j["tags"];

            std::cout << "搜索标签：" << tags << std::endl;

            std::vector<std::string> tag_list = split_tags(tags);
            if (tag_list.empty()) {
                res.status = 400;
                res.set_content("标签不能为空", "text/plain");
                return;
            }
            // search_result_images = get_image_files_by_tags(tag_list);

            json response;
            if (cache_cg_info) {
                response["images"] = get_image_files_by_tags(tag_list, cached_cg_list, cached_tags);
            } else {
                response["images"] = get_image_files_by_tags(tag_list);
            }
            res.set_content(response.dump(), "application/json");
        } catch (const std::exception& e) {
            std::cerr << __LINE__ << " Error parsing request: " << e.what() << std::endl;
            res.status = 400;
            res.set_content(std::string("请求解析失败: ") + e.what(), "text/plain");
        }
    });

    // /gallery?page=N
    // svr.Get("/gallery", [&](const httplib::Request& req, httplib::Response& res) {
    //     try {
    //         int page = 1;
    //         if (req.has_param("page")) {
    //             page = std::stoi(req.get_param_value("page"));
    //         }

    //         int total_pages = (search_result_images.size() + page_size - 1) / page_size;
    //         if (page > total_pages) page = total_pages;

    //         int start = (page - 1) * page_size;
    //         int end = std::min<int>(start + page_size, search_result_images.size());

    //         std::ostringstream html;
    //         html << "<html><body><h1>图片画廊</h1><div style='display:flex;flex-wrap:wrap;'>";

    //         for (int i = start; i < end; ++i) {
    //             html << "<div style='margin:10px;text-align:center'>"
    //                 << "<a href='/img/" << search_result_images[i] << "' target='_blank'>"
    //                 << "<img src='/img/" << search_result_images[i] << "' width='200'></a><br>"
    //                 << search_result_images[i] << "</div>";
    //         }

    //         html << "</div><div style='margin-top:20px;'>";
    //         if (page > 1) {
    //             html << "<a href='/gallery?page=" << (page - 1) << "'>上一页</a> ";
    //         }
    //         if (page < total_pages) {
    //             html << "<a href='/gallery?page=" << (page + 1) << "'>下一页</a>";
    //         }
    //         html << "</div></body></html>";

    //         res.set_content(html.str(), "text/html");
    //     } catch (const std::exception& e) {
    //         res.status = 400;
    //         res.set_content(std::string("请求解析失败: ") + e.what(), "text/plain");
    //     }
    // });

    // /img/<filename>
    svr.Get(R"(/img/(.+))", [&](const httplib::Request& req, httplib::Response& res) {
        std::string filename = req.matches[1];
        std::string path = image_dir  + "/" + filename;

        std::ifstream in(path, std::ios::binary);
        if (!in) {
            std::cerr << "Error: Image file not found: " << path << std::endl;
            res.status = 404;
            res.set_content("未找到图片", "text/plain");
            return;
        }

        std::ostringstream ss;
        ss << in.rdbuf();
        res.set_content(ss.str(), "image/webp");
    });

    // /image_info?file=<filename>
    svr.Get("/image_info", [&](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("file")) {
            std::cerr << "Error: Missing 'file' parameter in request." << std::endl;
            res.status = 400;
            res.set_content("缺少 file 参数", "text/plain");
            return;
        }

        std::string filename = req.get_param_value("file");
        std::string fullpath = image_dir + "/" + filename;

        // 假设详细信息存在 .txt 文件中（img_001.webp → img_001.txt）
        std::string tag_info_path = tag_dir + "/" + filename.substr(0, filename.find_last_of('.')) + ".txt";

        std::ifstream fin(tag_info_path);
        if (!fin) {
            std::cerr << "Error: Tag info file not found: " << tag_info_path << std::endl;
            res.status = 404;
            res.set_content("找不到该图片的描述信息", "text/plain");
            return;
        }

        std::ostringstream oss;
        if (id_title_map.find(filename.substr(0, filename.find_first_of('/'))) != id_title_map.end()) {
            oss << "<strong>Image Source:</strong> " << id_title_map[filename.substr(0, filename.find_first_of('/'))] << "<br>";
        }

        std::string tag;
        double score = 0.0;
        while (fin >> tag >> score) {
            if (tag_translation_map.find(tag) != tag_translation_map.end()) {
                oss << tag << " (" << tag_translation_map[tag] << ") " << " " << score << "<br>";
            } else {
                oss << tag << "<br>";
            }
        }

        res.set_content(oss.str(), "text/html");
    });

    std::cout << "Server running at http://localhost:8080/ ...\n";
    svr.listen("0.0.0.0", 8080);
}
