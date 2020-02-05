#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/descriptor.h>

#include <iostream>
#include <memory>
#include <list>

using namespace std;

using google::protobuf::Descriptor;
using google::protobuf::FieldDescriptor;
using google::protobuf::EnumDescriptor;

class ErrorCollector : public google::protobuf::compiler::MultiFileErrorCollector
{
public:
    ErrorCollector() {}

    void AddError(const std::string & filename, int line, int column, const std::string & message) override
    {
        cerr << "Error: " << filename << "@" << line << "," << column << ": " << message << endl;
    }

    void AddWarning(const std::string & filename, int line, int column, const std::string & message) override
    {
        cerr << "Warning: " << filename << "@" << line << "," << column << ": " << message << endl;
    }
};

class Source
{
    using DiskSourceTree = google::protobuf::compiler::DiskSourceTree;
    using Importer = google::protobuf::compiler::Importer;
    using DescriptorPool = google::protobuf::DescriptorPool;
    using FileDescriptor = google::protobuf::FileDescriptor;

public:
    Source() {}
    Source(const string & file_path, const string & root_dir)
    {
        source_tree.MapPath("", root_dir);

        ErrorCollector error_collector;

        importer = make_shared<Importer>(&source_tree, &error_collector);

        d_file_descriptor = importer->Import(file_path);
        if (!d_file_descriptor)
        {
            throw std::runtime_error("Failed to load source.");
        }
    }

    const FileDescriptor * file_descriptor() const { return d_file_descriptor; }
    const DescriptorPool * pool() const { return importer->pool(); }

private:
    DiskSourceTree source_tree;
    ErrorCollector error_collector;
    shared_ptr<Importer> importer;
    const FileDescriptor * d_file_descriptor = nullptr;
};

class Comparison
{
public:
    struct Item
    {
        Item(const string & what): what(what) {}
        string what;
    };

    struct Section
    {
        Section(const string & what) : what(what) {}

        Section & add_subsection(const string & what)
        {
            subsections.emplace_back(what);
            return subsections.back();
        }

        void add_item(const string & what)
        {
            items.emplace_back(what);
        }

        bool is_empty() const { return subsections.empty() and items.empty(); }

        void trim()
        {
            auto s = subsections.begin();
            while (s != subsections.end())
            {
                s->trim();
                if (s->is_empty())
                    s = subsections.erase(s);
                else
                    ++s;
            }
        }

        void print(int level = 0)
        {
            cout << string(level*2, ' ') << what << endl;

            ++level;

            string subprefix(level*2, ' ');

            for (auto & item : items)
            {
                cout << subprefix << "* " << item.what << endl;
            }

            for (auto & subsection : subsections)
            {
                subsection.print(level);
            }
        }

        string what;
        list<Section> subsections;
        list<Item> items;
    };

    void compare(Source & source1, Source & source2);
    void compare(Source & source1, Source & source2, const string & message_name);
    Section compare(const EnumDescriptor * enum1, const EnumDescriptor * enum2);
    Section compare(const Descriptor * desc1, const Descriptor * desc2);
    Section compare(const FieldDescriptor * field1, const FieldDescriptor * field2);
    bool compare_default_value(const FieldDescriptor * field1, const FieldDescriptor * field2);

    Section root { "/" };
};

void print_field(const FieldDescriptor * field)
{
    cout << field->number() << " = " << field->full_name()
         << " " << field->type_name()
         << " " << field->label();

    if (field->has_default_value())
    {
        cout << " (";
        switch(field->cpp_type())
        {
        case FieldDescriptor::CPPTYPE_INT32:
            cout << field->default_value_int32(); break;
        case FieldDescriptor::CPPTYPE_INT64:
            cout << field->default_value_int64(); break;
        case FieldDescriptor::CPPTYPE_UINT32:
            cout << field->default_value_uint32(); break;
        case FieldDescriptor::CPPTYPE_UINT64:
            cout << field->default_value_uint64(); break;
        case FieldDescriptor::CPPTYPE_FLOAT:
            cout << field->default_value_float(); break;
        case FieldDescriptor::CPPTYPE_DOUBLE:
            cout << field->default_value_double(); break;
        case FieldDescriptor::CPPTYPE_BOOL:
            cout << field->default_value_bool(); break;
        case FieldDescriptor::CPPTYPE_STRING:
            cout << field->default_value_string(); break;
        case FieldDescriptor::CPPTYPE_ENUM:
            cout << field->default_value_enum()->name(); break;
        }
        cout << " )";
    }
}

