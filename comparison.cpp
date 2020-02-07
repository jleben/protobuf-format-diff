#include "comparison.h"

#include <iostream>
#include <string>

using namespace std;

string Comparison::Item::message() const
{
    string msg;

    switch (type)
    {
    case Enum_Value_Name_Changed:
        msg = "Value name changed";
        break;
    case Enum_Value_Id_Changed:
        msg = "Value ID changed";
        break;
    case Enum_Value_Added:
        msg = "Value added";
        break;
    case Enum_Value_Removed:
        msg = "Value removed";
        break;
    case Message_Field_Name_Changed:
        msg = "Name changed";
        break;
    case Message_Field_Id_Changed:
        msg = "ID changed";
        break;
    case Message_Field_Label_Changed:
        msg = "Label changed";
        break;
    case Message_Field_Type_Changed:
        msg = "Type changed";
        break;
    case Message_Field_Default_Value_Changed:
        msg = "Default value changed";
        break;
    case Message_Field_Added:
        msg = "Field added";
        break;
    case Message_Field_Removed:
        msg = "Field removed";
        break;
    case File_Message_Added:
        msg = "Message added";
        break;
    case File_Message_Removed:
        msg = "Message removed";
        break;
    case File_Enum_Added:
        msg = "Enum added";
        break;
    case File_Enum_Removed:
        msg = "Enum removed";
        break;
    case Name_Missing:
        msg = "Name missing";
        break;
    default:
        msg = "?";
        return msg;
    }

    msg += ": " + a + " -> " + b;

    return msg;
}

string Comparison::Section::message() const
{
    ostringstream msg;

    switch(type)
    {
    case Root_Section:
        msg << "/";
        break;
    case Message_Comparison:
        msg << "Comparing messages: " << a << " -> " << b;
        break;
    case Message_Field_Comparison:
        msg << "Comparing fields: " << a << " -> " << b;
        break;
    case Enum_Comparison:
        msg << "Comparing enums: " << a << " -> " << b;
        break;
    case Enum_Value_Comparison:
        msg << "Comparing enum values: " << a << " -> " << b;
        break;
    default:
        msg << "?";
    }

    return msg.str();
}

static
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

void Comparison::Section::print(int level)
{
    cout << string(level*2, ' ') << message() << endl;

    ++level;

    string subprefix(level*2, ' ');

    for (auto & note : notes)
    {
        cout << subprefix << note << endl;
    }

    for (auto & item : items)
    {
        cout << subprefix << "* " << item.message() << endl;
    }

    for (auto & subsection : subsections)
    {
        subsection.print(level);
    }
}

Comparison::Comparison(const Options & options):
    options(options)
{}

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

Comparison::Section * Comparison::compare(const EnumDescriptor * enum1, const EnumDescriptor * enum2)
{
    string key = enum1->full_name() + ":" + enum2->full_name();
    if (compared.count(key))
        return compared.at(key);

    auto & section = root.add_subsection(Enum_Comparison, enum1->full_name(), enum2->full_name());
    compared.emplace(key, &section);

    for (int i = 0; i < enum1->value_count(); ++i)
    {
        auto * value1 = enum1->value(i);
        auto * value2 = options.binary ?
                    enum2->FindValueByNumber(value1->number()) :
                    enum2->FindValueByName(value1->name());

        if (value2)
        {
            auto & subsection = section.add_subsection(Enum_Value_Comparison, value1->name(), value2->name());

            if (value1->number() != value2->number())
            {
                subsection.add_item(Enum_Value_Id_Changed,
                                    to_string(value1->number()), to_string(value2->number()));
            }
            if (value1->name() != value2->name())
            {
                subsection.add_item(Enum_Value_Name_Changed,
                                    value1->name(), value2->name());
            }
        }
        else
        {
            string value1_id = options.binary ? to_string(value1->number()) : value1->name();
            section.add_item(Enum_Value_Removed, value1_id, "");
        }
    }

    for (int i = 0; i < enum2->value_count(); ++i)
    {
        auto * value2 = enum2->value(i);
        auto * value1 = options.binary ?
                    enum1->FindValueByNumber(value2->number()) :
                    enum1->FindValueByName(value2->name());

        if (!value1)
        {
            string value2_id = options.binary ? to_string(value2->number()) : value2->name();
            section.add_item(Enum_Value_Added, "", value2_id);
        }
    }

    return &section;
}

