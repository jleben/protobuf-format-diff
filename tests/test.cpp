#include "../json/json.hpp"
#include "../comparison.h"

#include <iostream>
#include <fstream>

using nlohmann::json;
using namespace std;

void confirm(bool value, const string & what)
{
    if (value)
        cerr << "OK: " << what << endl;
    else
        throw std::runtime_error(what);
}

string section_type_string(Comparison::SectionType type)
{
    switch(type)
    {
    case Comparison::Root_Section:
        return "/";
    case Comparison::Message_Comparison:
        return "message_comparison";
    case Comparison::Message_Field_Comparison:
        return "message_field_comparison";
    case Comparison::Enum_Comparison:
        return "enum_comparison";
    case Comparison::Enum_Value_Comparison:
        return "enum_value_comparison";
    default:
        throw std::runtime_error("Unexpected section type.");
    }
}

string item_type_string(Comparison::ItemType type)
{
    switch (type)
    {
    case Comparison::Enum_Value_Id_Changed:
        return "enum_value_id_changed";
    case Comparison::Enum_Value_Added:
        return "enum_value_added";
    case Comparison::Enum_Value_Removed:
        return "enum_value_removed";
    case Comparison::Message_Field_Name_Changed:
        return "message_field_name_changed";
    case Comparison::Message_Field_Id_Changed:
        return "message_field_id_changed";
    case Comparison::Message_Field_Label_Changed:
        return "message_field_label_changed";
    case Comparison::Message_Field_Type_Changed:
        return "message_field_type_changed";
    case Comparison::Message_Field_Default_Value_Changed:
        return "message_field_default_value_changed";
    case Comparison::Message_Field_Added:
        return "message_field_added";
    case Comparison::Message_Field_Removed:
        return "message_field_removed";
    case Comparison::File_Message_Added:
        return "file_message_added";
    case Comparison::File_Message_Removed:
        return "file_message_removed";
    case Comparison::File_Enum_Added:
        return "file_enum_added";
    case Comparison::File_Enum_Removed:
        return "file_enum_removed";
    case Comparison::Name_Missing:
        return "name_missing";
    default:
        throw std::runtime_error("Unexpected item type.");
    }
}

void verify(const Comparison::Item & item, json & expected)
{
    string expected_type = expected["type"];
    confirm(item_type_string(item.type) == expected_type,
            "Item type " + item_type_string(item.type) + " = " + expected_type);

    string expected_a = expected["a"];
    confirm(item.a == expected_a, "Item side A: '" + item.a + "' = '" + expected_a + "'");

    string expected_b = expected["b"];
    confirm(item.b == expected_b, "Item side B: '" + item.b + "' = " + expected_b + "'");
}

void verify(const Comparison::Section & section, json & expected)
{
    confirm(expected.is_object(), "JSON is an object.");

    string expected_type = expected["type"];
    confirm(section_type_string(section.type) == expected_type, "Section type = " + expected_type);

    if (!section.a.empty())
    {
        string expected_a = expected["a"];
        confirm(section.a == expected_a, "Section side A = " + expected_a);
    }

    if (!section.b.empty())
    {
        string expected_b = expected["b"];
        confirm(section.b == expected_b, "Section side B = " + expected_b);
    }

    auto & expected_items = expected["items"];

    confirm(section.items.size() == expected_items.size(),
            "Number of items = " + to_string(expected_items.size()));

    if (section.items.size())
    {
        confirm(expected_items.is_array(), "JSON has array of items.");

        auto item_it = section.items.begin();
        auto expected_item_it = expected_items.begin();
        while (item_it != section.items.end())
        {
            verify(*item_it, *expected_item_it);
            ++item_it;
            ++expected_item_it;
        }
    }

    auto & expected_subsections = expected["sections"];

    confirm(section.subsections.size() == expected_subsections.size(),
            "Number of subsections = " + to_string(expected_subsections.size()));

    if (section.subsections.size())
    {
        confirm(expected_subsections.is_array(), "JSON has array of subsections.");

        auto section_it = section.subsections.begin();
        auto expected_section_it = expected_subsections.begin();
        while (section_it != section.subsections.end())
        {
            verify(*section_it, *expected_section_it);
            ++section_it;
            ++expected_section_it;
        }
    }
}

void verify(const Comparison & comparison, json & expected)
{
    verify(comparison.root, expected);
}

int main(int argc, char * argv[])
{
    if (argc < 2)
    {
        cerr << "Expected argument: <test directory>" << endl;
        return 1;
    }

    string test_path(argv[1]);

    Comparison comparison;

    try
    {
        Source source_a("a.proto", test_path);
        Source source_b("b.proto", test_path);
        comparison.compare(source_a, source_b);
    }
    catch (std::exception & e)
    {
        cerr << "Error while comparing: " << e.what() << endl;
        return 1;
    }

    json expected;

    try
    {
        string diff_path = test_path + "/diff.json";
        ifstream diff_file(diff_path);
        if (!diff_file.is_open())
        {
            cerr << "Failed to open diff file: " << diff_path << endl;
            return 1;
        }

        diff_file >> expected;
    }
    catch (json::exception & e)
    {
        cerr << "Failed to parse diff file: " << e.what() << endl;
        return 1;
    }

    comparison.root.trim();

    try
    {
        verify(comparison, expected);
    }
    catch (std::exception & e)
    {
        cerr << "Failed to verify: " << e.what() << endl;
        return 1;
    }

    cerr << "OK." << endl;
}
