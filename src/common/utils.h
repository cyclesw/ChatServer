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
    
    /**
     * @brief 生成一个由16位随机字符组成的唯一ID
     * @return std::string 返回生成的ID字符串，格式为：XXXXXX-XXXX-XXXX
     *         其中前6位是随机十六进制数，中间用'-'分隔，后4位是递增的序号
     */
    std::string Uuid()
    {
        // 初始化随机数生成器
        std::random_device rd;
        std::mt19937 generator(rd());
        std::uniform_int_distribution<int> distribution(0, 255);

        std::stringstream ss;
        // 生成6个随机十六进制数，并在第3个位置添加'-'分隔符
        for (int i = 0; i < 6; ++i)
        {
            if (i == 2)
                ss << '-';
            ss << std::setw(2) << std::setfill('0') << std::hex << distribution(generator);
        }

        // 添加分隔符并添加递增的序号
        ss << '-';
        static std::atomic<short> idx(0);
        short tmp = idx.fetch_add(1);
        ss << std::setw(4) << std::setfill('0') << std::hex << tmp;
        return ss.str();
    }

    /**
     * @brief 读取文件内容到字符串中
46  * @param filename 要读取的文件名
47  * @param body 用于存储文件内容的字符串引用
48  * @return bool 成功返回true，失败返回false
     */
    bool ReadFile(const std::string &filename, std::string &body)
    {
        // 以二进制模式打开文件
        std::ifstream ifs(filename, std::ios::binary | std::ios::in);
        if (!ifs.is_open())
        {
            LOG_ERROR("打开文件 {} 失败", filename);
            return false;
        }

        // 获取文件大小并读取全部内容
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

    /**
     * @brief 将字符串内容写入文件
78  * @param filename 要写入的文件名
79  * @param body 要写入文件的内容
80  * @return bool 成功返回true，失败返回false
     */
    bool WriteFile(const std::string &filename, const std::string &body)
    {
        // 以二进制模式打开文件
        std::ofstream ofs(filename, std::ios::binary | std::ios::out);
        if (!ofs.is_open())
        {
            LOG_ERROR("打开文件 {} 失败", filename);
            return false;
        }

        // 写入内容并检查是否成功
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

    /**
     * @brief 生成4位数字验证码
107  * @return std::string 返回4位数字组成的验证码字符串
     */
    std::string VerifyCode() {
        // 初始化随机数生成器
        std::random_device rd;
        std::mt19937 generator(rd());
        std::uniform_int_distribution<int> distribution(0, 9);

        std::stringstream ss;
        // 生成4位随机数字
        for (int i = 0; i < 4; ++i)
        {
            ss << distribution(generator);
        }
        return ss.str();
    }


} // namespace im