bool Comparison::compare_default_value(const FieldDescriptor * field1, const FieldDescriptor * field2)
{
    if (field1->has_default_value() != field2->has_default_value())
        return false;

    if (!field1->has_default_value())
        return true;

    if (field1->cpp_type() != field2->cpp_type())
        return false;

    switch(field1->cpp_type())
    {
    case FieldDescriptor::CPPTYPE_INT32:
        return field1->default_value_int32() == field2->default_value_int32(); break;
    case FieldDescriptor::CPPTYPE_INT64:
        return field1->default_value_int64() == field2->default_value_int64(); break;
    case FieldDescriptor::CPPTYPE_UINT32:
        return field1->default_value_uint32() == field1->default_value_uint32(); break;
    case FieldDescriptor::CPPTYPE_UINT64:
        return field1->default_value_uint64() == field2->default_value_uint64(); break;
    case FieldDescriptor::CPPTYPE_FLOAT:
        return field1->default_value_float() == field2->default_value_float(); break;
    case FieldDescriptor::CPPTYPE_DOUBLE:
        return field1->default_value_double() == field1->default_value_double(); break;
    case FieldDescriptor::CPPTYPE_BOOL:
        return field1->default_value_bool() == field1->default_value_bool(); break;
    case FieldDescriptor::CPPTYPE_STRING:
        return field1->default_value_string() == field1->default_value_string(); break;
    case FieldDescriptor::CPPTYPE_ENUM:
        return field1->default_value_enum()->number() == field1->default_value_enum()->number(); break;
    default:
        return false;
    }
}

Comparison::Section Comparison::compare(const EnumDescriptor * enum1, const EnumDescriptor * enum2)
{
    Section section("Comparing enums " + enum1->full_name() + " -> " + enum2->full_name());

    for (int i = 0; i < enum1->value_count(); ++i)
    {
        auto * value1 = enum1->value(i);
        auto * value2 = enum2->FindValueByName(value1->name());

        if (value2)
        {
            if (value1->number() != value2->number())
            {
                section.add_item("Changed value ID: "
                                 + value1->name() + " = " + to_string(value1->number())
                                 + " -> "
                                 + value2->name() + " = " + to_string(value2->number()));
            }
        }
        else
        {
            section.add_item("Removed enum value: " + value1->name());
        }
    }

    for (int i = 0; i < enum2->value_count(); ++i)
    {
        auto * value2 = enum2->value(i);
        auto * value1 = enum1->FindValueByName(value2->name());
        if (!value1)
        {
            section.add_item("Added enum value: " + value2->name());
        }
    }

    return section;
}

Comparison::Section Comparison::compare(const FieldDescriptor * field1, const FieldDescriptor * field2)
{
#if 0
    print_field(field1);
    cout << " -> ";
    print_field(field2);
    cout << endl;
#endif

    Section section("Comparing fields: " + field1->full_name() + " -> " + field2->full_name());

    if (field1->name() != field2->name())
    {
        section.add_item("Changed name.");
    }

    if (field1->number() != field2->number())
    {
        section.add_item("Changed ID.");
    }

    if (field1->label() != field2->label())
    {
        section.add_item("Changed label.");
    }

    if (field1->type() != field2->type())
    {
        section.add_item("Changed type.");
    }

    if (field1->type() == FieldDescriptor::TYPE_ENUM)
    {
        auto * enum1 = field1->enum_type();
        auto * enum2 = field2->enum_type();

        if (enum1->full_name() != enum2->full_name())
        {
            section.items.push_back("Changed field type name: "
                                    + enum1->full_name() + " -> "
                                    + enum2->full_name());
        }

        {
            section.subsections.push_back(compare(enum1, enum2));
        }
    }

    if (field1->type() == FieldDescriptor::TYPE_MESSAGE)
    {
        auto * msg1 = field1->message_type();
        auto * msg2 = field2->message_type();

        if (msg1->full_name() != msg2->full_name())
        {
            section.items.push_back("Changed field type name: "
                                    + msg1->full_name() + " -> "
                                    + msg2->full_name());
        }

        {
            section.subsections.push_back(compare(msg1, msg2));
        }
    }

    if (field1->cpp_type() == field2->cpp_type())
    {
        if (!compare_default_value(field1, field2))
        {
            section.add_item("Changed default value.");
        }
    }

    return section;
}

