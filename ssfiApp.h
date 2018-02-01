#ifndef WORDCOUNTER_H
#define WORDCOUNTER_H

#include <unordered_map>
#include <vector>
#include <thread>
#include <string>

#include "concurrentqueue.h"

class SsfiApp
{
public:
    using TopWords = std::vector<std::pair<std::string, size_t>>;

    SsfiApp(std::string directory, size_t numThreads = 1);
    TopWords getTopWords(unsigned topCount); // Blocking call
private:
    using WordCounts = std::unordered_map<std::string, size_t>;

    void countWordsThreadFunc(); // Worker threads run this function to count words in a file
    void fileSearchThreadFunc(std::string dir); // Searches files in given dir and pushes filenames to files queue
    void mergeResultsThreadFunc(); // merges the counts from worker threads

    ConcurrentQueue<std::string> files; // fileSearcher pushes filenames here
    ConcurrentQueue<WordCounts> results; // worker threads pop from files queue and push their word counts on this queue
    WordCounts aggregatedWordCounts; // mergeResultsThread pops from results and creates final results here

    std::thread fileSearcherThread;
    std::vector<std::thread> wordCountWorkerThreads;
    std::thread mergeResultsThread;
    size_t numWorkerThreads;
};

#endif // WORDCOUNTER_H
