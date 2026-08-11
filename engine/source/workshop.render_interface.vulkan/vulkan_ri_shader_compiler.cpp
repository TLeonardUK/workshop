// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.vulkan/vulkan_ri_shader_compiler.h"
#include "workshop.render_interface.vulkan/vulkan_ri_interface.h"
#include "workshop.core/filesystem/virtual_file_system.h"

#include <regex>
#include <unordered_set>

namespace ws {

namespace {

std::string clean_include_path(const std::string& input)
{
    std::string path = input;

    size_t first_alias = path.find_first_of(":");
    size_t last_alias_end = path.find_last_of(':');

    if (first_alias != last_alias_end &&
        first_alias != std::string::npos)
    {
        size_t last_alias_start = path.find_last_of('/', last_alias_end);
        if (last_alias_start != std::string::npos)
        {
            path.erase(path.begin(), path.begin() + last_alias_start + 1);
        }
    }

    return path;
}

class vulkan_shader_include_handler : public IDxcIncludeHandler
{
public:
    vulkan_shader_include_handler(CComPtr<IDxcLibrary>& library)
        : m_library(library)
    {
    }

    virtual ~vulkan_shader_include_handler()
    {
    }

    HRESULT STDMETHODCALLTYPE LoadSource(_In_ LPCWSTR pFilename, _COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource) override
    {
        CComPtr<IDxcBlobEncoding> pEncoding;
        std::string path = clean_include_path(narrow_string(std::wstring(pFilename)));

        included_files.insert(path);

        std::string source = "";
        {
            std::unique_ptr<stream> stream = virtual_file_system::get().open(path.c_str(), false);
            if (!stream)
            {
                db_error(asset, "Failed to open stream to shader source.", path);
                return E_FAIL;
            }

            source = stream->read_all_string();
        }

        CComPtr<IDxcBlobEncoding> sourceBlob;
        HRESULT ret = m_library->CreateBlobWithEncodingOnHeapCopy(source.c_str(), static_cast<UINT>(strlen(source.c_str())), CP_UTF8, &sourceBlob);
        if (FAILED(ret))
        {
            db_error(asset, "CreateBlob failed with hresult 0x%08x", ret);
            return E_FAIL;
        }

        *ppIncludeSource = sourceBlob.Detach();

        return S_OK;
    }

    HRESULT QueryInterface(REFIID riid, void** ppvObject) override
    {
        return 0;
    }

    ULONG AddRef() override
    {
        return 0;
    }

    ULONG Release() override
    {
        return 0;
    }

public:
    std::unordered_set<std::string> included_files;

private:
    CComPtr<IDxcLibrary> m_library;

};

}; // namespace

vulkan_ri_shader_compiler::vulkan_ri_shader_compiler(vulkan_render_interface& renderer)
    : m_renderer(renderer)
{
}

vulkan_ri_shader_compiler::~vulkan_ri_shader_compiler()
{
    m_library = nullptr;
    m_compiler = nullptr;
}

result<void> vulkan_ri_shader_compiler::create_resources()
{
    HRESULT ret = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&m_library));
    if (FAILED(ret))
    {
        db_error(render_interface, "Failed to create DxcLibrary with error 0x%08x.", ret);
        return false;
    }

    ret = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_compiler));
    if (FAILED(ret))
    {
        db_error(render_interface, "Failed to create DxcCompiler with error 0x%08x.", ret);
        return false;
    }

    return true;
}

void vulkan_ri_shader_compiler::parse_output(ri_shader_compiler_output& output, const std::string& text)
{
    std::vector<std::string> lines = string_split(text, "\n");

    std::regex error_pattern("(.+):(\\d+):(\\d+): ([\\s\\w]+): (.+)", std::regex_constants::ECMAScript | std::regex_constants::icase);

    ri_shader_compiler_output::log log;
    std::string log_type = "";

    auto push_output = [&]()
    {
        if (_stricmp(log_type.c_str(), "error") == 0 ||
            _stricmp(log_type.c_str(), "fatal") == 0 ||
            _stricmp(log_type.c_str(), "fatal error") == 0)
        {
            output.push_error(log);
        }
        else if (_stricmp(log_type.c_str(), "warning") == 0)
        {
            output.push_warning(log);
        }
        else
        {
            output.push_message(log);
        }

        log.file = "";
    };

    for (size_t i = 0; i < lines.size(); i++)
    {
        std::string& line = lines[i];

        if (line.find("In file included from") != std::string::npos ||
            line.find("expanded from macro") != std::string::npos)
        {
            if (!log.file.empty())
            {
                push_output();
            }
            continue;
        }

        std::smatch match;
        if (std::regex_match(line, match, error_pattern))
        {
            if (!log.file.empty())
            {
                push_output();
            }

            log.file = clean_include_path(match[1].str());
            log.line = from_string<size_t>(match[2].str()).get();
            log.column = from_string<size_t>(match[3].str()).get();
            log_type = match[4].str();
            log.message = match[5].str();
            log.context.clear();
        }
        else
        {
            log.context.push_back(line);
        }
    }

    if (!log.file.empty())
    {
        push_output();
    }
}

