#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/descriptor.h>

#include <iostream>
#include <memory>

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

public:
    Source() {}
    Source(const string & file_path, const string & root_dir)
    {
        source_tree.MapPath("", root_dir);

        ErrorCollector error_collector;

        importer = make_shared<Importer>(&source_tree, &error_collector);

        auto * file_descriptor = importer->Import(file_path);
        if (!file_descriptor)
        {
            throw std::runtime_error("Failed to load source.");
        }
    }

    const DescriptorPool * pool() const { return importer->pool(); }

private:
    DiskSourceTree source_tree;
    ErrorCollector error_collector;
    shared_ptr<Importer> importer;
};

void compare(const Descriptor * desc1, const Descriptor * desc2);

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

bool compare_default_value(const FieldDescriptor * field1, const FieldDescriptor * field2)
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

void compare(const EnumDescriptor * enum1, const EnumDescriptor * enum2)
{
    for (int i = 0; i < enum1->value_count(); ++i)
    {
        auto * value1 = enum1->value(i);
        auto * value2 = enum2->FindValueByName(value1->name());

        if (value2)
        {
            if (value1->number() != value2->number())
            {
                cout << "Enum value has different ID: "
                     << enum1->full_name() << " -> " << enum2->full_name() << endl;
            }
        }
        else
        {
            cout << "Enum value removed: " << value1->full_name() << endl;
        }
    }

    for (int i = 0; i < enum2->value_count(); ++i)
    {
        auto * value2 = enum2->value(i);
        auto * value1 = enum1->FindValueByName(value2->name());
        if (!value1)
        {
            cout << "Enum value added: " << value2->full_name() << endl;
        }
    }
}

void compare(const FieldDescriptor * field1, const FieldDescriptor * field2)
{
    print_field(field1);
    cout << " -> ";
    print_field(field2);
    cout << endl;

    if (field1->number() != field2->number())
    {
        cout << "Changed ID." << endl;
    }

    if (field1->type() != field2->type())
    {
        cout << "Changed type." << endl;
    }

    if (field1->type() == FieldDescriptor::TYPE_ENUM)
    {
        auto * enum1 = field1->enum_type();
        auto * enum2 = field2->enum_type();
        if (enum1->full_name() != enum2->full_name())
        {
            cout << "Changed type name." << endl;
        }

        //if (!state.done_comparisons.count({ enum1->full_name(), enum2->full_name() }))
        {
            compare(enum1, enum2);
        }
    }

    if (field1->type() == FieldDescriptor::TYPE_MESSAGE)
    {
        auto * msg1 = field1->message_type();
        auto * msg2 = field2->message_type();

        if (msg1->full_name() != msg2->full_name())
        {
            cout << "Changed type name." << endl;
        }

        //if (!state.done_comparisons.count({ enum1->full_name(), enum2->full_name() }))
        {
            compare(msg1, msg2);
        }
    }

    if (field1->label() != field2->label())
    {
        cout << "Changed label." << endl;
    }

    if (!compare_default_value(field1, field2))
    {
        cout << "Changed default value." << endl;
    }
}

void compare(const Descriptor * desc1, const Descriptor * desc2)
{
    for (int i = 0; i < desc1->field_count(); ++i)
    {
        auto * field1 = desc1->field(i);
        auto * field2 = desc2->FindFieldByName(field1->name());

        if (field2)
        {
            compare(field1, field2);
        }
        else
        {
            cout << "Field removed: " << field1->full_name() << endl;
        }
    }

    for (int i = 0; i < desc2->field_count(); ++i)
    {
        auto * field2 = desc2->field(i);
        auto * field1 = desc1->FindFieldByName(field2->name());
        if (!field1)
        {
            cout << "Field added: " << field2->full_name() << endl;
        }
    }

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

void compare(Source & source1, Source & source2, const string & message_name)
{
    auto desc1 = source1.pool()->FindMessageTypeByName(message_name);
    auto desc2 = source2.pool()->FindMessageTypeByName(message_name);

    bool ok = desc1 != nullptr and desc2 != nullptr;

    if (!desc1)
    {
        cout << "Message '" << message_name << "' does not appear in source 1." << endl;
    }

    if (!desc2)
    {
        cout << "Message '" << message_name << "' does not appear in source 2." << endl;
    }

    if (!ok)
        return;

    compare(desc1, desc2);

    cout << "OK." << endl;
}

int main(int argc, char * argv[])
{
    if (argc < 6)
    {
        cerr << "Missing arguments: file1 root-dir1 file2 root-dir2 message" << endl;
        return 1;
    }

    Source source1;
    Source source2;

    try
    {
        Source source1(argv[1], argv[2]);
        Source source2(argv[3], argv[4]);
        string message_name = argv[5];
        compare(source1, source2, message_name);
    }
    catch(std::exception & e)
    {
        cerr << e.what() << endl;
    }

    return 0;
}

