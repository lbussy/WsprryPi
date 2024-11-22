// This file is released under the GPL v3 License, see <https://www.gnu.org/licenses/>.

/*
 * WsprryPi
 * Updated and maintained by Lee C. Bussy
 *
 * Originally forked from threeme3/WsprryPi (no longer active), this
 * project has been significantly updated, improved, and documented for
 * ease of use.
 *
 * Inspired by a conversation with Bruce Raymond of TAPR, this fork has
 * diverged substantially from its origins and operates as an independent
 * project.
 *
 * Contributors:
 *   - threeme3 (Original Author)
 *   - Bruce Raymond (Inspiration and Guidance)
 *   - Lee Bussy, aa0nt@arrl.net
 *
 * Copyright (C) 2023-2024 Lee C. Bussy (@LBussy). All rights reserved.
 *
 * This code is part of Lee Bussy's WsprryPi project, version 1.2.1-55ad7f3 [fix_57].
*/

#ifndef _INI_READER_HPP
#define _INI_READER_HPP

#include <map>
#include <set>
#include <string>
#include <fstream>

/**
 * @brief A class to parse and manage INI files.
 *
 * This class provides an interface for reading configuration data from
 * INI files, supporting sections, key-value pairs, and optional type
 * conversions. If the INI file does not exist, it creates one with default values.
 */
class INIReader {
public:
    // Constructors
    /**
     * @brief Construct and parse an INI file by filename.
     *
     * This constructor parses the INI file specified by the given filename.
     * If the file does not exist, it creates one with default values and comments.
     *
     * @param filename Path to the INI file.
     */
    explicit INIReader(const std::string& filename);

    /**
     * @brief Construct and parse an INI file from a FILE* stream.
     *
     * This constructor parses an INI file from an already opened FILE* stream.
     *
     * @param file Pointer to the FILE* stream containing the INI file data.
     */
    explicit INIReader(FILE* file);

    // Accessors
    /**
     * @brief Get the parsing error state.
     *
     * @return 0 if parsing succeeded, a positive line number for the first parse error, or -1 for file open errors.
     */
    int ParseError() const;

    /**
     * @brief Get all sections found in the INI file.
     *
     * @return A const reference to a set of section names.
     */
    const std::set<std::string>& Sections() const;

    /**
     * @brief Check if a key exists in a section.
     *
     * This function checks whether the given key exists within the specified section.
     *
     * @param section The name of the section to check.
     * @param name The name of the key to check.
     * @return True if the key exists, otherwise false.
     */
    bool KeyExists(const std::string& section, const std::string& name) const;

    // Value Getters
    /**
     * @brief Retrieve a value as a string.
     *
     * @param section The section name.
     * @param name The key name.
     * @param default_value The default value to return if the key is not found.
     * @return The value associated with the key, or the default value if the key is not found.
     */
    std::string Get(const std::string& section, const std::string& name, const std::string& default_value) const;

    /**
     * @brief Retrieve a value as an integer.
     *
     * @param section The section name.
     * @param name The key name.
     * @param default_value The default value to return if the key is not found or invalid.
     * @return The integer value associated with the key, or the default value if the key is not found or invalid.
     */
    long GetInteger(const std::string& section, const std::string& name, long default_value) const;

    /**
     * @brief Retrieve a value as a double.
     *
     * @param section The section name.
     * @param name The key name.
     * @param default_value The default value to return if the key is not found or invalid.
     * @return The double value associated with the key, or the default value if the key is not found or invalid.
     */
    double GetReal(const std::string& section, const std::string& name, double default_value) const;

    /**
     * @brief Retrieve a value as a float.
     *
     * @param section The section name.
     * @param name The key name.
     * @param default_value The default value to return if the key is not found or invalid.
     * @return The float value associated with the key, or the default value if the key is not found or invalid.
     */
    float GetFloat(const std::string& section, const std::string& name, float default_value) const;

    /**
     * @brief Retrieve a value as a boolean.
     *
     * @param section The section name.
     * @param name The key name.
     * @param default_value The default value to return if the key is not found or invalid.
     * @return True or False based on the key value, or the default value otherwise.
     */
    bool GetBoolean(const std::string& section, const std::string& name, bool default_value) const;

private:
    /**
     * @brief Create the INI file with default values and helpful comments.
     *
     * If the specified INI file does not exist, this method creates it with
     * default values and user-friendly comments to guide manual edits.
     *
     * @param filename Path to the INI file to create.
     */
    void CreateDefaultINI(const std::string& filename);

    /**
     * @brief Create a case-insensitive key from a section and name.
     *
     * Combines the section and key names into a single string for use as a map key.
     * The resulting key is case-insensitive by converting all characters to lowercase.
     *
     * @param section The section name.
     * @param name The key name.
     * @return A combined key in lowercase.
     */
    static std::string MakeKey(const std::string& section, const std::string& name);

    /**
     * @brief Callback handler for ini_parse.
     *
     * This static function serves as a callback for the `ini_parse` function. It
     * processes each section, key, and value encountered during parsing.
     *
     * @param user Pointer to the INIReader instance.
     * @param section The section name being parsed.
     * @param name The key name being parsed.
     * @param value The value associated with the key.
     * @return 1 on success, 0 on failure.
     */
    static int ValueHandler(void* user, const char* section, const char* name, const char* value);

    int _error{0};  ///< Error code from parsing.
    std::map<std::string, std::string> _values;  ///< Parsed key-value pairs.
    std::set<std::string> _sections;  ///< Parsed sections.
};

#endif  // _INI_READER_HPP
