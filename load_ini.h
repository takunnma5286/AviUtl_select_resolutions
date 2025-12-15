#pragma once

class resolution_info{
public:
	int width;
	int height;
	std::string fps;
	std::string comment;
	resolution_info();
	resolution_info(int w, int h, const std::string& f, const std::string& c);
};

resolution_info* LoadResolutionsFromFile(const char* filename);
