# Gera uma cópia instrumentada do arquivo do SDK; nunca edita o checkout baixado.
option(BLACK_TRACE_MODELS "Compilar diagnóstico optativo de leituras de bg1_pst.db" OFF)
option(BLACK_MODEL_OVERRIDES "Priorizar DB externo da pistola na pasta Black" ON)
if(BLACK_TRACE_MODELS OR BLACK_MODEL_OVERRIDES)
    set(trace_sdk_file "${SDK}/runtime/kernel/files.cpp")
    file(READ "${trace_sdk_file}" trace_text)
    set(trace_patched "${trace_text}")
    function(trace_replace needle replacement expected)
        string(LENGTH "${trace_patched}" before)
        string(REPLACE "${needle}" "" stripped "${trace_patched}")
        string(LENGTH "${stripped}" after)
        string(LENGTH "${needle}" length)
        math(EXPR count "(${before} - ${after}) / ${length}")
        if(NOT count EQUAL expected)
            message(FATAL_ERROR "Diagnóstico de modelos: SDK incompatível na resolução de nomes")
        endif()
        string(REPLACE "${needle}" "${replacement}" changed "${trace_patched}")
        set(trace_patched "${changed}" PARENT_SCOPE)
    endfunction()
    if(BLACK_MODEL_OVERRIDES)
        trace_replace([=[    if (best->first == kCdRom)
        if (const Disc* disc = MountedDisc()) return {{}, true, disc->Find(parts)};
    return {MatchCase(best->second, parts).string()};]=] [=[    if (best->first == kCdRom)
        if (const Disc* disc = MountedDisc()) {
            const auto* entry = disc->Find(parts);
            std::string external = model_override::Find(parts, entry ? entry->size : 0);
            if (!external.empty()) return {external};
            return {{}, true, entry};
        }
    auto host = MatchCase(best->second, parts);
    if (!model_override::Wanted(parts)) return {host.string()};
    std::error_code model_error;
    uint64_t model_size = fs::file_size(host, model_error);
    std::string external = model_override::Find(parts, model_error ? 0 : model_size);
    return {external.empty() ? host.string() : external};]=] 1)
        set(trace_patched "#include \"model_override.h\"\n${trace_patched}")
    endif()
    if(BLACK_TRACE_MODELS)
    set(trace_anchor "        n += uint32_t(done);")
    string(REGEX MATCHALL "n \\+= uint32_t\\(done\\)" trace_hits "${trace_text}")
    list(LENGTH trace_hits trace_count)
    if(NOT trace_count EQUAL 1)
        message(FATAL_ERROR "Diagnóstico de modelos: SDK incompatível; revisar Transfer antes de instrumentar")
    endif()
    set(trace_insert "        if (!write && done && model_trace::Wanted(f->trace_path)) {\n            std::error_code model_error;\n            uint64_t model_size = f->entry ? f->entry->size : fs::file_size(f->host, model_error);\n            if (!model_error) model_trace::Read(f->trace_path, model_size, pos + n, HostPtr(at), done);\n        }\n${trace_anchor}")
    string(REPLACE "${trace_anchor}" "${trace_insert}" trace_patched "${trace_patched}")
    set(trace_patched "#include \"model_trace.h\"\n${trace_patched}")
    trace_replace([=[    std::string xbox;]=] [=[    std::string xbox;
    std::string trace_path;  // Nome completo somente para diagnóstico.]=] 1)
    trace_replace([=[std::string ObjectName(]=] [=[std::string ModelTracePath(const std::string& name, uint32_t root) {
    std::string path = Normalize(name);
    if (auto parent = FileFromHandle(root))
        path = Normalize((parent->trace_path.empty() ? parent->xbox : parent->trace_path) + "\\" + path);
    return path;
}

std::string ObjectName(]=] 1)
    trace_replace([=[    f->host = host;
    f->xbox = Normalize(name);]=] [=[    f->host = host;
    f->trace_path = ModelTracePath(name, root);
    f->xbox = Normalize(name);]=] 2)
    trace_replace([=[        f->host = host;
        f->xbox = Normalize(name);]=] [=[        f->host = host;
        f->trace_path = ModelTracePath(name, root);
        f->xbox = Normalize(name);]=] 1)
    trace_replace([=[if (t.disc) return OpenOnDisc(t.entry, name, handle_out, iosb, disposition, options);]=] [=[if (t.disc) {
        uint32_t status = OpenOnDisc(t.entry, name, handle_out, iosb, disposition, options);
        if (status == kSuccess)
            if (auto opened = FileFromHandle(R32(handle_out))) opened->trace_path = ModelTracePath(name, root);
        return status;
    }]=] 1)
    trace_replace([=[if (t.disc) return OpenOnDisc(t.entry, name, handle_out, iosb, 1, options);]=] [=[if (t.disc) {
        uint32_t status = OpenOnDisc(t.entry, name, handle_out, iosb, 1, options);
        if (status == kSuccess)
            if (auto opened = FileFromHandle(R32(handle_out))) opened->trace_path = ModelTracePath(name, root);
        return status;
    }]=] 1)
        add_compile_definitions(BLACK_TRACE_MODELS=1)
    endif()
    set(trace_output "${CMAKE_CURRENT_BINARY_DIR}/model_trace_files.cpp")
    file(CONFIGURE OUTPUT "${trace_output}" CONTENT "${trace_patched}" @ONLY)
    list(REMOVE_ITEM RUNTIME_SOURCES "${trace_sdk_file}")
    list(APPEND RUNTIME_SOURCES "${trace_output}")
endif()