ri_shader_compiler_output vulkan_ri_shader_compiler::compile(
    ri_shader_stage stage,
    const char* source,
    const char* file,
    const char* entrypoint,
    std::unordered_map<std::string, std::string>& defines,
    bool debug)
{
    defines["TARGET_VULKAN"] = "1";

    std::array<std::wstring, static_cast<int>(ri_shader_stage::COUNT)> stage_target_profiles = {
        L"vs_6_3",
        L"ps_6_3",
        L"ds_6_3",
        L"hs_6_3",
        L"gs_6_3",
        L"cs_6_3",

        L"lib_6_3",
        L"lib_6_3",
        L"lib_6_3",
        L"lib_6_3",
        L"lib_6_3",
    };

    std::wstring wide_target_profile = stage_target_profiles[static_cast<int>(stage)];
    std::wstring wide_file = widen_string(file);
    std::wstring wide_entrypoint = widen_string(entrypoint);

    ri_shader_compiler_output output;

    std::vector<std::wstring> arguments;

    arguments.push_back(L"-HV 2018");
    arguments.push_back(L"-spirv");
    arguments.push_back(L"-fspv-target-env=vulkan1.2");
    arguments.push_back(L"-Qembed_debug");
    arguments.push_back(L"-Zi");
    arguments.push_back(L"-O0");

    CComPtr<IDxcBlobEncoding> sourceBlob;
    HRESULT ret = m_library->CreateBlobWithEncodingOnHeapCopy(source, static_cast<UINT>(strlen(source)), CP_UTF8, &sourceBlob);
    if (FAILED(ret))
    {
        output.push_error({ string_format("CreateBlob failed with hresult 0x%08x", ret), file, 0, 0 });
        return output;
    }

    std::vector<LPCWSTR> argument_ptrs;
    for (size_t i = 0; i < arguments.size(); i++)
    {
        argument_ptrs.push_back(arguments[i].c_str());
    }

    std::vector<DxcDefine> dxc_defines;
    std::vector<std::wstring> define_names;
    std::vector<std::wstring> define_values;
    for (std::pair<std::string, std::string> pair : defines)
    {
        define_names.push_back(widen_string(pair.first));
        define_values.push_back(widen_string(pair.second));
    }

    for (size_t i = 0; i < define_names.size(); i++)
    {
        dxc_defines.push_back({
            define_names[i].c_str(),
            define_values[i].empty() ? nullptr : define_values[i].c_str()
        });
    }

    vulkan_shader_include_handler include_handler(m_library);

    CComPtr<IDxcOperationResult> compile_result;

    ret = m_compiler->Compile(
        sourceBlob,
        wide_file.c_str(),
        wide_entrypoint.c_str(),
        wide_target_profile.c_str(),
        argument_ptrs.data(),
        static_cast<UINT32>(argument_ptrs.size()),
        dxc_defines.data(),
        static_cast<UINT32>(dxc_defines.size()),
        &include_handler,
        &compile_result);

    if (FAILED(ret))
    {
        output.push_error({ string_format("Compile failed with hresult 0x%08x", ret), file, 0, 0 });
        return output;
    }

    CComPtr<IDxcBlobEncoding> error_blob;
    ret = compile_result->GetErrorBuffer(&error_blob);

    HRESULT compileRet;
    compile_result->GetStatus(&compileRet);

    if (FAILED(ret))
    {
        output.push_error({ string_format("Failed to get compile result with error 0x%08x", ret), file, 0, 0 });
        return output;
    }
    if (error_blob)
    {
        const char* ptr = reinterpret_cast<const char*>(error_blob->GetBufferPointer());
        std::string errors_string(ptr, ptr + error_blob->GetBufferSize());

        parse_output(output, errors_string);

        if (FAILED(compileRet))
        {
            db_log(render_interface, "=============== COMPILE RESULT ===============");
            db_log(render_interface, "%s", source);
            db_log(render_interface, "%s", errors_string.c_str());
        }

        if (!SUCCEEDED(compileRet))
        {
            if (output.get_messages().empty() &&
                output.get_warnings().empty() &&
                output.get_errors().empty())
            {
                ri_shader_compiler_output::log log;
                log.column = 0;
                log.file = file;
                log.line = 0;
                log.message = errors_string;
                output.push_error(log);
            }
        }
    }

    if (SUCCEEDED(compileRet))
    {
        CComPtr<IDxcBlob> bytecode_blob;

        ret = compile_result->GetResult(&bytecode_blob);
        if (FAILED(ret))
        {
            output.push_error({ string_format("Failed to get compile result with error 0x%08x", ret), file, 0, 0 });
            return output;
        }

        std::vector<uint8_t> bytecode;
        bytecode.resize(bytecode_blob->GetBufferSize());
        memcpy(bytecode.data(), reinterpret_cast<uint8_t*>(bytecode_blob->GetBufferPointer()), bytecode.size());
        output.set_bytecode(std::move(bytecode));

        for (std::string dependency : include_handler.included_files)
        {
            output.push_dependency(dependency);
        }
    }

    return output;
}

}; // namespace ws
