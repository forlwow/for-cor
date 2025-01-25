#include "http_parser.h"
#include "http.h"
#include "http_parse/http11_parser.h"
#include "log.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <pthread.h>
#include <socketfunc_cpp20.h>
#include <sys/types.h>

namespace server{

namespace http{

static Logger::ptr s_log = SERVER_LOGGER_SYSTEM;

static const uint64_t s_http_request_buffer_size = 1 << 12;   // 请求体最大大小 4KB
static const uint64_t s_http_request_max_body_size = 1 << 26; // 请求内容最大大小 64MB


// 读取到Request Method触发的回调 适配http_parse
void on_request_method(void *data, const char*at, size_t length){
    HttpRequestParser_v1* parser = static_cast<HttpRequestParser_v1*>(data);
    HttpMethod m = String2HttpMethod(std::string(at, length));

    if(m == HTTP_METHOD_INVALID){
        // TODO: set error num
        parser->SetError(INVALID_METHOD);
        return ;
    }
    parser->GetData()->SetMethod(m);
}
void on_request_uri(void *data, const char*at, size_t length){

}
void on_request_fragment(void *data, const char*at, size_t length){
    HttpRequestParser_v1* parser = static_cast<HttpRequestParser_v1*>(data);
    parser->GetData()->SetFragment(std::string(at, length));
}
void on_request_path(void *data, const char*at, size_t length){
    HttpRequestParser_v1* parser = static_cast<HttpRequestParser_v1*>(data);
    parser->GetData()->SetPath(std::string(at, length));
}
void on_request_query(void *data, const char*at, size_t length){
    HttpRequestParser_v1* parser = static_cast<HttpRequestParser_v1*>(data);
    parser->GetData()->SetQuery(std::string(at, length));
}
void on_request_version(void *data, const char*at, size_t length){
    HttpRequestParser_v1* parser = static_cast<HttpRequestParser_v1*>(data);
    int v;
    if(strncmp(at, "HTTP/1.1", length) == 0){
        v = 0x11;
    }
    else if(strncmp(at, "HTTP/1.0", length) == 0){
        v = 0x10;
    }
    else{
        parser->SetError(INVALID_HTTP_VERSION);
        return ;
    }
    parser->GetData()->SetVersion(v);
}
void on_request_header_done(void *data, const char*at, size_t length){

}

void on_request_http_field(void *data, const char* field, size_t flen, const char *value, size_t vlen){
    HttpRequestParser_v1* parser = static_cast<HttpRequestParser_v1*>(data);
    if (flen == 0){
        parser->SetError(INVALID_FIELD_LEN);
    }
    parser->GetData()->SetHeaders(std::string(field, flen), std::string(value, vlen));
}



HttpRequestParser_v1::HttpRequestParser_v1()
{
    m_data.reset(new HttpRequest);
    http_parser_init(&m_parser);
    m_parser.request_method = on_request_method;
    m_parser.request_uri = on_request_uri;
    m_parser.fragment = on_request_fragment;
    m_parser.request_path = on_request_path;
    m_parser.query_string = on_request_query;
    m_parser.http_version = on_request_version;
    m_parser.header_done = on_request_header_done;
    m_parser.http_field = on_request_http_field;
    m_parser.data = this;
}

size_t HttpRequestParser_v1::execute(char* data, size_t len, size_t off){
    size_t offset = http_parser_execute(&m_parser, data, len, off);
    memmove(data, (data + offset), (len- offset));
    return offset;
}

int HttpRequestParser_v1::isFinished() const {
    return http_parser_is_finished(&m_parser);
}

std::string HttpRequestParser_v1::GetErrstr() const {
    std::string res ;
    int err = GetError();
    if(err & HTTP_PARSER_ERROR::PARSRE_ERROR){
        res.append("PARSRE_ERROR");
    }
    if(err & HTTP_PARSER_ERROR::INVALID_HTTP_VERSION){
        res.append("INVALID_HTTP_VERSION");
    }
    if(err & HTTP_PARSER_ERROR::INVALID_FIELD_LEN){
        res.append("INVALID_FIELD_LEN");
    }
    if(err & HTTP_PARSER_ERROR::INVALID_METHOD){
        res.append("INVALID_METHOD");
    }
    if(res.empty()){
        return "NOERROR";
    }
    return res;  
}

void on_response_reason(void *data, const char *at, size_t length){
    HttpResponseParser_v1 *parser = static_cast<HttpResponseParser_v1*>(data);
    parser->GetData()->SetReason(std::string(at, length));
}
void on_response_status(void *data, const char *at, size_t length){
    HttpResponseParser_v1 *parser = static_cast<HttpResponseParser_v1*>(data);
    parser->GetData()->SetStatus(String2HttpStatus(std::string(at, length)));
}
void on_response_chunk(void *data, const char *at, size_t length){
}
void on_response_version(void *data, const char *at, size_t length){
    HttpResponseParser_v1 *parser = static_cast<HttpResponseParser_v1*>(data);
    uint8_t v = 0;
    if(strncmp(at, "HTTP/1.1", length) == 0){
        v = 0x11;
    }
    else if (strncmp(at, "HTTP/1.0", length) == 0){
        v = 0x10;
    }
    else{
        parser->SetError(INVALID_HTTP_VERSION);
        return ;
    }
    parser->GetData()->SetVersion(v); 
}
void on_response_header_done(void *data, const char *at, size_t lenght){

}
void on_response_last_chunk(void *data, const char *at, size_t lenght){

}

void on_response_http_field(void *data, const char *field, size_t flen, const char *value, size_t vlen){
    HttpResponseParser_v1 *parser = static_cast<HttpResponseParser_v1*>(data);
    if(flen == 0){
        parser->SetError(INVALID_FIELD_LEN);
        return;
    }
    parser->GetData()->SetHeaders(std::string(field, flen), std::string(value, vlen));
}

void on_response_body(void *data, const char *at, size_t length){
    HttpResponseParser_v1 *parser = static_cast<HttpResponseParser_v1*>(data);
    parser->GetData()->SetBody(std::string(at, length));
}


HttpResponseParser_v1::HttpResponseParser_v1(){
    httpclient_parser_init(&m_parser);
    m_data = std::make_shared<HttpResponse>();
    m_parser.reason_phrase = on_response_reason;
    m_parser.status_code = on_response_status;
    m_parser.chunk_size = on_response_chunk;
    m_parser.http_version = on_response_version;
    m_parser.header_done = on_response_header_done;
    m_parser.last_chunk = on_response_last_chunk;
    m_parser.http_field = on_response_http_field;
}

size_t HttpResponseParser_v1::execute(char* data, size_t len, size_t off){
    size_t offset = httpclient_parser_execute(&m_parser, data, len, off);
    memmove(data, (data + offset), (len - offset));
    return offset;
}
  
int HttpResponseParser_v1::isFinished() const{
    return httpclient_parser_is_finished(&m_parser);
}

std::string HttpResponseParser_v1::GetErrstr() const {
    std::string res ;
    int err = GetError();
    if(err & HTTP_PARSER_ERROR::PARSRE_ERROR){
        res.append(":PARSRE_ERROR");
    }
    if(err & HTTP_PARSER_ERROR::PARSRE_ERROR){
        res.append(":INVALID_HTTP_VERSION");
    }
    if(err & HTTP_PARSER_ERROR::PARSRE_ERROR){
        res.append(":INVALID_FIELD_LEN");
    }
    if(err & HTTP_PARSER_ERROR::PARSRE_ERROR){
        res.append(":INVALID_METHOD");
    }
    if(res.empty()){
        return "NOERROR";
    }
    return res;  
}



// V2版本parser
// 读取到数据触发的回调 适配llhttp
int on_request_data_v2(llhttp_t *llparser, const char *at, size_t length){
    HttpRequestParser_v2 *parser = static_cast<HttpRequestParser_v2*>(llparser->data);
    parser->Getbuffer().append(at, length);
    return llhttp_get_errno(llparser);
}

int on_request_method_complete_v2(llhttp_t *llparser){
    HttpRequestParser_v2 *parser = static_cast<HttpRequestParser_v2*>(llparser->data);
    parser->GetData()->SetMethod(String2HttpMethod(parser->Getbuffer()));
    parser->Getbuffer().clear();
    return llhttp_get_errno(llparser);
}

int on_request_url_complete_v2(llhttp_t *llparser){
    HttpRequestParser_v2 *parser = static_cast<HttpRequestParser_v2*>(llparser->data);
    // TODO:
    parser->GetData()->SetPath(parser->Getbuffer());
    int i = 0;
    while (i < parser->Getbuffer().size()){
        ++i;
    }
    parser->Getbuffer().clear();
    return llhttp_get_errno(llparser);
}

int on_request_body_complete_v2(llhttp_t *llparser){
    HttpRequestParser_v2 *parser = static_cast<HttpRequestParser_v2*>(llparser->data);
    parser->GetData()->SetBody(parser->Getbuffer());
    auto err = llhttp_get_errno(llparser);
    if (err != HPE_OK)
        parser->SetError(err);
    parser->Getbuffer().clear();
    return err;
}

int on_request_version_complete_v2(llhttp_t *llparser){
    HttpRequestParser_v2 *parser = static_cast<HttpRequestParser_v2*>(llparser->data);
    int v = 0;
    if(parser->Getbuffer() == "1.1"){
        v = 0x11;
    }
    else if(parser->Getbuffer() == "1.0"){
        v = 0x10;
    }
    else{
        parser->SetError(HPE_INVALID_VERSION);
        return llhttp_get_errno(llparser);
    }
    parser->GetData()->SetVersion(v);
    parser->Getbuffer().clear();
    return llhttp_get_errno(llparser);
}

int on_request_header_field_complete_v2(llhttp_t *llparser){
    HttpRequestParser_v2 *parser = static_cast<HttpRequestParser_v2*>(llparser->data);
    parser->Getbuffer().push_back(':');
    return llhttp_get_errno(llparser);
}

int on_request_header_value_complete_v2(llhttp_t *llparser){
    HttpRequestParser_v2 *parser = static_cast<HttpRequestParser_v2*>(llparser->data);
    size_t pos = parser->Getbuffer().find(':');
    if(pos == std::string::npos || pos == parser->Getbuffer().size()-1){
        parser->SetError(HPE_INVALID_HEADER_TOKEN);
        return HPE_INVALID_HEADER_TOKEN;
    }
    std::string_view view = parser->Getbuffer();
    parser->GetData()->SetHeaders(parser->Getbuffer().substr(0, pos), view.substr(pos + 1));
    parser->Getbuffer().clear();
    return llhttp_get_errno(llparser);
}



HttpRequestParser_v2::HttpRequestParser_v2()
{
    m_data = std::make_shared<HttpRequest>();
    llhttp_settings_init(&m_settings);
    llhttp_init(&m_parser, HTTP_REQUEST, &m_settings);
    m_parser.data = this;

    m_settings.on_method = on_request_data_v2;
    m_settings.on_url = on_request_data_v2;
    m_settings.on_body = on_request_data_v2;
    m_settings.on_header_field = on_request_data_v2;
    m_settings.on_header_value = on_request_data_v2;
    m_settings.on_version = on_request_data_v2;

    m_settings.on_method_complete = on_request_method_complete_v2;
    m_settings.on_url_complete = on_request_url_complete_v2;
    m_settings.on_message_complete = on_request_body_complete_v2;
    m_settings.on_header_field_complete = on_request_header_field_complete_v2;
    m_settings.on_header_value_complete = on_request_header_value_complete_v2;
    m_settings.on_version_complete = on_request_version_complete_v2;
}

size_t HttpRequestParser_v2::execute(char* data, size_t len, size_t off)
{
    return llhttp_execute(&m_parser, data + off, len);
}

bool HttpRequestParser_v2::isFinished() const
{
    return llhttp_message_needs_eof(&m_parser);
}

std::string HttpRequestParser_v2::GetErrstr() const
{
    return llhttp_errno_name(m_error);
}

std::string HttpRequestParser_v2::GetErrReason() const
{
    return llhttp_get_error_reason(&m_parser);
}

void HttpRequestParser_v2::Reset(HttpRequest::ptr req)
{
    m_error = HPE_OK;
    m_data = req;
    m_buff.clear();
    llhttp_finish(&m_parser);
}



int on_response_data_v2(llhttp_t *llparser, const char *at, size_t length){
    HttpResponseParser_v2 *parser = static_cast<HttpResponseParser_v2*>(llparser->data);
    parser->Getbuffer().append(at, length);
    return llhttp_get_errno(llparser);
}

int on_response_status_complete_v2(llhttp_t *llparser){
    HttpResponseParser_v2 *parser = static_cast<HttpResponseParser_v2*>(llparser->data);
    parser->GetData()->SetStatus(HttpStatus(llparser->status_code));
    parser->GetData()->SetReason(parser->Getbuffer());
    parser->Getbuffer().clear();
    return llhttp_get_errno(llparser);
}

int on_response_body_complete_v2(llhttp_t *llparser){
    HttpResponseParser_v2 *parser = static_cast<HttpResponseParser_v2*>(llparser->data);
    parser->GetData()->SetBody(parser->Getbuffer());
    auto err = llhttp_get_errno(llparser);
    if (err != HPE_OK)
        parser->SetError(err);
    parser->Getbuffer().clear();
    return err;
}

int on_response_version_complete_v2(llhttp_t *llparser){
    HttpResponseParser_v2 *parser = static_cast<HttpResponseParser_v2*>(llparser->data);
    int v = 0;
    if(parser->Getbuffer() == "1.1"){
        v = 0x11;
    }
    else if(parser->Getbuffer() == "1.0"){
        v = 0x10;
    }
    else{
        parser->SetError(HPE_INVALID_VERSION);
        return llhttp_get_errno(llparser);
    }
    parser->GetData()->SetVersion(v);
    parser->Getbuffer().clear();
    return llhttp_get_errno(llparser);
}

int on_response_header_field_complete_v2(llhttp_t *llparser){
    HttpResponseParser_v2 *parser = static_cast<HttpResponseParser_v2*>(llparser->data);
    parser->Getbuffer().push_back(':');
    return llhttp_get_errno(llparser);
}

int on_response_header_value_complete_v2(llhttp_t *llparser){
    HttpResponseParser_v2 *parser = static_cast<HttpResponseParser_v2*>(llparser->data);
    size_t pos = parser->Getbuffer().find(':');
    if(pos == std::string::npos || pos == parser->Getbuffer().size()-1){
        parser->SetError(HPE_INVALID_HEADER_TOKEN);
        return HPE_INVALID_HEADER_TOKEN;
    }
    std::string_view view = parser->Getbuffer();
    parser->GetData()->SetHeaders(parser->Getbuffer().substr(0, pos), view.substr(pos + 1));
    parser->Getbuffer().clear();
    return llhttp_get_errno(llparser);
}

HttpResponseParser_v2::HttpResponseParser_v2()
{
    m_data = std::make_shared<HttpResponse>();
    llhttp_settings_init(&m_settings);
    llhttp_init(&m_parser, HTTP_RESPONSE, &m_settings);
    m_parser.data = this;

    m_settings.on_url = on_response_data_v2;
    m_settings.on_status = on_response_data_v2;
    m_settings.on_body = on_response_data_v2;
    m_settings.on_header_field = on_response_data_v2;
    m_settings.on_header_value = on_response_data_v2;
    m_settings.on_version = on_response_data_v2;
    m_settings.on_method = on_response_data_v2;

    m_settings.on_status_complete = on_response_status_complete_v2;
    m_settings.on_message_complete = on_response_body_complete_v2;
    m_settings.on_header_field_complete = on_response_header_field_complete_v2;
    m_settings.on_header_value_complete = on_response_header_value_complete_v2;
    m_settings.on_version_complete = on_response_version_complete_v2;
}

size_t HttpResponseParser_v2::execute(char* data, size_t len, size_t off)
{
    llhttp_errno_t err = llhttp_execute(&m_parser, data + off, len);
    if (err != HPE_OK)
        SetError(err);
    return err;
}

bool HttpResponseParser_v2::isFinished() const
{
    return llhttp_message_needs_eof(&m_parser) == 1;
}

std::string HttpResponseParser_v2::GetErrstr() const
{
    return llhttp_errno_name(m_error);
}

std::string HttpResponseParser_v2::GetErrReason() const
{
    return llhttp_get_error_reason(&m_parser);
}

void HttpResponseParser_v2::Reset(HttpResponse::ptr res)
{
    m_data = res;
    m_buff.clear();
    m_error = HPE_OK;
    llhttp_finish(&m_parser);
}
} // namespace http

} // namespace server