Comparison::Section Comparison::compare(const Descriptor * desc1, const Descriptor * desc2)
{
    Section section("Comparing messages: "
                    + desc1->full_name() + " -> " + desc2->full_name());

    for (int i = 0; i < desc1->field_count(); ++i)
    {
        auto * field1 = desc1->field(i);
        auto * field2 = desc2->FindFieldByName(field1->name());

        if (field2)
        {
            section.subsections.push_back(compare(field1, field2));
        }
        else
        {
            section.add_item("Removed field: " + field1->name());
        }
    }

    for (int i = 0; i < desc2->field_count(); ++i)
    {
        auto * field2 = desc2->field(i);
        auto * field1 = desc1->FindFieldByName(field2->name());
        if (!field1)
        {
            section.add_item("Added field: " + field2->name());
        }
    }

    return section;

#if 0
    //
    std::set<int> field_number_set;

    for (int i = 0; i < desc1.field_count(); ++i)
    {
        field_number_set.insert(desc1.field(i)->number());
    }

    for (int i = 0; i < desc2.field_count(); ++i)
    {
        field_number_set.insert(desc2.field(i)->number());
    }

    for (int field_number : field_number_set)
    {
        auto field1 = desc1->FindFieldByNumber(field_number);
        auto field2 = desc2->FindFieldByNumber(field_number);
        if (!field1)
        {
            cout << desc2->full_name() << ": added field id: " << field_number << endl;
            continue;
        }
        if (!field2)
        {
            cout << desc2->full_name() << ": removed field id: " << field_number << endl;
            continue;
        }
        compare(field1, field2);
    }
#endif
}

void Comparison::compare(Source & source1, Source & source2)
{
    auto * file1 = source1.file_descriptor();
    auto * file2 = source2.file_descriptor();

    Section & section = root.add_subsection("Comparing files: "
                                            + file1->name() + " -> " + file2->name());

    for (int i = 0; i < file1->message_type_count(); ++i)
    {
        auto * msg1 = file1->message_type(i);
        auto * msg2 = file2->FindMessageTypeByName(msg1->name());
        if (msg2)
        {
            section.subsections.push_back(compare(msg1, msg2));
        }
        else
        {
            section.add_item("Removed message: " + msg1->full_name());
        }
    }

    for (int i = 0; i < file2->message_type_count(); ++i)
    {
        auto * msg2 = file2->message_type(i);
        auto * msg1 = file1->FindMessageTypeByName(msg2->name());
        if (!msg1)
        {
            section.add_item("Added message: " + msg2->full_name());
        }
    }
}

void Comparison::compare(Source & source1, Source & source2, const string & message_name)
{
    auto desc1 = source1.pool()->FindMessageTypeByName(message_name);
    auto desc2 = source2.pool()->FindMessageTypeByName(message_name);

    bool ok = desc1 != nullptr and desc2 != nullptr;

    Section & section =
            root.add_subsection("Comparing message " + message_name + " in files: "
                                + source1.file_descriptor()->name() + " -> "
                                + source2.file_descriptor()->name());

    if (!desc1)
    {
        section.add_item("Message '" + message_name + "' does not appear in source 1.");
    }

    if (!desc2)
    {
        section.add_item("Message '" + message_name + "' does not appear in source 2.");
    }

    if (!ok)
        return;

    section.subsections.push_back(compare(desc1, desc2));
}

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

