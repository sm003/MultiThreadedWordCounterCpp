#include <iostream>
#include "ssfiApp.h"
#include <cstdlib>

using namespace std;

int main(int argc, char *argv[])
{
    // todo - parse cmd line args
    size_t numThreads = static_cast<size_t>(atol(argv[1]));
    SsfiApp wordCountApp(argv[2], numThreads);
    SsfiApp::TopWords topWordCounts = wordCountApp.getTopWords(10);

    cout << "Top Words: " << endl;
    for (const auto & wordCountPair : topWordCounts)
        cout << wordCountPair.first << "\t" << wordCountPair.second << endl;

    return 0;
}