Comparison::Section Comparison::compare(const FieldDescriptor * field1, const FieldDescriptor * field2)
{
    Section section(Message_Field_Comparison, field1->name(), field2->name());

    if (field1->name() != field2->name())
    {
        section.add_item(Message_Field_Name_Changed, field1->name(), field2->name());
    }

    if (field1->number() != field2->number())
    {
        section.add_item(Message_Field_Id_Changed, to_string(field1->number()), to_string(field2->number()));
    }

    if (field1->label() != field2->label())
    {
        section.add_item(Message_Field_Label_Changed, "", "");
    }

    if (field1->type() != field2->type())
    {
        section.add_item(Message_Field_Type_Changed, field1->type_name(), field2->type_name());
    }
    else if (field1->type() == FieldDescriptor::TYPE_ENUM)
    {
        auto * enum1 = field1->enum_type();
        auto * enum2 = field2->enum_type();

        auto * type_comparison = compare(enum1, enum2);
        type_comparison->notes.push_back("Required by " + field1->full_name() + " -> " + field2->full_name());

        type_comparison->trim();
        if (!type_comparison->is_empty())
        {
            section.add_item(Message_Field_Type_Changed, enum1->full_name(), enum2->full_name());
        }
    }
    else if (field1->type() == FieldDescriptor::TYPE_MESSAGE)
    {
        auto * msg1 = field1->message_type();
        auto * msg2 = field2->message_type();

        auto * type_comparison = compare(msg1, msg2);
        type_comparison->notes.push_back("Required by " + field1->full_name() + " -> " + field2->full_name());

        type_comparison->trim();
        if (!type_comparison->is_empty())
        {
            section.add_item(Message_Field_Type_Changed, msg1->full_name(), msg2->full_name());
        }
    }

    if (field1->cpp_type() == field2->cpp_type())
    {
        if (!compare_default_value(field1, field2))
        {
            section.add_item(Message_Field_Default_Value_Changed, "", "");
        }
    }

    return section;
}

Comparison::Section * Comparison::compare(const Descriptor * desc1, const Descriptor * desc2)
{
    string key = desc1->full_name() + ":" + desc2->full_name();
    if (compared.count(key))
        return compared.at(key);

    auto & section = root.add_subsection(Message_Comparison, desc1->full_name(), desc2->full_name());
    compared.emplace(key, &section);


    for (int i = 0; i < desc1->field_count(); ++i)
    {
        auto * field1 = desc1->field(i);
        auto * field2 = options.binary ?
                    desc2->FindFieldByNumber(field1->number()) :
                    desc2->FindFieldByName(field1->name());

        if (field2)
        {
            section.subsections.push_back(compare(field1, field2));
        }
        else
        {
            string field1_id = options.binary ? to_string(field1->number()) : field1->name();
            section.add_item(Message_Field_Removed, field1_id, "");
        }
    }

    for (int i = 0; i < desc2->field_count(); ++i)
    {
        auto * field2 = desc2->field(i);
        auto * field1 =  options.binary ?
                    desc1->FindFieldByNumber(field2->number()) :
                    desc1->FindFieldByName(field2->name());

        if (!field1)
        {
            string field2_id = options.binary ? to_string(field2->number()) : field2->name();
            section.add_item(Message_Field_Added, "", field2_id);
        }
    }

    return &section;
}

void Comparison::compare(Source & source1, Source & source2)
{
    auto * file1 = source1.file_descriptor();
    auto * file2 = source2.file_descriptor();

    for (int i = 0; i < file1->message_type_count(); ++i)
    {
        auto * msg1 = file1->message_type(i);
        auto * msg2 = file2->FindMessageTypeByName(msg1->name());
        if (msg2)
        {
            compare(msg1, msg2);
        }
        else
        {
            root.add_item(File_Message_Removed, msg1->full_name(), "");
        }
    }

    for (int i = 0; i < file2->message_type_count(); ++i)
    {
        auto * msg2 = file2->message_type(i);
        auto * msg1 = file1->FindMessageTypeByName(msg2->name());
        if (!msg1)
        {
            root.add_item(File_Message_Added, "", msg2->full_name());
        }
    }

    for (int i = 0; i < file1->enum_type_count(); ++i)
    {
        auto * enum1 = file1->enum_type(i);
        auto * enum2 = file2->FindEnumTypeByName(enum1->name());
        if (enum2)
        {
            compare(enum1, enum2);
        }
        else
        {
            root.add_item(File_Enum_Removed, enum1->full_name(), "");
        }
    }

    for (int i = 0; i < file2->enum_type_count(); ++i)
    {
        auto * enum2 = file2->enum_type(i);
        auto * enum1 = file1->FindEnumTypeByName(enum2->name());
        if (!enum1)
        {
            root.add_item(File_Enum_Added, "", enum2->full_name());
        }
    }
}


void Comparison::compare(Source & source1, const string & name1, Source & source2, const string &name2)
{
    auto desc1 = source1.pool()->FindMessageTypeByName(name1);
    auto desc2 = source2.pool()->FindMessageTypeByName(name2);

    auto enum1 = source1.pool()->FindEnumTypeByName(name1);
    auto enum2 = source2.pool()->FindEnumTypeByName(name2);

    if (desc1 and desc2)
    {
        compare(desc1, desc2);
    }
    else if (enum1 and enum2)
    {
        compare(enum1, enum2);
    }
    else
    {
        root.add_item(Name_Missing, name1, name2);
    }
}

