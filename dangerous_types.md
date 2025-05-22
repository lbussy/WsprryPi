# Data Types for 64-Bit Conversion Consideration

Using:

```bash
grep -RPn \
  --include='*.c' \
  --include='*.cpp' \
  --include='*.h' \
  --include='*.hpp' \
  '^(?!\s*(//|/\*|\*)).*\b(?:uintptr_t|unsigned long|long|int)\b' \
  .
```

We see:

```cpp
./src/PPM-Manager/src/ppm_manager.cpp:141:bool PPMManager::setPriority(int schedPolicy, int priority)
./src/PPM-Manager/src/ppm_manager.cpp:154:    int ret = pthread_setschedparam(ppm_thread_.native_handle(), schedPolicy, &sch_params);
./src/PPM-Manager/src/ppm_manager.cpp:204:    int ret = std::system("systemctl is-active --quiet chronyd");
./src/PPM-Manager/src/ppm_manager.cpp:258:double PPMManager::measure_clock_drift(int seconds)
./src/PPM-Manager/src/ppm_manager.cpp:266:    const int check_interval = 1;
./src/PPM-Manager/src/ppm_manager.cpp:267:    int elapsed_seconds = 0;
./src/PPM-Manager/src/ppm_manager.cpp:324:PPMStatus PPMManager::ppm_update_loop(int interval_seconds)
./src/PPM-Manager/src/ppm_manager.cpp:326:    const int check_interval = 1;
./src/PPM-Manager/src/ppm_manager.cpp:379:        for (int i = 0; i < interval_seconds; i += check_interval)
./src/PPM-Manager/src/ppm_manager.cpp:405:    int policy = SCHED_RR; // Round-robin scheduling; alternatives include SCHED_FIFO
./src/PPM-Manager/src/ppm_manager.cpp:410:    int ret = pthread_setschedparam(ppm_thread_.native_handle(), policy, &sch_params);
./src/PPM-Manager/src/main.cpp:37:void signal_handler(int signum)
./src/PPM-Manager/src/main.cpp:54:void worker_thread(int id)
./src/PPM-Manager/src/main.cpp:60:        for (volatile int i = 0; i < 1000000; ++i)
./src/PPM-Manager/src/main.cpp:76:int main()
./src/PPM-Manager/src/main.cpp:111:    const int num_workers = 4;
./src/PPM-Manager/src/main.cpp:114:    for (int i = 0; i < num_workers; ++i)
./src/PPM-Manager/src/ppm_manager.hpp:97:    bool setPriority(int schedPolicy, int priority);
./src/PPM-Manager/src/ppm_manager.hpp:144:    static constexpr int clock_drift_interval_ = 300; ///< Interval in seconds for measuring clock drift.
./src/PPM-Manager/src/ppm_manager.hpp:145:    static constexpr int ppm_update_interval_ = 120;  ///< Interval in seconds between PPM updates.
./src/PPM-Manager/src/ppm_manager.hpp:146:    static constexpr int ppm_loop_priority_ = 10;     ///< Default scheduling priority.
./src/PPM-Manager/src/ppm_manager.hpp:167:    double measure_clock_drift(int seconds = clock_drift_interval_);
./src/PPM-Manager/src/ppm_manager.hpp:186:    PPMStatus ppm_update_loop(int interval_seconds);
./src/version.cpp:185:    int value;        ///< Integer value representing the processor type.
./src/version.cpp:293:int get_processor_type_int()
./src/INI-Handler/src/main.cpp:44:        int txPower = config.get_int_value("Common", "TX Power");
./src/INI-Handler/src/main.cpp:205:int main()
./src/INI-Handler/src/ini_file.cpp:251:int IniFile::get_int_value(const std::string &section, const std::string &key) const
./src/INI-Handler/src/ini_file.cpp:361:void IniFile::set_int_value(const std::string &section, const std::string &key, int value)
./src/INI-Handler/src/ini_file.hpp:103:    int get_int_value(const std::string &section, const std::string &key) const;
./src/INI-Handler/src/ini_file.hpp:117:    void set_int_value(const std::string &section, const std::string &key, int value);
./src/gpio_output.hpp:81:    bool enableGPIOPin(int pin, bool active_high = true);
./src/gpio_output.hpp:102:    int pin_;
./src/gpio_output.hpp:111:    int compute_physical_state(bool logical_state) const;
./src/config_handler.hpp:86:    int power_dbm;           ///< Transmit power in dBm.
./src/config_handler.hpp:88:    int tx_pin;              ///< GPIO pin number for RF transmit control.
./src/config_handler.hpp:94:    int power_level; ///< Power level for RF hardware (0–7).
./src/config_handler.hpp:96:    int led_pin;     ///< GPIO pin for LED indicator.
./src/config_handler.hpp:99:    int web_port;      ///< Web server port number.
./src/config_handler.hpp:100:    int socket_port;   ///< Socket server port number.
./src/config_handler.hpp:102:    int shutdown_pin;  ///< GPIO pin used to signal shutdown.
./src/config_handler.hpp:107:    std::atomic<int> tx_iterations; ///< Number of transmission iterations (0 = infinite).
./src/json.hpp:2112:    #define JSON_HEDLEY_IS_CONSTEXPR_(expr) __builtin_types_compatible_p(__typeof__((1 ? (void*) ((__INTPTR_TYPE__) ((expr) * 0)) : (int*) 0)), int*)
./src/json.hpp:2115:    #define JSON_HEDLEY_IS_CONSTEXPR_(expr) __builtin_types_compatible_p(__typeof__((1 ? (void*) ((intptr_t) ((expr) * 0)) : (int*) 0)), int*)
./src/json.hpp:2129:    #define JSON_HEDLEY_IS_CONSTEXPR_(expr) _Generic((1 ? (void*) ((__INTPTR_TYPE__) ((expr) * 0)) : (int*) 0), int*: 1, void*: 0)
./src/json.hpp:2132:    #define JSON_HEDLEY_IS_CONSTEXPR_(expr) _Generic((1 ? (void*) ((intptr_t) * 0) : (int*) 0), int*: 1, void*: 0)
./src/json.hpp:2773:    template<typename BasicJsonType, nlohmann::detail::enable_if_t<nlohmann::detail::is_basic_json<BasicJsonType>::value, int> = 0> \
./src/json.hpp:2775:    template<typename BasicJsonType, nlohmann::detail::enable_if_t<nlohmann::detail::is_basic_json<BasicJsonType>::value, int> = 0> \
./src/json.hpp:2785:    template<typename BasicJsonType, nlohmann::detail::enable_if_t<nlohmann::detail::is_basic_json<BasicJsonType>::value, int> = 0> \
./src/json.hpp:2787:    template<typename BasicJsonType, nlohmann::detail::enable_if_t<nlohmann::detail::is_basic_json<BasicJsonType>::value, int> = 0> \
./src/json.hpp:2797:    template<typename BasicJsonType, nlohmann::detail::enable_if_t<nlohmann::detail::is_basic_json<BasicJsonType>::value, int> = 0> \
./src/json.hpp:2807:    template<typename BasicJsonType, nlohmann::detail::enable_if_t<nlohmann::detail::is_basic_json<BasicJsonType>::value, int> = 0> \
./src/json.hpp:2809:    template<typename BasicJsonType, nlohmann::detail::enable_if_t<nlohmann::detail::is_basic_json<BasicJsonType>::value, int> = 0> \
./src/json.hpp:2819:    template<typename BasicJsonType, nlohmann::detail::enable_if_t<nlohmann::detail::is_basic_json<BasicJsonType>::value, int> = 0> \
./src/json.hpp:2821:    template<typename BasicJsonType, nlohmann::detail::enable_if_t<nlohmann::detail::is_basic_json<BasicJsonType>::value, int> = 0> \
./src/json.hpp:2831:    template<typename BasicJsonType, nlohmann::detail::enable_if_t<nlohmann::detail::is_basic_json<BasicJsonType>::value, int> = 0> \
./src/json.hpp:2841:    template<typename BasicJsonType, nlohmann::detail::enable_if_t<nlohmann::detail::is_basic_json<BasicJsonType>::value, int> = 0> \
./src/json.hpp:2843:    template<typename BasicJsonType, nlohmann::detail::enable_if_t<nlohmann::detail::is_basic_json<BasicJsonType>::value, int> = 0> \
./src/json.hpp:2853:    template<typename BasicJsonType, nlohmann::detail::enable_if_t<nlohmann::detail::is_basic_json<BasicJsonType>::value, int> = 0> \
./src/json.hpp:2855:    template<typename BasicJsonType, nlohmann::detail::enable_if_t<nlohmann::detail::is_basic_json<BasicJsonType>::value, int> = 0> \
./src/json.hpp:2865:    template<typename BasicJsonType, nlohmann::detail::enable_if_t<nlohmann::detail::is_basic_json<BasicJsonType>::value, int> = 0> \
./src/json.hpp:2875:    template<typename BasicJsonType, nlohmann::detail::enable_if_t<nlohmann::detail::is_basic_json<BasicJsonType>::value, int> = 0> \
./src/json.hpp:2877:    template<typename BasicJsonType, nlohmann::detail::enable_if_t<nlohmann::detail::is_basic_json<BasicJsonType>::value, int> = 0> \
./src/json.hpp:2887:    template<typename BasicJsonType, nlohmann::detail::enable_if_t<nlohmann::detail::is_basic_json<BasicJsonType>::value, int> = 0> \
./src/json.hpp:2889:    template<typename BasicJsonType, nlohmann::detail::enable_if_t<nlohmann::detail::is_basic_json<BasicJsonType>::value, int> = 0> \
./src/json.hpp:2899:    template<typename BasicJsonType, nlohmann::detail::enable_if_t<nlohmann::detail::is_basic_json<BasicJsonType>::value, int> = 0> \
./src/json.hpp:4171:template < typename T, typename U, enable_if_t < !std::is_same<T, U>::value, int > = 0 >
./src/json.hpp:4177:template<typename T, typename U, enable_if_t<std::is_same<T, U>::value, int> = 0>
./src/json.hpp:4416:                         && detect_string_can_append_op<OutStringType, Arg>::value, int > = 0 >
./src/json.hpp:4422:                         && detect_string_can_append_iter<OutStringType, Arg>::value, int > = 0 >
./src/json.hpp:4429:                         && detect_string_can_append_data<OutStringType, Arg>::value, int > = 0 >
./src/json.hpp:4433:         enable_if_t<detect_string_can_append<OutStringType, Arg>::value, int> = 0>
./src/json.hpp:4442:                         && detect_string_can_append_op<OutStringType, Arg>::value, int > >
./src/json.hpp:4452:                         && detect_string_can_append_iter<OutStringType, Arg>::value, int > >
./src/json.hpp:4463:                         && detect_string_can_append_data<OutStringType, Arg>::value, int > >
./src/json.hpp:4515:    const int id; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
./src/json.hpp:4519:    exception(int id_, const char* what_arg) : id(id_), m(what_arg) {} // NOLINT(bugprone-throw-keyword-missing)
./src/json.hpp:4521:    static std::string name(const std::string& ename, int id_)
./src/json.hpp:4633:    template<typename BasicJsonContext, enable_if_t<is_basic_json_context<BasicJsonContext>::value, int> = 0>
./src/json.hpp:4634:    static parse_error create(int id_, const position_t& pos, const std::string& what_arg, BasicJsonContext context)
./src/json.hpp:4641:    template<typename BasicJsonContext, enable_if_t<is_basic_json_context<BasicJsonContext>::value, int> = 0>
./src/json.hpp:4642:    static parse_error create(int id_, std::size_t byte_, const std::string& what_arg, BasicJsonContext context)
./src/json.hpp:4662:    parse_error(int id_, std::size_t byte_, const char* what_arg)
./src/json.hpp:4677:    template<typename BasicJsonContext, enable_if_t<is_basic_json_context<BasicJsonContext>::value, int> = 0>
./src/json.hpp:4678:    static invalid_iterator create(int id_, const std::string& what_arg, BasicJsonContext context)
./src/json.hpp:4686:    invalid_iterator(int id_, const char* what_arg)
./src/json.hpp:4695:    template<typename BasicJsonContext, enable_if_t<is_basic_json_context<BasicJsonContext>::value, int> = 0>
./src/json.hpp:4696:    static type_error create(int id_, const std::string& what_arg, BasicJsonContext context)
./src/json.hpp:4704:    type_error(int id_, const char* what_arg) : exception(id_, what_arg) {}
./src/json.hpp:4712:    template<typename BasicJsonContext, enable_if_t<is_basic_json_context<BasicJsonContext>::value, int> = 0>
./src/json.hpp:4713:    static out_of_range create(int id_, const std::string& what_arg, BasicJsonContext context)
./src/json.hpp:4721:    out_of_range(int id_, const char* what_arg) : exception(id_, what_arg) {}
./src/json.hpp:4729:    template<typename BasicJsonContext, enable_if_t<is_basic_json_context<BasicJsonContext>::value, int> = 0>
./src/json.hpp:4730:    static other_error create(int id_, const std::string& what_arg, BasicJsonContext context)
./src/json.hpp:4738:    other_error(int id_, const char* what_arg) : exception(id_, what_arg) {}
./src/json.hpp:4851:                         int > = 0 >
./src/json.hpp:4910:        && !is_json_ref<StringType>::value, int > = 0 >
./src/json.hpp:4941:         enable_if_t<std::is_enum<EnumType>::value, int> = 0>
./src/json.hpp:4952:         enable_if_t<is_getable<BasicJsonType, T>::value, int> = 0>
./src/json.hpp:4969:         enable_if_t<is_getable<BasicJsonType, T>::value, int> = 0>
./src/json.hpp:5062:             int> = 0>
./src/json.hpp:5086:             int> = 0>
./src/json.hpp:5111:               int > = 0 >
./src/json.hpp:5156:         enable_if_t<is_constructible_object_type<BasicJsonType, ConstructibleObjectType>::value, int> = 0>
./src/json.hpp:5188:               int > = 0 >
./src/json.hpp:5508:    iteration_proxy_value operator++(int)& // NOLINT(cert-dcl21-cpp)
./src/json.hpp:5607:template<std::size_t N, typename IteratorType, enable_if_t<N == 0, int> = 0>
./src/json.hpp:5615:template<std::size_t N, typename IteratorType, enable_if_t<N == 1, int> = 0>
./src/json.hpp:5721:                             int > = 0 >
./src/json.hpp:5817:                             int > = 0 >
./src/json.hpp:5846:             enable_if_t<std::is_convertible<T, BasicJsonType>::value, int> = 0>
./src/json.hpp:5886:               enable_if_t < !std::is_same<CompatibleObjectType, typename BasicJsonType::object_t>::value, int > = 0 >
./src/json.hpp:5906:         enable_if_t<std::is_constructible<BasicJsonType, T>::value, int> = 0>
./src/json.hpp:5921:         enable_if_t<std::is_same<T, typename BasicJsonType::boolean_t>::value, int> = 0>
./src/json.hpp:5934:               && std::is_convertible<const BoolRef&, typename BasicJsonType::boolean_t>::value, int > = 0 >
./src/json.hpp:5941:         enable_if_t<std::is_constructible<typename BasicJsonType::string_t, CompatibleString>::value, int> = 0>
./src/json.hpp:5954:         enable_if_t<std::is_floating_point<FloatType>::value, int> = 0>
./src/json.hpp:5961:         enable_if_t<is_compatible_integer_type<typename BasicJsonType::number_unsigned_t, CompatibleNumberUnsignedType>::value, int> = 0>
./src/json.hpp:5968:         enable_if_t<is_compatible_integer_type<typename BasicJsonType::number_integer_t, CompatibleNumberIntegerType>::value, int> = 0>
./src/json.hpp:5976:         enable_if_t<std::is_enum<EnumType>::value, int> = 0>
./src/json.hpp:5998:                         int > = 0 >
./src/json.hpp:6011:         enable_if_t<std::is_convertible<T, BasicJsonType>::value, int> = 0>
./src/json.hpp:6024:           enable_if_t < is_compatible_object_type<BasicJsonType, CompatibleObjectType>::value&& !is_basic_json<CompatibleObjectType>::value, int > = 0 >
./src/json.hpp:6040:                  int > = 0 >
./src/json.hpp:6046:template < typename BasicJsonType, typename T1, typename T2, enable_if_t < std::is_constructible<BasicJsonType, T1>::value&& std::is_constructible<BasicJsonType, T2>::value, int > = 0 >
./src/json.hpp:6054:         enable_if_t<std::is_same<T, iteration_proxy_value<typename BasicJsonType::iterator>>::value, int> = 0>
./src/json.hpp:6073:template<typename BasicJsonType, typename T, enable_if_t<is_constructible_tuple<BasicJsonType, T>::value, int > = 0>
./src/json.hpp:6677:                utf8_bytes[0] = static_cast<std::char_traits<char>::int_type>(0xC0u | ((static_cast<unsigned int>(wc) >> 6u) & 0x1Fu));
./src/json.hpp:6678:                utf8_bytes[1] = static_cast<std::char_traits<char>::int_type>(0x80u | (static_cast<unsigned int>(wc) & 0x3Fu));
./src/json.hpp:6683:                utf8_bytes[0] = static_cast<std::char_traits<char>::int_type>(0xE0u | ((static_cast<unsigned int>(wc) >> 12u) & 0x0Fu));
./src/json.hpp:6684:                utf8_bytes[1] = static_cast<std::char_traits<char>::int_type>(0x80u | ((static_cast<unsigned int>(wc) >> 6u) & 0x3Fu));
./src/json.hpp:6685:                utf8_bytes[2] = static_cast<std::char_traits<char>::int_type>(0x80u | (static_cast<unsigned int>(wc) & 0x3Fu));
./src/json.hpp:6690:                utf8_bytes[0] = static_cast<std::char_traits<char>::int_type>(0xF0u | ((static_cast<unsigned int>(wc) >> 18u) & 0x07u));
./src/json.hpp:6691:                utf8_bytes[1] = static_cast<std::char_traits<char>::int_type>(0x80u | ((static_cast<unsigned int>(wc) >> 12u) & 0x3Fu));
./src/json.hpp:6692:                utf8_bytes[2] = static_cast<std::char_traits<char>::int_type>(0x80u | ((static_cast<unsigned int>(wc) >> 6u) & 0x3Fu));
./src/json.hpp:6693:                utf8_bytes[3] = static_cast<std::char_traits<char>::int_type>(0x80u | (static_cast<unsigned int>(wc) & 0x3Fu));
./src/json.hpp:6735:                utf8_bytes[0] = static_cast<std::char_traits<char>::int_type>(0xC0u | ((static_cast<unsigned int>(wc) >> 6u)));
./src/json.hpp:6736:                utf8_bytes[1] = static_cast<std::char_traits<char>::int_type>(0x80u | (static_cast<unsigned int>(wc) & 0x3Fu));
./src/json.hpp:6741:                utf8_bytes[0] = static_cast<std::char_traits<char>::int_type>(0xE0u | ((static_cast<unsigned int>(wc) >> 12u)));
./src/json.hpp:6742:                utf8_bytes[1] = static_cast<std::char_traits<char>::int_type>(0x80u | ((static_cast<unsigned int>(wc) >> 6u) & 0x3Fu));
./src/json.hpp:6743:                utf8_bytes[2] = static_cast<std::char_traits<char>::int_type>(0x80u | (static_cast<unsigned int>(wc) & 0x3Fu));
./src/json.hpp:6750:                    const auto wc2 = static_cast<unsigned int>(input.get_character());
./src/json.hpp:6751:                    const auto charcode = 0x10000u + (((static_cast<unsigned int>(wc) & 0x3FFu) << 10u) | (wc2 & 0x3FFu));
./src/json.hpp:6932:               int >::type = 0 >
./src/json.hpp:6961:                   int >::type = 0 >
./src/json.hpp:6968:                 int>::type = 0>
./src/json.hpp:7177:    int get_codepoint()
./src/json.hpp:7181:        int codepoint = 0;
./src/json.hpp:7190:                codepoint += static_cast<int>((static_cast<unsigned int>(current) - 0x30u) << factor);
./src/json.hpp:7194:                codepoint += static_cast<int>((static_cast<unsigned int>(current) - 0x37u) << factor);
./src/json.hpp:7198:                codepoint += static_cast<int>((static_cast<unsigned int>(current) - 0x57u) << factor);
./src/json.hpp:7329:                            const int codepoint1 = get_codepoint();
./src/json.hpp:7330:                            int codepoint = codepoint1; // start with codepoint1
./src/json.hpp:7344:                                    const int codepoint2 = get_codepoint();
./src/json.hpp:7356:                                        codepoint = static_cast<int>(
./src/json.hpp:7358:                                                        (static_cast<unsigned int>(codepoint1) << 10u)
./src/json.hpp:7360:                                                        + static_cast<unsigned int>(codepoint2)
./src/json.hpp:7399:                                add(static_cast<char_int_type>(0xC0u | (static_cast<unsigned int>(codepoint) >> 6u)));
./src/json.hpp:7400:                                add(static_cast<char_int_type>(0x80u | (static_cast<unsigned int>(codepoint) & 0x3Fu)));
./src/json.hpp:7405:                                add(static_cast<char_int_type>(0xE0u | (static_cast<unsigned int>(codepoint) >> 12u)));
./src/json.hpp:7406:                                add(static_cast<char_int_type>(0x80u | ((static_cast<unsigned int>(codepoint) >> 6u) & 0x3Fu)));
./src/json.hpp:7407:                                add(static_cast<char_int_type>(0x80u | (static_cast<unsigned int>(codepoint) & 0x3Fu)));
./src/json.hpp:7412:                                add(static_cast<char_int_type>(0xF0u | (static_cast<unsigned int>(codepoint) >> 18u)));
./src/json.hpp:7413:                                add(static_cast<char_int_type>(0x80u | ((static_cast<unsigned int>(codepoint) >> 12u) & 0x3Fu)));
./src/json.hpp:7414:                                add(static_cast<char_int_type>(0x80u | ((static_cast<unsigned int>(codepoint) >> 6u) & 0x3Fu)));
./src/json.hpp:7415:                                add(static_cast<char_int_type>(0x80u | (static_cast<unsigned int>(codepoint) & 0x3Fu)));
./src/json.hpp:7932:    static void strtof(long double& f, const char* str, char** endptr) noexcept
./src/json.hpp:9181:        const bool keep = callback(static_cast<int>(ref_stack.size()), parse_event_t::object_start, discarded);
./src/json.hpp:9215:        const bool keep = callback(static_cast<int>(ref_stack.size()), parse_event_t::key, k);
./src/json.hpp:9231:            if (!callback(static_cast<int>(ref_stack.size()) - 1, parse_event_t::object_end, *ref_stack.back()))
./src/json.hpp:9279:        const bool keep = callback(static_cast<int>(ref_stack.size()), parse_event_t::array_start, discarded);
./src/json.hpp:9315:            keep = callback(static_cast<int>(ref_stack.size()) - 1, parse_event_t::array_end, *ref_stack.back());
./src/json.hpp:9474:        const bool keep = skip_callback || callback(static_cast<int>(ref_stack.size()), parse_event_t::value, value);
./src/json.hpp:9817:static inline bool little_endianness(int num = 1) noexcept
./src/json.hpp:10397:                           conditional_static_cast<std::size_t>(static_cast<unsigned int>(current) & 0x1Fu), tag_handler);
./src/json.hpp:10451:                return get_cbor_object(conditional_static_cast<std::size_t>(static_cast<unsigned int>(current) & 0x1Fu), tag_handler);
./src/json.hpp:10624:                const auto half = static_cast<unsigned int>((byte1 << 8u) + byte2);
./src/json.hpp:10627:                    const int exp = (half >> 10u) & 0x1Fu;
./src/json.hpp:10628:                    const unsigned int mant = half & 0x3FFu;
./src/json.hpp:10715:                return get_string(input_format_t::cbor, static_cast<unsigned int>(current) & 0x1Fu, result);
./src/json.hpp:10811:                return get_binary(input_format_t::cbor, static_cast<unsigned int>(current) & 0x1Fu, result);
./src/json.hpp:11121:                return get_msgpack_object(conditional_static_cast<std::size_t>(static_cast<unsigned int>(current) & 0x0Fu));
./src/json.hpp:11140:                return get_msgpack_array(conditional_static_cast<std::size_t>(static_cast<unsigned int>(current) & 0x0Fu));
./src/json.hpp:11244:            case 0xD0: // int 8
./src/json.hpp:11250:            case 0xD1: // int 16
./src/json.hpp:11256:            case 0xD2: // int 32
./src/json.hpp:11262:            case 0xD3: // int 64
./src/json.hpp:11389:                return get_string(input_format_t::msgpack, static_cast<unsigned int>(current) & 0x1Fu, result);
./src/json.hpp:12181:                const auto half = static_cast<unsigned int>((byte2 << 8u) + byte1);
./src/json.hpp:12184:                    const int exp = (half >> 10u) & 0x1Fu;
./src/json.hpp:12185:                    const unsigned int mant = half & 0x3FFu;
./src/json.hpp:12895:    std::function<bool(int /*depth*/, parse_event_t /*event*/, BasicJsonType& /*parsed*/)>;
./src/json.hpp:13476:    primitive_iterator_t operator++(int)& noexcept // NOLINT(cert-dcl21-cpp)
./src/json.hpp:13489:    primitive_iterator_t operator--(int)& noexcept // NOLINT(cert-dcl21-cpp)
./src/json.hpp:13909:    iter_impl operator++(int)& // NOLINT(cert-dcl21-cpp)
./src/json.hpp:13960:    iter_impl operator--(int)& // NOLINT(cert-dcl21-cpp)
./src/json.hpp:14369:    json_reverse_iterator operator++(int)& // NOLINT(cert-dcl21-cpp)
./src/json.hpp:14381:    json_reverse_iterator operator--(int)& // NOLINT(cert-dcl21-cpp)
./src/json.hpp:14722:        const unsigned long long res = std::strtoull(p, &p_end, 10); // NOLINT(runtime/int)
./src/json.hpp:14732:        if (res >= static_cast<unsigned long long>((std::numeric_limits<size_type>::max)()))  // NOLINT(runtime/int)
./src/json.hpp:15522:        enable_if_t<std::is_constructible<value_type, Args...>::value, int> = 0 >
./src/json.hpp:17056:                 std::is_floating_point<NumberType>::value, int>::type = 0>
./src/json.hpp:17070:                 std::is_unsigned<NumberType>::value, int>::type = 0>
./src/json.hpp:17158:                   !std::is_floating_point<NumberType>::value, int >::type = 0 >
./src/json.hpp:17664:    static constexpr int kPrecision = 64; // = q
./src/json.hpp:17667:    int e = 0;
./src/json.hpp:17669:    constexpr diyfp(std::uint64_t f_, int e_) noexcept : f(f_), e(e_) {}
./src/json.hpp:17769:    static diyfp normalize_to(const diyfp& x, const int target_exponent) noexcept
./src/json.hpp:17771:        const int delta = x.e - target_exponent;
./src/json.hpp:17809:    constexpr int      kPrecision = std::numeric_limits<FloatType>::digits; // = p (includes the hidden bit)
./src/json.hpp:17810:    constexpr int      kBias      = std::numeric_limits<FloatType>::max_exponent - 1 + (kPrecision - 1);
./src/json.hpp:17811:    constexpr int      kMinExp    = 1 - kBias;
./src/json.hpp:17823:                    : diyfp(F + kHiddenBit, static_cast<int>(E) - kBias);
./src/json.hpp:17916:constexpr int kAlpha = -60;
./src/json.hpp:17917:constexpr int kGamma = -32;
./src/json.hpp:17922:    int e;
./src/json.hpp:17923:    int k;
./src/json.hpp:17933:inline cached_power get_cached_power_for_binary_exponent(int e)
./src/json.hpp:17985:    constexpr int kCachedPowersMinDecExp = -300;
./src/json.hpp:17986:    constexpr int kCachedPowersDecStep = 8;
./src/json.hpp:18079:    const int f = kAlpha - e - 1;
./src/json.hpp:18080:    const int k = ((f * 78913) / (1 << 18)) + static_cast<int>(f > 0);
./src/json.hpp:18082:    const int index = (-kCachedPowersMinDecExp + k + (kCachedPowersDecStep - 1)) / kCachedPowersDecStep;
./src/json.hpp:18097:inline int find_largest_pow10(const std::uint32_t n, std::uint32_t& pow10)
./src/json.hpp:18151:inline void grisu2_round(char* buf, int len, std::uint64_t dist, std::uint64_t delta,
./src/json.hpp:18192:inline void grisu2_digit_gen(char* buffer, int& length, int& decimal_exponent,
./src/json.hpp:18225:    auto p1 = static_cast<std::uint32_t>(M_plus.f >> -one.e); // p1 = f div 2^-e (Since -e >= 32, p1 fits into a 32-bit int.)
./src/json.hpp:18235:    const int k = find_largest_pow10(p1, pow10);
./src/json.hpp:18255:    int n = k;
./src/json.hpp:18357:    int m = 0;
./src/json.hpp:18433:inline void grisu2(char* buf, int& len, int& decimal_exponent,
./src/json.hpp:18493:void grisu2(char* buf, int& len, int& decimal_exponent, FloatType value)
./src/json.hpp:18533:inline char* append_exponent(char* buf, int e)
./src/json.hpp:18585:inline char* format_buffer(char* buf, int len, int decimal_exponent,
./src/json.hpp:18586:                           int min_exp, int max_exp)
./src/json.hpp:18591:    const int k = len;
./src/json.hpp:18592:    const int n = len + decimal_exponent;
./src/json.hpp:18704:    int len = 0;
./src/json.hpp:18705:    int decimal_exponent = 0;
./src/json.hpp:18711:    constexpr int kMinExp = -4;
./src/json.hpp:18713:    constexpr int kMaxExp = std::numeric_limits<FloatType>::digits10;
./src/json.hpp:18816:              const unsigned int indent_step,
./src/json.hpp:18817:              const unsigned int current_indent = 0)
./src/json.hpp:19352:    unsigned int count_digits(number_unsigned_t x) noexcept
./src/json.hpp:19354:        unsigned int n_digits = 1;
./src/json.hpp:19393:    template <typename NumberType, enable_if_t<std::is_signed<NumberType>::value, int> = 0>
./src/json.hpp:19399:    template < typename NumberType, enable_if_t <std::is_unsigned<NumberType>::value, int > = 0 >
./src/json.hpp:19419:                   int > = 0 >
./src/json.hpp:19450:        unsigned int n_chars{};
./src/json.hpp:19770:                 detail::is_usable_as_key_type<key_compare, key_type, KeyType>::value, int> = 0>
./src/json.hpp:19790:                 detail::is_usable_as_key_type<key_compare, key_type, KeyType>::value, int> = 0>
./src/json.hpp:19802:                 detail::is_usable_as_key_type<key_compare, key_type, KeyType>::value, int> = 0>
./src/json.hpp:19822:                 detail::is_usable_as_key_type<key_compare, key_type, KeyType>::value, int> = 0>
./src/json.hpp:19850:                 detail::is_usable_as_key_type<key_compare, key_type, KeyType>::value, int> = 0>
./src/json.hpp:19884:                 detail::is_usable_as_key_type<key_compare, key_type, KeyType>::value, int> = 0>
./src/json.hpp:19975:                 detail::is_usable_as_key_type<key_compare, key_type, KeyType>::value, int> = 0>
./src/json.hpp:20001:                 detail::is_usable_as_key_type<key_compare, key_type, KeyType>::value, int> = 0>
./src/json.hpp:20836:                   !detail::is_basic_json<U>::value && detail::is_compatible_type<basic_json_t, U>::value, int > = 0 >
./src/json.hpp:20850:                   detail::is_basic_json<BasicJsonType>::value&& !std::is_same<basic_json, BasicJsonType>::value, int > = 0 >
./src/json.hpp:21038:                   std::is_same<InputIT, typename basic_json_t::const_iterator>::value, int >::type = 0 >
./src/json.hpp:21147:                                 std::is_same<typename JsonRef::value_type, basic_json>>::value, int> = 0 >
./src/json.hpp:21298:    string_t dump(const int indent = -1,
./src/json.hpp:21308:            s.dump(*this, true, ensure_ascii, static_cast<unsigned int>(indent));
./src/json.hpp:21570:                 std::is_pointer<PointerType>::value, int>::type = 0>
./src/json.hpp:21581:                   std::is_const<typename std::remove_pointer<PointerType>::type>::value, int >::type = 0 >
./src/json.hpp:21631:                   int > = 0 >
./src/json.hpp:21673:                   int > = 0 >
./src/json.hpp:21698:                   int > = 0 >
./src/json.hpp:21721:                 int> = 0>
./src/json.hpp:21734:                 int> = 0>
./src/json.hpp:21810:                 std::is_pointer<PointerType>::value, int>::type = 0>
./src/json.hpp:21823:                   int > = 0 >
./src/json.hpp:21836:                 int> = 0>
./src/json.hpp:21847:            detail::has_from_json<basic_json_t, Array>::value, int > = 0 >
./src/json.hpp:21859:                 std::is_reference<ReferenceType>::value, int>::type = 0>
./src/json.hpp:21870:                   std::is_const<typename std::remove_reference<ReferenceType>::type>::value, int >::type = 0 >
./src/json.hpp:21884:    instance `int` for JSON integer numbers, `bool` for JSON booleans, or
./src/json.hpp:21921:                                                >::value, int >::type = 0 >
./src/json.hpp:22029:                 detail::is_usable_as_basic_json_key_type<basic_json_t, KeyType>::value, int> = 0>
./src/json.hpp:22067:                 detail::is_usable_as_basic_json_key_type<basic_json_t, KeyType>::value, int> = 0>
./src/json.hpp:22197:                 detail::is_usable_as_basic_json_key_type<basic_json_t, KeyType>::value, int > = 0 >
./src/json.hpp:22221:                 detail::is_usable_as_basic_json_key_type<basic_json_t, KeyType>::value, int > = 0 >
./src/json.hpp:22251:                   && !std::is_same<value_t, detail::uncvref_t<ValueType>>::value, int > = 0 >
./src/json.hpp:22276:                   && !std::is_same<value_t, detail::uncvref_t<ValueType>>::value, int > = 0 >
./src/json.hpp:22302:                   && !std::is_same<value_t, detail::uncvref_t<ValueType>>::value, int > = 0 >
./src/json.hpp:22329:                   && !std::is_same<value_t, detail::uncvref_t<ValueType>>::value, int > = 0 >
./src/json.hpp:22352:                   && !std::is_same<value_t, detail::uncvref_t<ValueType>>::value, int > = 0 >
./src/json.hpp:22377:                   && !std::is_same<value_t, detail::uncvref_t<ValueType>>::value, int > = 0 >
./src/json.hpp:22400:                   && !std::is_same<value_t, detail::uncvref_t<ValueType>>::value, int > = 0 >
./src/json.hpp:22411:                   && !std::is_same<value_t, detail::uncvref_t<ValueType>>::value, int > = 0 >
./src/json.hpp:22454:                   std::is_same<IteratorType, typename basic_json_t::const_iterator>::value, int > = 0 >
./src/json.hpp:22524:                   std::is_same<IteratorType, typename basic_json_t::const_iterator>::value, int > = 0 >
./src/json.hpp:22595:                   detail::has_erase_with_key_type<basic_json_t, KeyType>::value, int > = 0 >
./src/json.hpp:22608:                   !detail::has_erase_with_key_type<basic_json_t, KeyType>::value, int > = 0 >
./src/json.hpp:22640:                 detail::is_usable_as_basic_json_key_type<basic_json_t, KeyType>::value, int> = 0>
./src/json.hpp:22706:                 detail::is_usable_as_basic_json_key_type<basic_json_t, KeyType>::value, int> = 0>
./src/json.hpp:22722:                 detail::is_usable_as_basic_json_key_type<basic_json_t, KeyType>::value, int> = 0>
./src/json.hpp:22746:                 detail::is_usable_as_basic_json_key_type<basic_json_t, KeyType>::value, int> = 0>
./src/json.hpp:22763:                 detail::is_usable_as_basic_json_key_type<basic_json_t, KeyType>::value, int> = 0>
./src/json.hpp:22776:    template<typename BasicJsonType, detail::enable_if_t<detail::is_basic_json<BasicJsonType>::value, int> = 0>
./src/json.hpp:23828:                 std::is_scalar<ScalarType>::value, int>::type = 0>
./src/json.hpp:23837:                 std::is_scalar<ScalarType>::value, int>::type = 0>
./src/json.hpp:23857:                 std::is_scalar<ScalarType>::value, int>::type = 0>
./src/json.hpp:23866:                 std::is_scalar<ScalarType>::value, int>::type = 0>
./src/json.hpp:23885:                 std::is_scalar<ScalarType>::value, int>::type = 0>
./src/json.hpp:23894:                 std::is_scalar<ScalarType>::value, int>::type = 0>
./src/json.hpp:23914:                 std::is_scalar<ScalarType>::value, int>::type = 0>
./src/json.hpp:23923:                 std::is_scalar<ScalarType>::value, int>::type = 0>
./src/json.hpp:23944:                 std::is_scalar<ScalarType>::value, int>::type = 0>
./src/json.hpp:23953:                 std::is_scalar<ScalarType>::value, int>::type = 0>
./src/json.hpp:23973:                 std::is_scalar<ScalarType>::value, int>::type = 0>
./src/json.hpp:23982:                 std::is_scalar<ScalarType>::value, int>::type = 0>
./src/json.hpp:24013:        s.dump(j, pretty_print, false, static_cast<unsigned int>(indentation));
./src/json.hpp:24670:    template<typename BasicJsonType, detail::enable_if_t<detail::is_basic_json<BasicJsonType>::value, int> = 0>
./src/json.hpp:24684:    template<typename BasicJsonType, detail::enable_if_t<detail::is_basic_json<BasicJsonType>::value, int> = 0>
./src/json.hpp:24698:    template<typename BasicJsonType, detail::enable_if_t<detail::is_basic_json<BasicJsonType>::value, int> = 0>
./src/json.hpp:24712:    template<typename BasicJsonType, detail::enable_if_t<detail::is_basic_json<BasicJsonType>::value, int> = 0>
./src/config_handler.cpp:131:    config.tx_iterations.store(jConfig["Meta"]["TX Iterations"].get<int>());
./src/config_handler.cpp:141:    config.power_dbm = jConfig["Common"]["TX Power"].get<int>();
./src/config_handler.cpp:143:    config.tx_pin = jConfig["Common"]["Transmit Pin"].get<int>();
./src/config_handler.cpp:150:    config.led_pin = jConfig["Extended"]["LED Pin"].get<int>();
./src/config_handler.cpp:151:    config.power_level = jConfig["Extended"]["Power Level"].get<int>();
./src/config_handler.cpp:154:    config.web_port = jConfig["Server"]["Web Port"].get<int>();
./src/config_handler.cpp:155:    config.socket_port = jConfig["Server"]["Socket Port"].get<int>();
./src/config_handler.cpp:157:    config.shutdown_pin = jConfig["Server"]["Shutdown Button"].get<int>();
./src/config_handler.cpp:308:                long lval = std::strtol(raw_value.c_str(), &end, 10);
./src/WSPR-Message/src/wspr_message.hpp:60:    WsprMessage(const std::string &callsign, const std::string &location, int power);
./src/WSPR-Message/src/wspr_message.hpp:73:    WsprMessage &set_message_parameters(const std::string &callsign, const std::string &location, int power);
./src/WSPR-Message/src/wspr_message.hpp:89:    static constexpr int size = MSG_SIZE;
./src/WSPR-Message/src/wspr_message.hpp:98:    static int get_character_value(char ch);
./src/WSPR-Message/src/wspr_message.hpp:106:    static int calculate_parity(uint32_t ch);
./src/WSPR-Message/src/wspr_message.hpp:138:    void generate_wspr_symbols(const std::string &callsign, const std::string &location, int power);
./src/WSPR-Message/src/main.cpp:46:            std::cout << static_cast<int>(symbols[i]);
./src/WSPR-Message/src/main.cpp:64:int main()
./src/WSPR-Message/src/main.cpp:69:    int power = 20;                 ///< Transmission power level in dBm
./src/WSPR-Message/src/wspr_message.cpp:55:WsprMessage::WsprMessage(const std::string &callsign, const std::string &location, int power)
./src/WSPR-Message/src/wspr_message.cpp:75:WsprMessage &WsprMessage::set_message_parameters(const std::string &callsign, const std::string &location, int power)
./src/WSPR-Message/src/wspr_message.cpp:138:void WsprMessage::generate_wspr_symbols(const std::string &callsign, const std::string &location, int power)
./src/WSPR-Message/src/wspr_message.cpp:176:    int i;
./src/WSPR-Message/src/wspr_message.cpp:222:int WsprMessage::get_character_value(char ch)
./src/WSPR-Message/src/wspr_message.cpp:295:int WsprMessage::calculate_parity(uint32_t ch)
./src/WSPR-Message/src/wspr_message.cpp:297:    int even = 0; // Tracks parity (0 = even, 1 = odd)
./src/gpio_input.hpp:123:    bool enable(int pin, bool trigger_high, PullMode pull_mode, std::function<void()> callback);
./src/gpio_input.hpp:155:    bool setPriority(int schedPolicy, int priority);
./src/gpio_input.hpp:178:    int gpio_pin_;                   ///< BCM GPIO pin number.
./src/Signal-Handler/src/main.cpp:56:void signal_handler(int signum, bool critical = false)
./src/Signal-Handler/src/main.cpp:78:void worker_thread(int id)
./src/Signal-Handler/src/main.cpp:85:        for (volatile int i = 0; i < 1000000; ++i)
./src/Signal-Handler/src/main.cpp:105:int main()
./src/Signal-Handler/src/main.cpp:117:    const int num_workers = 4;
./src/Signal-Handler/src/main.cpp:118:    for (int i = 0; i < num_workers; ++i)
./src/Signal-Handler/src/signal_handler.hpp:117:    void setCallback(const std::function<void(int, bool)> &cb);
./src/Signal-Handler/src/signal_handler.hpp:133:    bool setPriority(int schedPolicy, int priority);
./src/Signal-Handler/src/signal_handler.hpp:141:    static std::string_view signalToString(int signum);
./src/Signal-Handler/src/signal_handler.hpp:150:    static const std::unordered_map<int, std::pair<std::string_view, bool>> signal_map;
./src/Signal-Handler/src/signal_handler.hpp:176:    std::function<void(int, bool)> callback;
./src/Signal-Handler/src/signal_handler.cpp:68:const std::unordered_map<int, std::pair<std::string_view, bool>> SignalHandler::signal_map = {
./src/Signal-Handler/src/signal_handler.cpp:268:void SignalHandler::setCallback(const std::function<void(int, bool)> &cb)
./src/Signal-Handler/src/signal_handler.cpp:285:std::string_view SignalHandler::signalToString(int signum)
./src/Signal-Handler/src/signal_handler.cpp:367:bool SignalHandler::setPriority(int schedPolicy, int priority)
./src/Signal-Handler/src/signal_handler.cpp:380:    int ret = pthread_setschedparam(worker_thread.native_handle(), schedPolicy, &sch_params);
./src/Signal-Handler/src/signal_handler.cpp:436:        int sig = sigwaitinfo(&local_set, &siginfo);
./src/web_socket.cpp:129:    int opt = 1;
./src/web_socket.cpp:137:    int off = 0;
./src/web_socket.cpp:208:        for (int fd : client_sockets_)
./src/web_socket.cpp:477:        for (int i = 0; i < 8; i++)
./src/web_socket.cpp:554:        int val = data[i] << 16;
./src/web_socket.cpp:608:        for (int i = 7; i >= 0; --i)
./src/web_socket.cpp:618:    for (int fd : client_sockets_)
./src/web_socket.cpp:660:        for (int i = 7; i >= 0; --i)
./src/web_socket.cpp:670:    for (int fd : client_sockets_)
./src/web_socket.cpp:695:        int client = accept(
./src/web_socket.cpp:765:void WebSocketServer::clientLoop(int client_fd)
./src/web_socket.cpp:822:                llog.logS(WARN, "Unhandled opcode", static_cast<int>(opcode),
./src/web_socket.cpp:864:            for (int fd : client_sockets_)
./src/web_socket.cpp:892:bool WebSocketServer::performHandshake(int client)
./src/web_socket.cpp:894:    const int buf_size = 4096;
./src/web_socket.cpp:962:bool WebSocketServer::setThreadPriority(int schedPolicy, int priority)
./src/web_socket.cpp:970:        int ret = pthread_setschedparam(server_thread_.native_handle(), schedPolicy, &sch_params);
./src/web_socket.cpp:980:        int ret = pthread_setschedparam(keep_alive_thread_.native_handle(), schedPolicy, &sch_params);
./src/gpio_output.cpp:78:bool GPIOOutput::enableGPIOPin(int pin, bool active_high)
./src/gpio_output.cpp:103:        int init_value = compute_physical_state(false);
./src/gpio_output.cpp:159:        int physical_state = compute_physical_state(state);
./src/gpio_output.cpp:181:int GPIOOutput::compute_physical_state(bool logical_state) const
./src/Broadcom-Mailbox/src/mailbox.h:58:int mbox_open();
./src/Broadcom-Mailbox/src/mailbox.h:65:void mbox_close(int file_desc);
./src/Broadcom-Mailbox/src/mailbox.h:73:uint32_t get_version(int file_desc);
./src/Broadcom-Mailbox/src/mailbox.h:84:uint32_t mem_alloc(int file_desc, uint32_t size, uint32_t align, uint32_t flags);
./src/Broadcom-Mailbox/src/mailbox.h:93:uint32_t mem_free(int file_desc, uint32_t handle);
./src/Broadcom-Mailbox/src/mailbox.h:102:uint32_t mem_lock(int file_desc, uint32_t handle);
./src/Broadcom-Mailbox/src/mailbox.h:111:uint32_t mem_unlock(int file_desc, uint32_t handle);
./src/Broadcom-Mailbox/src/mailbox.h:138:uint32_t execute_code(int file_desc, uint32_t code, uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3, uint32_t r4, uint32_t r5);
./src/Broadcom-Mailbox/src/mailbox.h:147:uint32_t qpu_enable(int file_desc, uint32_t enable);
./src/Broadcom-Mailbox/src/mailbox.h:159:uint32_t execute_qpu(int file_desc, uint32_t num_qpus, uint32_t control, uint32_t noflush, uint32_t timeout);
./src/Broadcom-Mailbox/src/mailbox.c:61:    int mem_fd;
./src/Broadcom-Mailbox/src/mailbox.c:102:static int mbox_property(int file_desc, void *buf)
./src/Broadcom-Mailbox/src/mailbox.c:104:    int ret_val = ioctl(file_desc, IOCTL_MBOX_PROPERTY, buf);
./src/Broadcom-Mailbox/src/mailbox.c:114:    int i;
./src/Broadcom-Mailbox/src/mailbox.c:122:unsigned mem_alloc(int file_desc, unsigned size, unsigned align, unsigned flags)
./src/Broadcom-Mailbox/src/mailbox.c:124:    int i = 0;
./src/Broadcom-Mailbox/src/mailbox.c:147:unsigned mem_free(int file_desc, unsigned handle)
./src/Broadcom-Mailbox/src/mailbox.c:149:    int i = 0;
./src/Broadcom-Mailbox/src/mailbox.c:177:unsigned mem_lock(int file_desc, unsigned handle)
./src/Broadcom-Mailbox/src/mailbox.c:207:unsigned mem_unlock(int file_desc, unsigned handle)
./src/Broadcom-Mailbox/src/mailbox.c:238:unsigned execute_code(int file_desc, unsigned code, unsigned r0, unsigned r1, unsigned r2, unsigned r3, unsigned r4, unsigned r5)
./src/Broadcom-Mailbox/src/mailbox.c:272:int mbox_open()
./src/Broadcom-Mailbox/src/mailbox.c:274:    int file_desc = open(DEVICE_FILE_NAME, O_RDWR);
./src/Broadcom-Mailbox/src/mailbox.c:288:void mbox_close(int file_desc)
./src/arg_parser.hpp:99:extern std::atomic<int> wspr_interval;
./src/arg_parser.hpp:150:void print_usage(const std::string &message = "", int exit_code = 3);
./src/arg_parser.hpp:157:inline void print_usage(int exit_code)
./src/arg_parser.hpp:210:bool parse_command_line(int argc, char *argv[]);
./src/web_socket.hpp:102:    bool setThreadPriority(int schedPolicy, int priority);
./src/web_socket.hpp:125:    int listen_fd_;             ///< Socket file descriptor for listening.
./src/web_socket.hpp:131:    std::vector<int> client_sockets_;
./src/web_socket.hpp:141:    void clientLoop(int client_fd);
./src/web_socket.hpp:165:    bool performHandshake(int client);
./src/main.cpp:59:constexpr int SINGLETON_PORT = 1234;
./src/main.cpp:73:void callback_signal_handler(int signum, bool is_critical)
./src/main.cpp:110:int main(int argc, char *argv[])
./src/main.cpp:112:    int retval = EXIT_SUCCESS;
./src/Singleton/src/main.cpp:102:int main(int argc, char *argv[])
./src/Singleton/src/singleton.hpp:128:    int socket_fd_; ///< File descriptor for the bound socket, or -1 if unbound.
./src/web_server.hpp:84:    void start(int port);
./src/web_server.hpp:102:    bool setThreadPriority(int schedPolicy, int priority);
./src/web_server.hpp:105:    int port_;                         ///< Port on which the server listens.
./src/scheduling.cpp:97:int freq_iterator = 0;
./src/scheduling.cpp:123:extern int sig_pipe_fds[2];
./src/scheduling.cpp:352:        for (int i = 0; i < 3; ++i)
./src/scheduling.cpp:397:        for (int i = 0; i < 2; ++i)
./src/scheduling.cpp:730:            int remaining = --config.tx_iterations;
./src/httplib.cpp:17:        bool is_hex(char c, int &v)
./src/httplib.cpp:38:                           int &val)
./src/httplib.cpp:77:        size_t to_utf8(int code, char *buff)
./src/httplib.cpp:252:                            << static_cast<int>(static_cast<unsigned char>(c));
./src/httplib.cpp:727:        int close_socket(socket_t sock)
./src/httplib.cpp:753:        ssize_t read_socket(socket_t sock, void *ptr, size_t size, int flags)
./src/httplib.cpp:758:                                              static_cast<char *>(ptr), static_cast<int>(size),
./src/httplib.cpp:766:                            int flags)
./src/httplib.cpp:771:                                              static_cast<const char *>(ptr), static_cast<int>(size),
./src/httplib.cpp:778:        int poll_wrapper(struct pollfd *fds, nfds_t nfds, int timeout)
./src/httplib.cpp:794:            auto timeout = static_cast<int>(sec * 1000 + usec / 1000);
./src/httplib.cpp:817:            auto timeout = static_cast<int>(sec * 1000 + usec / 1000);
./src/httplib.cpp:871:            void get_remote_ip_and_port(std::string &ip, int &port) const override;
./src/httplib.cpp:872:            void get_local_ip_and_port(std::string &ip, int &port) const override;
./src/httplib.cpp:909:            void get_remote_ip_and_port(std::string &ip, int &port) const override;
./src/httplib.cpp:910:            void get_local_ip_and_port(std::string &ip, int &port) const override;
./src/httplib.cpp:1025:        int shutdown_socket(socket_t sock)
./src/httplib.cpp:1058:        socket_t create_socket(const std::string &host, const std::string &ip, int port,
./src/httplib.cpp:1059:                               int address_family, int socket_flags, bool tcp_nodelay,
./src/httplib.cpp:1287:        std::string if2ip(int address_family, const std::string &ifn)
./src/httplib.cpp:1338:            const std::string &host, const std::string &ip, int port,
./src/httplib.cpp:1339:            int address_family, bool tcp_nodelay, bool ipv6_v6only,
./src/httplib.cpp:1416:                             socklen_t addr_len, std::string &ip, int &port)
./src/httplib.cpp:1444:        void get_local_ip_and_port(socket_t sock, std::string &ip, int &port)
./src/httplib.cpp:1455:        void get_remote_ip_and_port(socket_t sock, std::string &ip, int &port)
./src/httplib.cpp:1488:        constexpr unsigned int str2tag_core(const char *s, size_t l,
./src/httplib.cpp:1489:                                            unsigned int h)
./src/httplib.cpp:1496:                             (((std::numeric_limits<unsigned int>::max)() >> 6) &
./src/httplib.cpp:1501:        unsigned int str2tag(const std::string &s)
./src/httplib.cpp:1509:            constexpr unsigned int operator""_t(const char *s, size_t l)
./src/httplib.cpp:2261:            unsigned long chunk_len;
./src/httplib.cpp:2355:        bool prepare_content_receiver(T &x, int &status,
./src/httplib.cpp:2424:        bool read_content(Stream &strm, T &x, size_t payload_max_length, int &status,
./src/httplib.cpp:2485:        ssize_t write_response_line(Stream &strm, int status)
./src/httplib.cpp:3665:            unsigned int hash_length = 0;
./src/httplib.cpp:3676:                   << static_cast<unsigned int>(hash[i]);
./src/httplib.cpp:3713:                if (qop.find("auth-int") != std::string::npos)
./src/httplib.cpp:3715:                    qop = "auth-int";
./src/httplib.cpp:3742:                if (qop == "auth-int")
./src/httplib.cpp:4227:    void Response::set_redirect(const std::string &url, int stat)
./src/httplib.cpp:4410:                (std::min)(size, static_cast<size_t>((std::numeric_limits<int>::max)()));
./src/httplib.cpp:4477:                (std::min)(size, static_cast<size_t>((std::numeric_limits<int>::max)()));
./src/httplib.cpp:4484:                                                  int &port) const
./src/httplib.cpp:4490:                                                 int &port) const
./src/httplib.cpp:4529:                                                  int & /*port*/) const {}
./src/httplib.cpp:4532:                                                 int & /*port*/) const {}
./src/httplib.cpp:4857:    Server &Server::set_address_family(int family)
./src/httplib.cpp:4933:    bool Server::bind_to_port(const std::string &host, int port,
./src/httplib.cpp:4934:                              int socket_flags)
./src/httplib.cpp:4943:    int Server::bind_to_any_port(const std::string &host, int socket_flags)
./src/httplib.cpp:4955:    bool Server::listen(const std::string &host, int port,
./src/httplib.cpp:4956:                        int socket_flags)
./src/httplib.cpp:5439:    Server::create_server_socket(const std::string &host, int port,
./src/httplib.cpp:5440:                                 int socket_flags,
./src/httplib.cpp:5460:    int Server::bind_internal(const std::string &host, int port,
./src/httplib.cpp:5461:                              int socket_flags)
./src/httplib.cpp:5887:                            int remote_port, const std::string &local_addr,
./src/httplib.cpp:5888:                            int local_port, bool close_connection,
./src/httplib.cpp:5963:            int status = StatusCode::Continue_100;
./src/httplib.cpp:6107:        int remote_port = 0;
./src/httplib.cpp:6111:        int local_port = 0;
./src/httplib.cpp:6134:    ClientImpl::ClientImpl(const std::string &host, int port)
./src/httplib.cpp:6137:    ClientImpl::ClientImpl(const std::string &host, int port,
./src/httplib.cpp:7141:                int dummy_status;
./src/httplib.cpp:7966:    int ClientImpl::port() const { return port_; }
./src/httplib.cpp:8043:    void ClientImpl::set_address_family(int family)
./src/httplib.cpp:8066:    void ClientImpl::set_proxy(const std::string &host, int port)
./src/httplib.cpp:8110:        auto mem = BIO_new_mem_buf(ca_cert, static_cast<int>(size));
./src/httplib.cpp:8127:            for (auto i = 0; i < static_cast<int>(sk_X509_INFO_num(inf)); i++)
./src/httplib.cpp:8192:                auto bio = BIO_new_socket(static_cast<int>(sock), BIO_NOCLOSE);
./src/httplib.cpp:8345:                return SSL_read(ssl_, ptr, static_cast<int>(size));
./src/httplib.cpp:8349:                auto ret = SSL_read(ssl_, ptr, static_cast<int>(size));
./src/httplib.cpp:8365:                            return SSL_read(ssl_, ptr, static_cast<int>(size));
./src/httplib.cpp:8370:                            ret = SSL_read(ssl_, ptr, static_cast<int>(size));
./src/httplib.cpp:8395:                auto handle_size = static_cast<int>(
./src/httplib.cpp:8396:                    std::min<size_t>(size, (std::numeric_limits<int>::max)()));
./src/httplib.cpp:8398:                auto ret = SSL_write(ssl_, ptr, static_cast<int>(handle_size));
./src/httplib.cpp:8415:                            ret = SSL_write(ssl_, ptr, static_cast<int>(handle_size));
./src/httplib.cpp:8434:                                                     int &port) const
./src/httplib.cpp:8440:                                                    int &port) const
./src/httplib.cpp:8584:            int remote_port = 0;
./src/httplib.cpp:8588:            int local_port = 0;
./src/httplib.cpp:8619:    SSLClient::SSLClient(const std::string &host, int port)
./src/httplib.cpp:8622:    SSLClient::SSLClient(const std::string &host, int port,
./src/httplib.cpp:8658:    SSLClient::SSLClient(const std::string &host, int port,
./src/httplib.cpp:8728:    long SSLClient::get_openssl_verify_result() const
./src/httplib.cpp:9214:    Client::Client(const std::string &host, int port)
./src/httplib.cpp:9217:    Client::Client(const std::string &host, int port,
./src/httplib.cpp:9696:    int Client::port() const { return cli_->port(); }
./src/httplib.cpp:9719:    void Client::set_address_family(int family)
./src/httplib.cpp:9780:    void Client::set_proxy(const std::string &host, int port)
./src/httplib.cpp:9848:    long Client::get_openssl_verify_result() const
./src/wspr_band_lookup.hpp:58:    using FrequencyRange = std::tuple<long long, long long, std::string>;
./src/wspr_band_lookup.hpp:94:    std::string validate_frequency(long long frequency) const;
./src/wspr_band_lookup.hpp:112:    std::variant<double, std::string> lookup(const std::variant<std::string, double, int> &input) const;
./src/wspr_band_lookup.hpp:120:    std::string freq_display_string(long long frequency) const;
./src/wspr_band_lookup.hpp:128:    long long parse_frequency_string(const std::string &freq_str) const;
./src/MonitorFile/src/monitorfile.hpp:87:    bool setPriority(int schedPolicy, int priority);
./src/MonitorFile/src/monitorfile.hpp:139:    int stable_checks = 0;                      ///< Counts stable intervals before confirming change.
./src/MonitorFile/src/main.cpp:52:void signalHandler(int signum)
./src/MonitorFile/src/main.cpp:65:void workerThread(int id)
./src/MonitorFile/src/main.cpp:69:        for (volatile int i = 0; i < 1000000; ++i)
./src/MonitorFile/src/main.cpp:115:int main()
./src/MonitorFile/src/main.cpp:124:    const int num_workers = 4;
./src/MonitorFile/src/main.cpp:127:    for (int i = 0; i < num_workers; ++i)
./src/MonitorFile/src/monitorfile.cpp:92:bool MonitorFile::setPriority(int schedPolicy, int priority)
./src/MonitorFile/src/monitorfile.cpp:103:    int ret = pthread_setschedparam(monitoring_thread.native_handle(), schedPolicy, &sch_params);
./src/wspr_band_lookup.cpp:125:long long WSPRBandLookup::parse_frequency_string(const std::string &freq_str) const
./src/wspr_band_lookup.cpp:141:            return static_cast<long long>(value * 1e9);
./src/wspr_band_lookup.cpp:143:            return static_cast<long long>(value * 1e6);
./src/wspr_band_lookup.cpp:145:            return static_cast<long long>(value * 1e3);
./src/wspr_band_lookup.cpp:147:            return static_cast<long long>(value);
./src/wspr_band_lookup.cpp:150:        return static_cast<long long>(value);
./src/wspr_band_lookup.cpp:189:std::string WSPRBandLookup::validate_frequency(long long frequency) const
./src/wspr_band_lookup.cpp:220:std::string WSPRBandLookup::freq_display_string(long long frequency) const
./src/wspr_band_lookup.cpp:263:std::variant<double, std::string> WSPRBandLookup::lookup(const std::variant<std::string, double, int> &input) const
./src/wspr_band_lookup.cpp:269:        return validate_frequency(static_cast<long long>(value));
./src/wspr_band_lookup.cpp:273:    if (std::holds_alternative<int>(input))
./src/wspr_band_lookup.cpp:275:        int value = std::get<int>(input);
./src/wspr_band_lookup.cpp:276:        return validate_frequency(static_cast<long long>(value));
./src/wspr_band_lookup.cpp:337:                std::string band = validate_frequency(static_cast<long long>(raw_freq));
./src/sha1.cpp:128:    for (int i = 7; i >= 0; i--)
./src/sha1.cpp:136:    auto write_uint32 = [&](uint32_t val, int offset)
./src/sha1.cpp:160:uint32_t SHA1::leftrotate(uint32_t value, unsigned int bits)
./src/sha1.cpp:177:    for (int i = 0; i < 16; i++)
./src/sha1.cpp:185:    for (int i = 16; i < 80; i++)
./src/sha1.cpp:193:    for (int i = 0; i < 80; i++)
./src/httplib.hpp:169:using ssize_t = long;
./src/httplib.hpp:193:using nfds_t = unsigned long;
./src/httplib.hpp:195:using socklen_t = int;
./src/httplib.hpp:224:using socket_t = int;
./src/httplib.hpp:352:            inline unsigned char to_lower(int c)
./src/httplib.hpp:896:        int remote_port = -1;
./src/httplib.hpp:898:        int local_port = -1;
./src/httplib.hpp:949:        int status = -1;
./src/httplib.hpp:963:        void set_redirect(const std::string &url, int status = StatusCode::Found_302);
./src/httplib.hpp:1018:        virtual void get_remote_ip_and_port(std::string &ip, int &port) const = 0;
./src/httplib.hpp:1019:        virtual void get_local_ip_and_port(std::string &ip, int &port) const = 0;
./src/httplib.hpp:1145:        bool set_socket_opt_impl(socket_t sock, int level, int optname,
./src/httplib.hpp:1147:        bool set_socket_opt(socket_t sock, int level, int optname, int opt);
./src/httplib.hpp:1148:        bool set_socket_opt_time(socket_t sock, int level, int optname, time_t sec,
./src/httplib.hpp:1155:    const char *status_message(int status);
./src/httplib.hpp:1254:            std::function<int(const Request &, Response &)>;
./src/httplib.hpp:1298:        Server &set_address_family(int family);
./src/httplib.hpp:1324:        bool bind_to_port(const std::string &host, int port, int socket_flags = 0);
./src/httplib.hpp:1325:        int bind_to_any_port(const std::string &host, int socket_flags = 0);
./src/httplib.hpp:1328:        bool listen(const std::string &host, int port, int socket_flags = 0);
./src/httplib.hpp:1339:                             int remote_port, const std::string &local_addr,
./src/httplib.hpp:1340:                             int local_port, bool close_connection,
./src/httplib.hpp:1368:        socket_t create_server_socket(const std::string &host, int port,
./src/httplib.hpp:1369:                                      int socket_flags,
./src/httplib.hpp:1371:        int bind_internal(const std::string &host, int port, int socket_flags);
./src/httplib.hpp:1442:        int address_family_ = AF_UNSPEC;
./src/httplib.hpp:1521:        explicit ClientImpl(const std::string &host, int port);
./src/httplib.hpp:1523:        explicit ClientImpl(const std::string &host, int port,
./src/httplib.hpp:1717:        int port() const;
./src/httplib.hpp:1729:        void set_address_family(int family);
./src/httplib.hpp:1769:        void set_proxy(const std::string &host, int port);
./src/httplib.hpp:1828:        const int port_;
./src/httplib.hpp:1876:        int address_family_ = AF_UNSPEC;
./src/httplib.hpp:1887:        int proxy_port_ = -1;
./src/httplib.hpp:1959:        explicit Client(const std::string &host, int port);
./src/httplib.hpp:1961:        explicit Client(const std::string &host, int port,
./src/httplib.hpp:2158:        int port() const;
./src/httplib.hpp:2170:        void set_address_family(int family);
./src/httplib.hpp:2209:        void set_proxy(const std::string &host, int port);
./src/httplib.hpp:2235:        long get_openssl_verify_result() const;
./src/httplib.hpp:2284:        explicit SSLClient(const std::string &host, int port);
./src/httplib.hpp:2286:        explicit SSLClient(const std::string &host, int port,
./src/httplib.hpp:2291:        explicit SSLClient(const std::string &host, int port, X509 *client_cert,
./src/httplib.hpp:2302:        long get_openssl_verify_result() const;
./src/httplib.hpp:2336:        long verify_result_ = 0;
./src/httplib.hpp:2417:        inline bool set_socket_opt_impl(socket_t sock, int level, int optname,
./src/httplib.hpp:2429:        inline bool set_socket_opt(socket_t sock, int level, int optname, int optval)
./src/httplib.hpp:2434:        inline bool set_socket_opt_time(socket_t sock, int level, int optname,
./src/httplib.hpp:2441:            timeout.tv_sec = static_cast<long>(sec);
./src/httplib.hpp:2460:    inline const char *status_message(int status)
./src/httplib.hpp:2781:            auto len = static_cast<int>(strlen(s));
./src/httplib.hpp:2789:                if (wlen != static_cast<int>(ws.size()))
./src/httplib.hpp:2810:            int ret_ = -1;
./src/httplib.hpp:2843:                                      int port, int address_family, bool tcp_nodelay,
./src/httplib.hpp:2866:        int close_socket(socket_t sock);
./src/httplib.hpp:2868:        ssize_t send_socket(socket_t sock, const void *ptr, size_t size, int flags);
./src/httplib.hpp:2870:        ssize_t read_socket(socket_t sock, void *ptr, size_t size, int flags);
./src/httplib.hpp:2893:            void get_remote_ip_and_port(std::string &ip, int &port) const override;
./src/httplib.hpp:2894:            void get_local_ip_and_port(std::string &ip, int &port) const override;
./src/httplib.hpp:3069:            int fd_ = -1;
./src/web_server.cpp:98:void WebServer::start(int port)
./src/web_server.cpp:247:bool WebServer::setThreadPriority(int schedPolicy, int priority)
./src/web_server.cpp:255:        int ret = pthread_setschedparam(serverThread.native_handle(), schedPolicy,
./src/WSPR-Transmitter/src/main.cpp:27:static int sig_pipe_fds[2] = {-1, -1};
./src/WSPR-Transmitter/src/main.cpp:57:int getch()
./src/WSPR-Transmitter/src/main.cpp:64:    int ch = getchar();
./src/WSPR-Transmitter/src/main.cpp:85:        int nf = std::max(STDIN_FILENO, sig_pipe_fds[0]) + 1;
./src/WSPR-Transmitter/src/main.cpp:121:        int nf = std::max(STDIN_FILENO, sig_pipe_fds[0]) + 1;
./src/WSPR-Transmitter/src/main.cpp:149:void sig_handler(int)
./src/WSPR-Transmitter/src/main.cpp:190:int main()
./src/WSPR-Transmitter/src/main.cpp:199:    std::array<int, 6> signals = {SIGINT, SIGTERM, SIGHUP, SIGUSR1, SIGUSR2, SIGQUIT};
./src/WSPR-Transmitter/src/main.cpp:200:    for (int s : signals)
./src/WSPR-Transmitter/src/wspr_transmit.cpp:41:#include <cstdint>   // std::uintptr_t
./src/WSPR-Transmitter/src/wspr_transmit.cpp:134:        MMapRegion(int fd, off_t offset, size_t size)
./src/WSPR-Transmitter/src/wspr_transmit.cpp:170:        int fd_;
./src/WSPR-Transmitter/src/wspr_transmit.cpp:187:        int get() const { return fd_; }
./src/WSPR-Transmitter/src/wspr_transmit.cpp:234:        int mbox_fd_;
./src/WSPR-Transmitter/src/wspr_transmit.cpp:245:        MailboxMemoryPool(int mbox_fd, unsigned numpages, uint32_t mem_flag)
./src/WSPR-Transmitter/src/wspr_transmit.cpp:373:    int power,
./src/WSPR-Transmitter/src/wspr_transmit.cpp:377:    int power_dbm,
./src/WSPR-Transmitter/src/wspr_transmit.cpp:411:    int offset_freq = 0;
./src/WSPR-Transmitter/src/wspr_transmit.cpp:505:void WsprTransmitter::setThreadScheduling(int policy, int priority)
./src/WSPR-Transmitter/src/wspr_transmit.cpp:651:        const int symbol_count = static_cast<int>(trans_params_.symbols.size());
./src/WSPR-Transmitter/src/wspr_transmit.cpp:652:        for (int i = 0; i < symbol_count; ++i)
./src/WSPR-Transmitter/src/wspr_transmit.cpp:654:            std::cout << static_cast<int>(trans_params_.symbols[i]);
./src/WSPR-Transmitter/src/wspr_transmit.cpp:777:    int bufPtr = 0;
./src/WSPR-Transmitter/src/wspr_transmit.cpp:800:        const int symbol_count =
./src/WSPR-Transmitter/src/wspr_transmit.cpp:801:            static_cast<int>(trans_params_.symbols.size());
./src/WSPR-Transmitter/src/wspr_transmit.cpp:804:        for (int i = 0; i < symbol_count; ++i)
./src/WSPR-Transmitter/src/wspr_transmit.cpp:824:            int symbol = static_cast<int>(trans_params_.symbols[i]);
./src/WSPR-Transmitter/src/wspr_transmit.cpp:939:constexpr int WsprTransmitter::get_gpio_power_mw(int level)
./src/WSPR-Transmitter/src/wspr_transmit.cpp:941:    if (level < 0 || level >= static_cast<int>(DRIVE_STRENGTH_TABLE.size()))
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1009:    int ret = pthread_setschedparam(pthread_self(), thread_policy_, &sch);
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1030:inline volatile int &WsprTransmitter::access_bus_address(std::uintptr_t bus_addr)
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1033:    std::uintptr_t offset = bus_addr - 0x7E000000UL;
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1040:    return *reinterpret_cast<volatile int *>(base + offset);
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1052:inline void WsprTransmitter::set_bit_bus_address(std::uintptr_t base, unsigned int bit)
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1066:inline void WsprTransmitter::clear_bit_bus_address(std::uintptr_t base, unsigned int bit)
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1080:inline std::uintptr_t WsprTransmitter::bus_to_physical(std::uintptr_t x)
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1095:int WsprTransmitter::symbol_timeval_subtract(struct timeval *result, const struct timeval *t2, const struct timeval *t1)
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1098:    long int diff_usec = (t2->tv_usec + 1000000 * t2->tv_sec) - (t1->tv_usec + 1000000 * t1->tv_sec);
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1153:    const int proc_id = (rev & 0x800000)
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1247:                  << " virt_addr=0x" << reinterpret_cast<unsigned long>(mailbox_.virt_addr)
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1280:                  << std::hex << reinterpret_cast<uintptr_t>(*bAddr)
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1281:                  << " virt_addr=0x" << reinterpret_cast<uintptr_t>(*vAddr)
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1338:    access_bus_address(CM_GP0CTL_BUS) = static_cast<int>(settings);
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1368:    int temp;
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1369:    std::memcpy(&temp, &setupword, sizeof(int));
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1399:    const int &sym_num,
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1401:    int &bufPtr)
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1413:    const int f0_idx = sym_num * 2;
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1414:    const int f1_idx = f0_idx + 1;
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1419:                  << reinterpret_cast<unsigned long>(&instructions_[bufPtr])
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1427:            const long int n_pwmclk = PWM_CLOCKS_PER_ITER_NOMINAL;
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1434:                                  { return stop_requested_.load(std::memory_order_acquire) || static_cast<std::uintptr_t>(
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1435:                                                                                                  access_bus_address(DMA_BUS_BASE + 0x04)) != reinterpret_cast<std::uintptr_t>(instructions_[bufPtr].b); });
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1440:                reinterpret_cast<std::uintptr_t>(const_page_.b) + f0_idx * 4;
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1447:                                  { return stop_requested_.load(std::memory_order_acquire) || static_cast<std::uintptr_t>(
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1448:                                                                                                  access_bus_address(DMA_BUS_BASE + 0x04)) != reinterpret_cast<std::uintptr_t>(instructions_[bufPtr].b); });
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1458:        const long int n_pwmclk_per_sym = std::lround(F_PWM_CLK_INIT * tsym);
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1459:        long int n_pwmclk_transmitted = 0;
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1460:        long int n_f0_transmitted = 0;
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1478:            long int n_pwmclk =
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1485:            const long int n_f0 = std::lround(f0_ratio * (n_pwmclk_transmitted + n_pwmclk)) - n_f0_transmitted;
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1486:            const long int n_f1 = n_pwmclk - n_f0;
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1493:                                  { return stop_requested_.load(std::memory_order_acquire) || static_cast<std::uintptr_t>(
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1494:                                                                                                  access_bus_address(DMA_BUS_BASE + 0x04)) != reinterpret_cast<std::uintptr_t>(instructions_[bufPtr].b); });
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1499:                reinterpret_cast<std::uintptr_t>(const_page_.b) + f0_idx * 4;
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1506:                                  { return stop_requested_.load(std::memory_order_acquire) || static_cast<std::uintptr_t>(
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1507:                                                                                                  access_bus_address(DMA_BUS_BASE + 0x04)) != reinterpret_cast<std::uintptr_t>(instructions_[bufPtr].b); });
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1518:                                  { return stop_requested_.load(std::memory_order_acquire) || static_cast<std::uintptr_t>(
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1519:                                                                                                  access_bus_address(DMA_BUS_BASE + 0x04)) != reinterpret_cast<std::uintptr_t>(instructions_[bufPtr].b); });
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1524:                reinterpret_cast<std::uintptr_t>(const_page_.b) + f1_idx * 4;
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1531:                                  { return stop_requested_.load(std::memory_order_acquire) || static_cast<std::uintptr_t>(
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1532:                                                                                                  access_bus_address(DMA_BUS_BASE + 0x04)) != reinterpret_cast<std::uintptr_t>(instructions_[bufPtr].b); });
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1547:                  << reinterpret_cast<unsigned long>(instructions_[bufPtr].v)
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1549:                  << reinterpret_cast<unsigned long>(instructions_[bufPtr].b)
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1584:double WsprTransmitter::bit_trunc(const double &d, const int &lsb)
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1656:    int instrCnt = 0;
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1667:        for (int i = 0; i < static_cast<int>(PAGE_SIZE / sizeof(struct CB)); i++)
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1670:            instructions_[instrCnt].v = reinterpret_cast<void *>(reinterpret_cast<long int>(instr_page_.v) + sizeof(struct CB) * i);
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1671:            instructions_[instrCnt].b = reinterpret_cast<void *>(reinterpret_cast<long int>(instr_page_.b) + sizeof(struct CB) * i);
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1674:            instr0->SOURCE_AD = reinterpret_cast<unsigned long int>(const_page_.b) + 2048;
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1693:                reinterpret_cast<struct CB *>(instructions_[instrCnt - 1].v)->NEXTCONBK = reinterpret_cast<long int>(instructions_[instrCnt].b);
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1702:    reinterpret_cast<struct CB *>(instructions_[1023].v)->NEXTCONBK = reinterpret_cast<long int>(instructions_[0].b);
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1726:    std::uintptr_t delta = DMA_BUS_BASE - 0x7E000000UL;
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1732:    volatile int *dma_base = reinterpret_cast<volatile int *>(base + delta);
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1739:    DMA0->CONBLK_AD = reinterpret_cast<unsigned long int>(instr_page_.b);
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1763:    int mem_fd = ::open("/dev/mem", O_RDWR | O_SYNC);
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1827:    std::vector<long int> tuning_word(1024);
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1830:    for (int i = 0; i < 8; i++)
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1841:        tuning_word[i] = static_cast<int>(div * std::pow(2.0, 12));
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1845:    for (int i = 8; i < 1024; i++)
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1848:        tuning_word[i] = static_cast<int>(div * std::pow(2.0, 12));
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1852:    for (int i = 0; i < 1024; i++)
./src/WSPR-Transmitter/src/wspr_transmit.cpp:1857:        reinterpret_cast<int *>(const_page_.v)[i] = (0x5A << 24) + tuning_word[i];
./src/WSPR-Transmitter/src/wspr_transmit.hpp:148:        int power = 0,
./src/WSPR-Transmitter/src/wspr_transmit.hpp:152:        int power_dbm = 0,
./src/WSPR-Transmitter/src/wspr_transmit.hpp:176:    void setThreadScheduling(int policy, int priority);
./src/WSPR-Transmitter/src/wspr_transmit.hpp:288:    int thread_policy_ = SCHED_OTHER;
./src/WSPR-Transmitter/src/wspr_transmit.hpp:296:    int thread_priority_ = 0;
./src/WSPR-Transmitter/src/wspr_transmit.hpp:393:    static constexpr int WSPR_RAND_OFFSET = 80;
./src/WSPR-Transmitter/src/wspr_transmit.hpp:409:    static constexpr int WSPR15_RAND_OFFSET = 8;
./src/WSPR-Transmitter/src/wspr_transmit.hpp:527:    static constexpr int BCM_HOST_PROCESSOR_BCM2835 = 0; // BCM2835 (RPi1)
./src/WSPR-Transmitter/src/wspr_transmit.hpp:534:    static constexpr int BCM_HOST_PROCESSOR_BCM2836 = 1; // BCM2836 (RPi2)
./src/WSPR-Transmitter/src/wspr_transmit.hpp:541:    static constexpr int BCM_HOST_PROCESSOR_BCM2837 = 2; // BCM2837 (RPi3)
./src/WSPR-Transmitter/src/wspr_transmit.hpp:548:    static constexpr int BCM_HOST_PROCESSOR_BCM2711 = 3; // BCM2711 (RPi4)
./src/WSPR-Transmitter/src/wspr_transmit.hpp:571:    static inline constexpr std::array<int, 8> DRIVE_STRENGTH_TABLE = {
./src/WSPR-Transmitter/src/wspr_transmit.hpp:588:        int power_dbm;                      ///< Reported transmission power in dBm
./src/WSPR-Transmitter/src/wspr_transmit.hpp:592:        int power;                          ///< GPIO power level 0-7
./src/WSPR-Transmitter/src/wspr_transmit.hpp:639:        int mem_flag;                  ///< Mailbox memory allocation flags for DMA.
./src/WSPR-Transmitter/src/wspr_transmit.hpp:706:        int handle = -1;       ///< mailbox fd
./src/WSPR-Transmitter/src/wspr_transmit.hpp:734:        volatile unsigned int TI;        ///< Transfer Information field for DMA control.
./src/WSPR-Transmitter/src/wspr_transmit.hpp:735:        volatile unsigned int SOURCE_AD; ///< Source bus address for the DMA transfer.
./src/WSPR-Transmitter/src/wspr_transmit.hpp:736:        volatile unsigned int DEST_AD;   ///< Destination bus address for the DMA transfer.
./src/WSPR-Transmitter/src/wspr_transmit.hpp:737:        volatile unsigned int TXFR_LEN;  ///< Length (in bytes) of the transfer.
./src/WSPR-Transmitter/src/wspr_transmit.hpp:738:        volatile unsigned int STRIDE;    ///< 2D stride value (unused if single block).
./src/WSPR-Transmitter/src/wspr_transmit.hpp:739:        volatile unsigned int NEXTCONBK; ///< Bus address of the next CB in the chain.
./src/WSPR-Transmitter/src/wspr_transmit.hpp:740:        volatile unsigned int RES1;      ///< Reserved, must be zero.
./src/WSPR-Transmitter/src/wspr_transmit.hpp:741:        volatile unsigned int RES2;      ///< Reserved, must be zero.
./src/WSPR-Transmitter/src/wspr_transmit.hpp:811:        volatile unsigned int CS;        ///< Control/Status register.
./src/WSPR-Transmitter/src/wspr_transmit.hpp:812:        volatile unsigned int CONBLK_AD; ///< Address of the current control block.
./src/WSPR-Transmitter/src/wspr_transmit.hpp:813:        volatile unsigned int TI;        ///< Transfer Information register.
./src/WSPR-Transmitter/src/wspr_transmit.hpp:814:        volatile unsigned int SOURCE_AD; ///< Source address for data transfer.
./src/WSPR-Transmitter/src/wspr_transmit.hpp:815:        volatile unsigned int DEST_AD;   ///< Destination address for data transfer.
./src/WSPR-Transmitter/src/wspr_transmit.hpp:816:        volatile unsigned int TXFR_LEN;  ///< Transfer length (in bytes).
./src/WSPR-Transmitter/src/wspr_transmit.hpp:817:        volatile unsigned int STRIDE;    ///< Stride for address increment.
./src/WSPR-Transmitter/src/wspr_transmit.hpp:818:        volatile unsigned int NEXTCONBK; ///< Address of the next control block.
./src/WSPR-Transmitter/src/wspr_transmit.hpp:819:        volatile unsigned int DEBUG;     ///< Debug register for diagnostics.
./src/WSPR-Transmitter/src/wspr_transmit.hpp:898:    constexpr int get_gpio_power_mw(int level);
./src/WSPR-Transmitter/src/wspr_transmit.hpp:920:    inline volatile int &access_bus_address(std::uintptr_t bus_addr);
./src/WSPR-Transmitter/src/wspr_transmit.hpp:931:    inline void set_bit_bus_address(std::uintptr_t base, unsigned int bit);
./src/WSPR-Transmitter/src/wspr_transmit.hpp:942:    inline void clear_bit_bus_address(std::uintptr_t base, unsigned int bit);
./src/WSPR-Transmitter/src/wspr_transmit.hpp:953:    inline std::uintptr_t bus_to_physical(std::uintptr_t x);
./src/WSPR-Transmitter/src/wspr_transmit.hpp:965:    int symbol_timeval_subtract(struct timeval *result, const struct timeval *t2, const struct timeval *t1);
./src/WSPR-Transmitter/src/wspr_transmit.hpp:1049:        const int &sym_num,
./src/WSPR-Transmitter/src/wspr_transmit.hpp:1051:        int &bufPtr);
./src/WSPR-Transmitter/src/wspr_transmit.hpp:1068:    double bit_trunc(const double &d, const int &lsb);
./src/WSPR-Transmitter/src/wspr_transmit.hpp:1206:            const int cycle = (parent_->trans_params_.wspr_mode == WsprMode::WSPR15)
./src/WSPR-Transmitter/external/config_handler.hpp:27:    int power_dbm = 0;
./src/WSPR-Transmitter/external/config_handler.hpp:33:    int tx_pin = -1;
./src/WSPR-Transmitter/external/config_handler.hpp:45:    int power_level = 7;
./src/WSPR-Transmitter/external/config_handler.hpp:51:    int led_pin = -1;
./src/WSPR-Transmitter/external/config_handler.hpp:54:    int web_port = -1;
./src/WSPR-Transmitter/external/config_handler.hpp:57:    int socket_port = -1;
./src/WSPR-Transmitter/external/config_handler.hpp:63:    int shutdown_pin = -1;
./src/WSPR-Transmitter/external/config_handler.hpp:72:    int tx_iterations = 0;
./src/WSPR-Transmitter/external/utils.cpp:40:    int status = pclose(pipe);
./src/WSPR-Transmitter/external/utils.cpp:62:        int exit_code = WEXITSTATUS(status);
./src/WSPR-Transmitter/external/utils.cpp:74:        int signal = WTERMSIG(status);
./src/sha1.hpp:96:    static uint32_t leftrotate(uint32_t value, unsigned int bits);
./src/LCBLog/src/main.cpp:48:    auto logTask = [](int threadId)
./src/LCBLog/src/main.cpp:50:        for (int i = 0; i < 5; ++i)
./src/LCBLog/src/main.cpp:58:    for (int i = 0; i < 10; ++i)
./src/LCBLog/src/main.cpp:277:    assert(shouldSkipSpace(42, "Word") == false);   // int should not break it
./src/LCBLog/src/main.cpp:279:    assert(shouldSkipSpace("Word", 100) == false);  // No space impact for int
./src/LCBLog/src/main.cpp:298:    llog.logS(INFO, "Test start:", 42, "is an int,", 3.14159, "is Pi,", -7.25,
./src/LCBLog/src/main.cpp:320:int main()
./src/arg_parser.cpp:134:const std::vector<int> wspr_power_levels = {0, 3, 7, 10, 13, 17, 20, 23, 27, 30, 33, 37, 40, 43, 47, 50, 53, 57, 60};
./src/arg_parser.cpp:223:int round_to_nearest_wspr_power(int power)
./src/arg_parser.cpp:226:    int closest = wspr_power_levels[0];
./src/arg_parser.cpp:227:    int min_diff = std::abs(power - closest);
./src/arg_parser.cpp:230:    for (int level : wspr_power_levels)
./src/arg_parser.cpp:232:        int diff = std::abs(power - level);
./src/arg_parser.cpp:346:void print_usage(const std::string &message, int exit_code)
./src/arg_parser.cpp:831:bool parse_command_line(int argc, char *argv[])
./src/arg_parser.cpp:887:        int option_index = 0;
./src/arg_parser.cpp:888:        int c = getopt_long(argc, argv, "h?vnroDp:x:t:a:l:s:d:w:k:", long_options, &option_index);
./src/arg_parser.cpp:1004:                int transmit_pin = std::stoi(optarg);
./src/arg_parser.cpp:1020:                int led_pin = std::stoi(optarg);
./src/arg_parser.cpp:1042:                int shutdown_pin = std::stoi(optarg);
./src/arg_parser.cpp:1064:                int power = std::stoi(optarg);
./src/arg_parser.cpp:1085:                int port = std::stoi(optarg);
./src/arg_parser.cpp:1107:                int port = std::stoi(optarg);
./src/arg_parser.cpp:1139:            for (int i = optind; i < argc; ++i)
./src/arg_parser.cpp:1178:                int power = std::stoi(positional_args[2]);
./src/arg_parser.cpp:1179:                int rounded_power = round_to_nearest_wspr_power(power);
./src/gpio_input.cpp:85:bool GPIOInput::enable(int pin, bool trigger_high, PullMode pull_mode, std::function<void()> callback)
./src/gpio_input.cpp:206:bool GPIOInput::setPriority(int schedPolicy, int priority)
./src/gpio_input.cpp:219:    int ret = pthread_setschedparam(monitor_thread_.native_handle(), schedPolicy, &sch_params);
```
