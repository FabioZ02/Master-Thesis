#include "BT_Data.hh"
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

BT_Input::BT_Input(string file_name)
{
    ifstream is(file_name);
    if(!is)
    {
        cerr <<"Cannot open input file " << file_name << endl
        exit(1):
    }

    json j;
    file >> j;

    o


}