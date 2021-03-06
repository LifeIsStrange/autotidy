#pragma once

#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

inline void pipeCommandToFile(std::string const& cmdLine,
                              std::string const& outFile)
{
    using arr = std::array<uint8_t, 1024>;
    auto* fp = popen(cmdLine.c_str(), "r");
    auto* outfp = fopen(outFile.c_str(), "we");
    arr buf;
    while (feof(fp) == 0) {
        auto sz = fread(buf.data(), sizeof(arr::value_type), buf.size(), fp);
        fwrite(buf.data(), 1, sz, outfp);
    }
    fclose(outfp);
    pclose(fp);
}

inline void pipeStringToCommand(std::string const& cmdLine,
                                std::string const& text)
{
    auto* fp = popen(cmdLine.c_str(), "w");
    fwrite(text.c_str(), 1, text.length(), fp);
    pclose(fp);
}

inline char getch()
{
    char buf = 0;
    termios old{};
    if (tcgetattr(0, &old) < 0) {
        throw std::runtime_error("tcsetattr()");
    }
    old.c_lflag &= ~ICANON;
    old.c_lflag &= ~ECHO;
    old.c_cc[VMIN] = 1;
    old.c_cc[VTIME] = 0;
    if (tcsetattr(0, TCSANOW, &old) < 0) {
        throw std::runtime_error("tcsetattr ICANON");
    }
    if (read(0, &buf, 1) < 0) {
        throw std::runtime_error("read()");
    }
    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    if (tcsetattr(0, TCSADRAIN, &old) < 0) {
        throw std::runtime_error("tcsetattr ~ICANON");
    }
    return buf;
}

inline bool fileExists(const std::string& name)
{
    struct stat buffer; // NOLINT
    return (stat(name.c_str(), &buffer) == 0);
}

inline std::string currentDir()
{
    char buf[4096] = {0};
    getcwd(&buf[0], sizeof(buf));
    return std::string(&buf[0]);
}

inline void copyFileToFrom(std::string const& target, std::string const& source)
{
    std::remove(target.c_str());
    std::ifstream src(source, std::ios::binary);
    if (src.is_open()) {
        std::ofstream dst(target, std::ios::binary);
        dst << src.rdbuf();
    }
}

inline std::vector<char> readFile(std::string const& fileName)
{
    std::vector<char> buffer;
    std::ifstream file(fileName, std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    if (size <= 0) {
        return buffer;
    }
    buffer.resize(size);

    if (file.read(buffer.data(), size)) {
        return buffer;
    }
    return buffer;
}

template <typename T>
inline void writeFile(std::string const& outputFile, T const& contents)
{
    std::ofstream out(outputFile);
    out << contents;
    out.close();
}

template <class Container, class T>
bool contains(Container const& c, const T& value)
{
    return c.count(value) > 0;
}

inline size_t lineColToOffset(std::vector<char> const& contents, int line,
                              int col)
{
    if (line == 0) {
        return -1;
    }
    if (line == 1) {
        return col;
    }

    auto it = contents.begin();
    col--;
    line--;
    while ((it = std::find(it, contents.end(), '\n')) != contents.end()) {
        line--;
        it++;
        if (line == 0) {
            return std::distance(contents.begin(), it) + col;
        }
    }
    return -1;
}

inline std::pair<int, int> offsetToLineCol(std::vector<char> const& contents,
                                           size_t offset)
{
    auto it = contents.begin();
    auto prevIt = it;
    int line = 1;
    int64_t offs = offset;

    while ((it = std::find(it, contents.end(), 0xa)) != contents.end()) {
        it++;
        if (std::distance(contents.begin(), it) > offs) {
            // We have passed our offset
            return std::make_pair(
                line, offs - std::distance(contents.begin() + 1, prevIt));
        }
        line++;
        prevIt = it;
    }
    return std::make_pair(-1, -1);
}

