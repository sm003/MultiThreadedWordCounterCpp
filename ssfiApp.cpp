#include "ssfiApp.h"
#include <fstream>
#include <regex>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <queue>
#include <chrono>
#include <boost/filesystem.hpp>

// todo - try catch
SsfiApp::SsfiApp(std::string directory, size_t numThreads) :
    fileSearcherThread(std::thread(&SsfiApp::fileSearchThreadFunc, this, std::move(directory))),
    wordCountWorkerThreads(numThreads),
    mergeResultsThread(std::thread(&SsfiApp::mergeResultsThreadFunc, this)),
    numWorkerThreads(numThreads)
{
    fileSearcherThread.detach();

    for (size_t i = 0; i < numThreads; ++i)
    {
        wordCountWorkerThreads[i] = std::thread(&SsfiApp::countWordsThreadFunc, this);
    }
}

void SsfiApp::fileSearchThreadFunc(std::string dir)
{
    boost::filesystem::path root(dir);
    boost::filesystem::path ext(".txt");

    if (!boost::filesystem::exists(root) || !boost::filesystem::is_directory(root))
        return;

    boost::filesystem::recursive_directory_iterator it(root);
    boost::filesystem::recursive_directory_iterator endit;

    while(it != endit)
    {
        if(boost::filesystem::is_regular_file(*it) && (it->path().extension() == ext))
        {
            files.Push(it->path().string());
          //  std::cout << it->path().string() << std::endl;
        }
        ++it;
    }

    files.Shutdown();
}

void SsfiApp::countWordsThreadFunc()
{
    std::string pattern("[a-zA-Z0-9]+");
    std::regex wordRegex(pattern, std::regex::egrep);
    WordCounts words;

    while (1)
    {
        std::string fileName;

        if (!files.Pop(fileName))
        {
            // Queue is shutdown and empty; all files processed
            results.Push(std::move(words));
            return;
        }

        std::ifstream ifile(fileName);
        if (!ifile.is_open())
        {
            std::cerr << "Unable to open file " << fileName << std::endl;
            continue;
        }

        std::string line;
        while (getline(ifile, line))
        {
            for (std::sregex_iterator begIt(line.begin(), line.end(), wordRegex), endIt; begIt != endIt; ++begIt)
            {
                std::string word = begIt->str();
                std::transform(word.begin(), word.end(), word.begin(), ::tolower);
                ++words[word];
            }
        }
    }
}

void SsfiApp::mergeResultsThreadFunc()
{
    auto start = std::chrono::steady_clock::now();
    while (1)
    {
        WordCounts wordCounts;
        if (!results.Pop(wordCounts))
        {
            auto time = std::chrono::steady_clock::now() - start;
            std::cout << "Merge results thread (ms): " << std::chrono::duration_cast<std::chrono::milliseconds>(time).count() << std::endl;
            return;
        }
        for (const auto & wordCountPair : wordCounts)
        {
            aggregatedWordCounts[wordCountPair.first] += wordCountPair.second;
        }
    }
}

auto SsfiApp::getTopWords(unsigned topCount) -> TopWords
{
    TopWords topWordCounts;

    auto start = std::chrono::steady_clock::now();

    for (size_t i = 0; i < numWorkerThreads; ++i)
        wordCountWorkerThreads[i].join();

    auto time1 = std::chrono::steady_clock::now();
    std::cout << "Join on workers (ms): " << std::chrono::duration_cast<std::chrono::milliseconds>(time1 - start).count() << std::endl;

    // all worker threads have pushed their results on the queue. Notify mergeResultsThread there are no more results left to merge
    results.Shutdown();
    mergeResultsThread.join();

    // Copy the word counts into a priority heap and return the top words
    using WordPair = std::pair<std::string, size_t>;
    auto compWordCounts = [] (const WordPair &w1, const WordPair &w2) { return w1.second < w2.second; };
    std::priority_queue<WordPair, std::vector<WordPair>, decltype(compWordCounts)> wordQueue(aggregatedWordCounts.begin(),
                                                                                             aggregatedWordCounts.end(), compWordCounts);

    for (size_t i = 0; i < topCount && !wordQueue.empty(); ++i)
    {
        topWordCounts.push_back(wordQueue.top());
        wordQueue.pop();
    }

    return topWordCounts;
}
