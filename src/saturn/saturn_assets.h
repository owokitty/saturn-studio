#ifndef SaturnAssets
#define SaturnAssets

#include <string>
#include <vector>
#include <map>

struct Asset {
    std::string path;
    int mio0;
    int pos;
    int len;
    std::vector<int> metadata;
};

struct SaturnAsset {
    std::string path;
    unsigned char* data;
    std::vector<int> metadata;
};

extern std::vector<Asset> assets;
extern std::vector<SaturnAsset> saturn_assets;
extern std::map<std::string, std::pair<int, unsigned int*>> file_processor;
extern std::vector<int> bitfs_ptrtable;
extern std::vector<std::string> menu_font;

#endif