#include <future>
#include <string>
#include <mutex>
#include <functional>

// Third party
#include <atlbase.h>
#include <comutil.h>
#include <msxml6.h>

// STL
#include <locale>

using ArgumentKey = std::string;
using ArgumentValue = std::string;
using Argument = std::pair<ArgumentKey, ArgumentValue>;
using Arguments = std::vector<Argument>;
using Body = std::vector<uint8_t>;
using ErrorCallback = std::function<void(long, std::string)>;
using GetCallback = std::function<void(std::string)>;

int request_count = 0;

std::string packArguments(const Arguments& arguments)
{
    std::string args;
    bool first = true;
    for (auto &kv : arguments)
    {
        if (first)
        {
            first = false;
            args += kv.first + "=" + kv.second;
        }
        else
        {
            args += "&" + kv.first + "=" + kv.second;
        }
    }
    return std::move(args);
}

#if defined(WIN32)
#include <codecvt>

std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> g_stringConverter;

std::wstring utf8ToUtf16(const std::string& utf8)
{
    return g_stringConverter.from_bytes(utf8);
}

std::string utf16ToUtf8(const std::wstring& utf16)
{
    return g_stringConverter.to_bytes(utf16);
}

Body httpGet(const std::string& url, Arguments arguments, const ErrorCallback& onError)
{
    arguments.push_back({ "time", std::to_string(std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()).time_since_epoch().count()) });
    auto args = packArguments(arguments);
    auto fullUrl = url;
    if (!args.empty())
    {
        fullUrl = url + "?" + args;
    }

    HRESULT hr;
    CComPtr<IXMLHTTPRequest> request;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    hr = request.CoCreateInstance(CLSID_XMLHTTP60);
    if (hr != S_OK)
    {
        if (onError)
        {
            onError(0, "error");
        }
        return{};
    }
    hr = request->open(_bstr_t("GET"),
        _bstr_t(fullUrl.c_str()),
        _variant_t(VARIANT_FALSE),
        _variant_t(),
        _variant_t());
    if (hr != S_OK)
    {
        if (onError)
        {
            onError(0, "error");
        }
        return{};
    }
    //hr = request->setRequestHeader(_bstr_t("Cache-Control"), _bstr_t("no-cache"));
    //if (hr != S_OK)
    //{
    //    if (onError)
    //    {
    //        onError(0, "error");
    //    }
    //    return{};
    //}
    hr = request->send(_variant_t());
    if (hr != S_OK)
    {
        if (onError)
        {
            onError(0, "error");
        }
        return{};
    }

    // get status - 200 if succuss
    long status;
    hr = request->get_status(&status);
    if (hr != S_OK)
    {
        if (onError)
        {
            onError(0, "error");
        }
        return{};
    }

    // Load the texture
    VARIANT body;
    request->get_responseBody(&body);

    if (status == 200)
    {
        auto pData = (const uint8_t*)body.parray->pvData;
        return{ pData, pData + body.parray->rgsabound[0].cElements };
    }
    else
    {
        if (onError)
        {
            BSTR bstrResponse = NULL;
            request->get_responseText(&bstrResponse);
            std::wstring wret = bstrResponse;
            if (bstrResponse) SysFreeString(bstrResponse);
            std::string ret = utf16ToUtf8(wret);
            onError(status, ret);
        }
        return{};
    }
}
#else
#endif

std::mutex net_mutex;
std::vector<std::function<void()>> net_callbacks;

void update_net()
{
    net_mutex.lock();
    for (auto& f : net_callbacks)
    {
        f();
        --request_count;
    }
    net_callbacks.clear();
    net_mutex.unlock();
}

void OSync(const std::function<void()>& fn)
{
    net_mutex.lock();
    net_callbacks.push_back(fn);
    net_mutex.unlock();
}

void httpGetAsync(const std::string& url, const Arguments& arguments, const GetCallback& onSuccess, const ErrorCallback& onError)
{
    ++request_count;
    std::async(std::launch::async, [url, arguments, onSuccess, onError]
    {
        auto ret = httpGet(url, arguments, [onError](long errCode, std::string message)
        {
            if (onError)
            {
                std::async(std::launch::async, [errCode, message, onError]
                {
                    onError(errCode, message);
                });
            }
        });
        if (!ret.empty() && onSuccess)
        {
            std::string ret_str = { reinterpret_cast<const char*>(ret.data()), ret.size() };
            OSync([ret_str, onSuccess]
            {
                onSuccess(ret_str);
            });
        }
    });
}
