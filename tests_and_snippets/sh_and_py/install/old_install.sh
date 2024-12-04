do_service() {
    local file_name extension retval
    file_name="$1"
    extension=$(add_dot "$2")
    script_path=$(remove_slash "$3")

    # Handle script install
    check_file "$file_name" "$extension" "$script_path"
    retval="$?"
    if [[ "$retval" == 0 ]]; then
        systemctl stop "$file_name" &> /dev/null
        systemctl disable "$file_name" &> /dev/null
        copy_file "$file_name" "$extension" "$script_path"
        copy_file "$file_name" ".service" "/etc/systemd/system"
        if [[ "$file_name" == "wspr" ]]; then
            do_ini "wspr" "ini" "/usr/local/etc"
            copy_file "logrotate" "conf" "/etc/logrotate.d/"
        fi
        eval "sudo systemctl daemon-reload" &> /dev/null
        systemctl enable "$file_name" &> /dev/null
        eval "systemctl start $file_name" &> /dev/null
    fi
}

copy_file_generic() {
    local file_name extension local_script_path file_path remote_url
    local source_file target_file
    file_name="$1"
    extension=$(remove_dot "$2")
    local_script_path=$(remove_slash "$3")
    file_path=$(remove_slash "$4")
    remote_url=$(remove_slash "$5")

    target_file="$file_path/$file_name.$extension"

    if [ "$USE_LOCAL" == "true" ]; then
        # Use local file
        source_file="$local_script_path/$file_name.$extension"
        if [ ! -f "$source_file" ]; then
            echo "Local file $source_file not found." && die
        fi
        cp "$source_file" "$target_file" || die
    else
        # Download from GitHub
        source_file="$remote_url/$file_name.$extension"
        curl -s "$source_file" > "$target_file" || die
    fi
}

copy_file() {
    local file_name extension script_path file_path remote_url local_scripts_dir
    file_name="$1"
    extension=$(remove_dot "$2")
    script_path=$(remove_slash "$3")
    local_scripts_dir=$(remove_slash "$LOCAL_SCRIPTS_DIR")

    git_raw=$(remove_slash "$GIT_RAW")
    git_repo=$(remove_slash "$GIT_BRCH")
    
    file_path="$script_path/$file_name.$extension"
    remote_url="$git_raw/$git_repo/scripts"

    copy_file_generic "$file_name" "$extension" "$local_scripts_dir" "$file_path" "$remote_url"

    # Set permissions
    chown root:root "$file_path"
    if file "$file_path" | grep -q executable; then
        chmod 0755 "$file_path"
    else
        chmod 0644 "$file_path"
    fi
}

extract_semantic_version() {
    # Function to extract semantic version from a string
    local line="$1"
    echo "$line" | grep -oE '[0-9]+\.[0-9]+\.[0-9]+(-[0-9A-Za-z-]+(\.[0-9A-Za-z-]+)*)?(\+[0-9A-Za-z-]+(\.[0-9A-Za-z-]+)*)?' || echo "unknown"
}

strip_pre_release() {
    # Function to strip unknown pre-release info
    local input_version="$1"

    # Match the base version and allowed pre-release types
    if [[ "$input_version" =~ ^([0-9]+\.[0-9]+\.[0-9]+)(-(alpha|beta|rc[0-9]*|final))?$ ]]; then
        # If it matches, return the base version with allowed pre-release types
        echo "${BASH_REMATCH[1]}${BASH_REMATCH[2]}"
        return 0
    else
        # Otherwise, strip pre-release metadata and append "-development"
        echo "${input_version%%-*}-development"
        return 0
    fi
}

check_file() {
    local file_name script_path verchk yn file_path
    local installed_version stripped_installed_version
    local available_version stripped_available_version
    file_name="$1"
    extension=$(remove_dot "$2")
    file_name="$file_name.$extension"
    script_path=$(remove_slash "$3")
    file_path="$script_path/$file_name"

    if [ -f "$file_path" ]; then
        # Get semantic versions
        installed_version=$("$script_path" -v)
        installed_version=$(extract_semantic_version "$installed_version")
        stripped_installed_version=$(strip_pre_release "$installed_version")
        available_version="$VERSION"
        available_version=$(extract_semantic_version "$available_version")
        stripped_available_version=$(strip_pre_release "$available_version")

        # Compare versions
        if [[ -z "$stripped_installed_version" || -z "$stripped_available_version" ]]; then
            # One or both versions failed to return a proper semantic version
            echo -e "Error: Failed to parse semantic version for:"
            [[ -z "$stripped_installed_version" ]] && echo "- Installed version: $installed_version"
            [[ -z "$stripped_available_version" ]] && echo "- Available version: $available_version"
            read -rp "Proceed with overwrite? [Y/n]: " yn < /dev/tty
            case "$yn" in
                [Nn]* ) return 255 ;;  # Skip Overwrite
                * ) return 0 ;;    # Overwrite (default to Yes)
            esac
        elif [[ "$stripped_installed_version" == *"development"* && "$stripped_available_version" == *"development"* ]]; then
            # Both versions are development versions
            echo -e "\nFile: $file_name exists and both it and the available file are a development version."
            read -rp "Overwrite anyway? [y/N]: " yn < /dev/tty
            case "$yn" in
                [Yy]* ) return 0 ;;  # Overwrite
                * ) return 255 ;;    # Skip overwrite (default to No)
            esac
        elif [[ "$stripped_installed_version" == *"development"* && "$stripped_available_version" != *"development"* ]]; then
            echo -e "\nFile: $file_name exists but is an older version ($installed_version vs. $available_version)." > /dev/tty
            read -rp "Upgrade to newest? [Y/n]: " yn < /dev/tty
            case "$yn" in
                [Nn]* ) return 255 ;;  # Skip Overwrite
                * ) return 0 ;;        # Do overwrite (default to Yes)
            esac
        elif [[ "$stripped_installed_version" != *"development"* && "$stripped_available_version" == *"development"* ]]; then
            echo -e "\nFile: $file_name is newer than the version being installed ($installed_version vs. $available_version)." > /dev/tty
            return 255  # Skip update
        else
            if dpkg --compare-versions "$stripped_installed_version" lt "$stripped_available_version"; then
                echo -e "\nFile: $file_name exists but is an older version ($installed_version vs. $available_version)." > /dev/tty
                read -rp "Upgrade to newest? [Y/n]: " yn < /dev/tty
                case "$yn" in
                    [Nn]* ) return 255 ;;  # Skip update
                    * ) return 0 ;;        # Do overwrite (default to Yes)
                esac
            elif dpkg --compare-versions "$stripped_installed_version" gt "$stripped_available_version"; then
                echo -e "\nFile: $file_name is newer than the version being installed ($installed_version vs. $available_version)." > /dev/tty
                return 255 # Skip update
            else
                echo -e "\nFile: $file_name exists and is the same version as the available version ($installed_version)."
                read -rp "Overwrite anyway? [y/N]: " yn < /dev/tty
                case "$yn" in
                    [Yy]* ) return 0 ;;  # Overwrite
                    * ) return 255 ;;    # Skip overwrite (default to No)
                esac
            fi
        fi
    else
        echo "File: $file_name does not exist. Proceeding with installation." > /dev/tty
        return 0  # File does not exist, proceed with installation
    fi
}

