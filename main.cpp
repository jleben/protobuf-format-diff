#include "comparison.h"

#include <iostream>

using namespace std;

int main(int argc, char * argv[])
{
    if (argc < 6)
    {
        cerr << "Expected arguments: root-dir1 file1 root-dir2 file2 message" << endl;
        cerr << "Use '.' for message to compare all messages in given files." << endl;
        return 1;
    }

    Comparison comparison;

    try
    {
        Source source1(argv[2], argv[1]);
        Source source2(argv[4], argv[3]);
        string message_name = argv[5];
        if (message_name == ".")
            comparison.compare(source1, source2);
        else
            comparison.compare(source1, source2, message_name);
    }
    catch(std::exception & e)
    {
        cerr << e.what() << endl;
        return 1;
    }

    comparison.root.trim();
    comparison.root.print();

    return 0;
}

