#include <google/protobuf/compiler/importer.h>

#include <iostream>

using namespace std;

class ErrorCollector : public google::protobuf::compiler::MultiFileErrorCollector
{
public:
    ErrorCollector() {}

    void AddError(const std::string & filename, int line, int column, const std::string & message) override
    {
        cerr << "Error: " << message << endl;
    }

    void AddWarning(const std::string & filename, int line, int column, const std::string & message) override
    {
        cerr << "Warning: " << message << endl;
    }
};

int main(int argc, char * argv[])
{
    if (argc < 3)
    {
        cerr << "Missing arguments: file root-dir" << endl;
        return 1;
    }

    string file_path = argv[1];
    string root_dir = argv[2];

    using namespace google::protobuf::compiler;

    DiskSourceTree source_tree;
    source_tree.MapPath("/", root_dir);

    ErrorCollector error_collector;

    Importer importer(&source_tree, &error_collector);
    auto * file_descriptor = importer.Import(file_path);

    if (!file_descriptor)
    {
        cerr << "Did not get a file descriptor." << endl;
        return 1;
    }

    cout << "Package: " << file_descriptor->package() << endl;

    return 0;
}