do_ini() {
    local file_name extension path file_path remote_url yn
    file_name="$1"
    extension=$(remove_dot "$2")
    path=$(remove_slash "$3")
    file_path="$path/$file_name.$extension"

    if [ -f "$file_path" ]; then
        echo -e "\nYour configuration can be reset to stock. "
        read -rp "Overwrite? [y/N]: " yn < /dev/tty
        case "$yn" in
            [Yy]* )
                echo "Overwriting configuration file."
                ;;
            * )
                echo "Skipping configuration file setup."
                return
                ;;  # Skip this step
        esac
    else
        echo "Creating configuration file."
    fi
    copy_file "$file_name" "$extension" "$path" # Create ini file
}

do_www() {
    local www_dir local_dir directory git_raw git_repo api_url local_files file
    # Arguments
    www_dir=$(remove_slash "$1")
    local_dir=$(remove_slash "$2")
    # Configuration
    directory="data"
    git_raw=$(remove_slash "$GIT_RAW")
    git_raw="$git_raw/$BRANCH"
    git_repo=$(remove_slash "$GIT_BRCH")

    # Delete old files
    echo -e "\nDeleting old files."
    rm -f "$www_dir/"

    # Copy down web pages
    echo -e "\nChecking and installing website."
    if [ ! -d "$www_dir" ]; then
        mkdir "$www_dir"
    fi
    
    # Get the list of files
    api_url=$(remove_slash "$GIT_API")
    api_url="$api_url/contents/$directory?ref=$BRANCH"

    if [ "$USE_LOCAL" == "true" ]; then
        local_files=$(find "$local_dir" -type f -exec realpath {} \;)
        for file in $local_files; do
            echo "Copying $file..."
            cp "$file" "$www_dir/$file"
        done
    else
        github_files=$(curl -s "$api_url" | jq -r '.[] | select(.type == "file") | .name')
        for file in $github_files; do
            echo "Downloading $file..."
            curl "$git_raw/$directory/$file" -o "$www_dir/$file"
        done
    fi

    echo "Website copy complete."

    # Set the permissions
    echo -e "\nFixing file permissions for $www_dir."
    chown -R www-data:www-data "$www_dir" || die
    find "$www_dir" -type d -exec chmod 2770 {} \; || die
    find "$www_dir" -type f -exec chmod 660 {} \; || die
}

do_shutdown_button() {
    local yn file_name extension script_path
    file_name="$1"
    extension=$(add_dot "$2")
    script_path=$(remove_slash "$3")
    echo
    # Check if the shutdown_watch service is already enabled
    if systemctl is-enabled shutdown_watch.service &>/dev/null; then
        echo "TAPR shutdown button support is already enabled."
        read -rp "Do you want to disable TAPR shutdown button support? [y/N]: " yn < /dev/tty
        case "$yn" in
            [Yy]* )
                echo "Disabling TAPR shutdown button support."
                systemctl stop shutdown_watch.service || true
                systemctl disable shutdown_watch.service || true
                rm -f /etc/systemd/system/shutdown_watch.service
                echo "Reloading systemd daemon configuration."
                systemctl daemon-reload
                return
                ;;
            * )
                echo "Keeping TAPR shutdown button support enabled."
                ;;
        esac
    else
        # Prompt user to enable support
        read -rp "Support system shutdown button (TAPR)? [y/N]: " yn < /dev/tty
        case "$yn" in
            [Yy]* )
                echo "Enabling TAPR shutdown button support."
                ;;
            * )
                echo "TAPR shutdown button support remains disabled."
                return
                ;;
        esac
    fi
    do_service "shutdown_watch" ".py" "/usr/local/bin" # Upgrade the service
}
