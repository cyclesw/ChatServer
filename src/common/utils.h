#pragma once

#include "log.hpp"
#include <ios>
#include <iostream>
#include <random>
#include <sstream>
#include <iomanip>
#include <atomic>
#include <fstream>

namespace im
{
    
    // 生成一个由16位随机字符组成的ID
std::string Uuid()
{
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<int> distribution(0, 255);

    std::stringstream ss;
    for (int i = 0; i < 6; ++i)
    {
        if (i == 2)
            ss << '-';
        ss << std::setw(2) << std::setfill('0') << std::hex << distribution(generator);
    }

    ss << '-';
    static std::atomic<short> idx(0);
    short tmp = idx.fetch_add(1);
    ss << std::setw(4) << std::setfill('0') << std::hex << tmp;
    return ss.str();
}

bool ReadFile(const std::string &filename, std::string &body)
{
    std::ifstream ifs(filename, std::ios::binary | std::ios::in);
    if (!ifs.is_open())
    {
        LOG_ERROR("打开文件 {} 失败", filename);
        return false;
    }

    ifs.seekg(0, std::ios::end);
    size_t flen = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    body.resize(flen);
    ifs.read(body.data(), flen);
    if (ifs.good() == false)
    {
        LOG_ERROR("读取文件 {} 失败", filename);
        ifs.close();
        return false;
    }
    ifs.close();
    return true;
}

bool WriteFile(const std::string &filename, const std::string &body)
{
    std::ofstream ofs(filename, std::ios::binary | std::ios::out);
    if (!ofs.is_open())
    {
        LOG_ERROR("打开文件 {} 失败", filename);
        return false;
    }

    ofs.write(body.data(), body.size());
    if (ofs.good() == false)
    {
        LOG_ERROR("写入文件 {} 失败", filename);
        ofs.close();
        return false;
    }

    ofs.close();
    return true;
}

std::string VerifyCode() {
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<int> distribution(0, 9);

    std::stringstream ss;
    for (int i = 0; i < 4; ++i)
    {
        ss << distribution(generator);
    }
    return ss.str();
}


} // namespace im