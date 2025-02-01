#include "http.h"
#include <cstdint>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace server{

namespace http{

struct InitHelper;

std::unordered_map<std::string, HttpMethod> HttpString2MethodMap;
std::unordered_map<HttpMethod, std::string> HttpMethod2StringMap;
std::unordered_map<std::string, HttpStatus> HttpString2StatusMap;
std::unordered_map<HttpStatus, std::string> HttpStatus2StringMap;

void init(){
    #define XX(num, name, string) HttpString2MethodMap[#name] = HttpMethod::HTTP_##name;
    #define YY(num, name, string) HttpMethod2StringMap[HttpMethod::HTTP_##name] = #name;
        HTTP_METHOD_MAP(YY)
        HTTP_METHOD_MAP(XX)
    #undef YY
    #undef XX
    
    #define XX(num, name, string) HttpString2StatusMap[#num] = HttpStatus::HTTP_STATUS_##name;
    #define YY(num, name, string) HttpStatus2StringMap[HttpStatus::HTTP_STATUS_##name] = #string;
        HTTP_STATUS_MAP(XX)
        HTTP_STATUS_MAP(YY)
    #undef YY
    #undef XX
}

struct InitHelper{
    InitHelper(){
        init();
    }
} static inithelper;

HttpMethod String2HttpMethod(const std::string& s){
    if (HttpString2MethodMap.contains(s))
        return HttpString2MethodMap[s];
    return HttpMethod::HTTP_METHOD_INVALID;
}

HttpStatus String2HttpStatus(const std::string& s){
    if (HttpString2StatusMap.contains(s))
        return HttpString2StatusMap[s];
    return HttpStatus::HTTP_STATUS_INVALID;
}

std::string HttpMethod2String(HttpMethod m){
    if (HttpMethod2StringMap.contains(m))
        return HttpMethod2StringMap[m];
    return "Invalid Method";
}

std::string HttpStatus2String(HttpStatus m){
    if (HttpStatus2StringMap.contains(m))
        return HttpStatus2StringMap[m];
    return "Invalid Status";
}

std::string HttpVersion2String(uint8_t v){
    switch (v)
    {
    case 0x11:
        return "HTTP/1.1";
    case 0x10:
        return "HTTP/1.0";
    case 0x20:
        return "HTTP/2.0";
    case 0x30:
        return "HTTP/3.0";
    default:
        return "Invalid HTTP Version";
    }
}

std::ostream& HttpRequest::dump(std::ostream& os) const {
    os << HttpMethod2String(m_method) << " "
        << m_path << (m_query.empty() ? "" : "?") << m_query << (m_fragment.empty() ? "" : "#") << m_fragment << " "
        << "HTTP/" << (uint32_t)(m_version >> 4) << "." << (uint32_t)(m_version & 0x0F)
        << "\r\n";
    os << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";
    for (auto &[key, value] : m_headers){
        os << key << ": " << value << "\r\n";
    }
    if(!m_body.empty()){
        os << "content-length: " << m_body.size() << "\r\n\r\n" 
        << m_body;
    }
    else{
        os << "\r\n";
    }
    return os;
}

std::string HttpRequest::toString() const{
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

HttpRequest::HttpRequest(uint8_t version, bool close)
    :   m_method(HTTP_GET),
        m_close(close),
        m_version(version)
{

}

std::ostream& HttpResponse::dump(std::ostream& os) const {
    os << "HTTP/" << (uint32_t)(m_version >> 4) << "." << (uint32_t)(m_version & 0x0f) << " "
        << (uint32_t)m_status << " "
        << (m_reason.empty() ? HttpStatus2String(m_status) : m_reason)
        << "\r\n";
    for(auto &[key, value] : m_headers){
        os << key << ": " << value << "\r\n";
    }
    os << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";
    if(!m_body.empty()){
        os << "content-length: " << m_body.size() << "\r\n\r\n"
        << m_body;
    }
    else{
        os << "\r\n\r\n";
    }
    return os;
}

std::string HttpResponse::toString() const{
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

HttpResponse::HttpResponse(uint8_t version, bool close)
    :   m_close(close),
        m_version(version)
{

}

} // namespace http

} // namespace server
