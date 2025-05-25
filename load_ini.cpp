#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "load_ini.h"



resolution_info::resolution_info(int w, int h, const std::string& c) {
    width = w;
    height = h;
    comment = c;
}

resolution_info::resolution_info() {
	width = 0;
	height = 0;
	comment = "unload";
}

resolution_info* LoadResolutionsFromFile(const char* filename) {
    static resolution_info resolutions[128];

	for (resolution_info& res : resolutions) {
		res = resolution_info(); // 初期化
	}

    std::ifstream file(filename);

    if (!file.is_open()) {
        // std::cerr << "ファイルを開けませんでした: " << filename << std::endl;
        return resolutions;
    }

    std::string line;
	int count = 0;
    while (std::getline(file, line)) {
        // 空行・コメント行をスキップ
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string widthStr, heightStr, commentStr;

        if (std::getline(iss, widthStr, ',') && std::getline(iss, heightStr, ',') && std::getline(iss, commentStr) && count < 128) {
            int width;
            int height;
            try {
                width = std::stoi(widthStr);
                height = std::stoi(heightStr);
			}
			catch (const std::invalid_argument) {
				// std::cerr << "無効な数値: " << widthStr << ", " << heightStr << std::endl;
				continue;
			}
			catch (const std::out_of_range) {
				// std::cerr << "数値が範囲外: " << widthStr << ", " << heightStr << std::endl;
				continue;
			}
            std::string comment = commentStr; // いらないけど、、、。
			resolutions[count] = resolution_info(width, height, comment);
            count++;
        }
    }
    return resolutions;
}