#include "ini_file.hpp"
#include "json.hpp"

#include <iostream>

// For convenience
using json = nlohmann::json;

// INI-Handler instance
IniFile ini;

json create_wsprrypi_config()
{
    // Create the JSON object
    json j;

    j["Meta"] = {
        {"Mode", "WSPR"},
        {"Use INI", "False"},
        {"Date Time Log", "False"},
        {"Loop TX", "False"},
        {"TX Iterations", "0"},
        {"Test Tone", "730000"},
        {"Center Frequency Set", "{}"}};

    j["Common"] = {
        {"Call Sign", "NXXX"},
        {"Frequency", "20m"},
        {"Grid Square", "ZZ99"},
        {"TX Power", "20"},
        {"Transmit Pin", "4"}};

    j["Control"] = {
        {"Transmit", "False"}};

    j["Extended"] = {
        {"LED Pin", "18"},
        {"Offset", "True"},
        {"PPM", "0.0"},
        {"Power Level", "7"},
        {"Use LED", "False"},
        {"Use NTP", "True"}};

    j["Server"] = {
        {"Port", "31415"},
        {"Shutdown Button", "19"},
        {"Use Shutdown", "False"}};

    return j;
}

json get_ini_data(std::string filename)
{
    json patch;
    std::map<std::string, std::unordered_map<std::string, std::string>> ini_data;
    try
    {
        ini.set_filename(filename);
        ini_data = ini.getData();
        // Convert the INI data into JSON.
        // Each section becomes a JSON object with key/value pairs.
        for (const auto &sectionPair : ini_data)
        {
            const std::string &section = sectionPair.first;
            const auto &keyValues = sectionPair.second;
            for (const auto &kv : keyValues)
            {
                patch[section][kv.first] = kv.second;
            }
        }
    }
    catch (const std::exception &ex)
    {
        // std::cerr << "Error: " << ex.what() << std::endl;
    }
    return patch;
}

void set_ini_data(json patch)
{
    if (patch["Meta"]["Use INI"] == "True")
    {
        // Convert JSON back to INI data.
        std::map<std::string, std::unordered_map<std::string, std::string>> newData;
        for (auto &section : patch.items())
        {
            const std::string sectionName = section.key();
            // Ensure the section value is an object
            if (section.value().is_object())
            {
                for (auto &kv : section.value().items())
                {
                    if (kv.value().is_array())
                    {
                        // Convert the array to a string representation using dump()
                        newData[sectionName][kv.key()] = kv.value().dump();
                    }
                    else
                    {
                        newData[sectionName][kv.key()] = kv.value().get<std::string>();
                    }
                }
            }
            else
            {
                // Optionally, handle non-object sections or skip them.
            }
        }

        // Set the new data into the INI file object.
        ini.setData(newData);
        ini.save();
    }
}

int main()
{
    // INI File
    std::string filename = "./wsprrypi.ini";

    // Create base and patch json objects
    json jConfig = create_wsprrypi_config();
    json patchConfig = get_ini_data(filename);

    // Pretty-print the original JSON with an indentation of 4 spaces.
    std::cout << "Base config:" << std::endl;
    std::cout << jConfig.dump(4) << std::endl;
    std::cout << std::endl;

    // Pretty-print the patch JSON with an indentation of 4 spaces.
    std::cout << (patchConfig.empty() ? "Patch config is empty." : "Patch config:") << std::endl;
    if (patchConfig.empty())
    {
        std::cout << std::endl;
    }
    else
    {
        std::cout << patchConfig.dump(4) << std::endl
                  << std::endl;
    }

    try
    {
        // Apply the patch
        if (!patchConfig.empty())
        {
            jConfig["Meta"]["Use INI"] = "True";
            jConfig.merge_patch(patchConfig);
        }
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }

    // Pretty-print the patched JSON with an indentation of 4 spaces.
    std::cout << "Patched config:" << std::endl;
    std::cout << jConfig.dump(4) << std::endl;
    std::cout << std::endl;

    std::vector<double> center_freq_set = {12.2, 123.7, 98754.323};
    jConfig["Meta"]["Center Frequency Set"] = center_freq_set;

    // Pretty-print the JSON after updating Center Frequency Set
    std::cout << "Patched config with Frequencies:" << std::endl;
    std::cout << jConfig.dump(4) << std::endl;
    std::cout << std::endl;
    
    // Write patched data back to INI file (does not create new lines)
    set_ini_data(jConfig);

    return 0;
}
