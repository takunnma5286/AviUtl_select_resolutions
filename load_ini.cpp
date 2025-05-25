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
		res = resolution_info(); // ������
	}

    std::ifstream file(filename);

    if (!file.is_open()) {
        // std::cerr << "�t�@�C�����J���܂���ł���: " << filename << std::endl;
        return resolutions;
    }

    std::string line;
	int count = 0;
    while (std::getline(file, line)) {
        // ��s�E�R�����g�s���X�L�b�v
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
				// std::cerr << "�����Ȑ��l: " << widthStr << ", " << heightStr << std::endl;
				continue;
			}
			catch (const std::out_of_range) {
				// std::cerr << "���l���͈͊O: " << widthStr << ", " << heightStr << std::endl;
				continue;
			}
            std::string comment = commentStr; // ����Ȃ����ǁA�A�A�B
			resolutions[count] = resolution_info(width, height, comment);
            count++;
        }
    }
    return resolutions;
}